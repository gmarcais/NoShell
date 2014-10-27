#include <unistd.h>
#include <fcntl.h>
#include <iterator>

#include <ext/stdio_filebuf.h>

#include <noshell/setters.hpp>

namespace noshell {
struct save_restore_errno {
  const int save_errno;
  save_restore_errno() : save_errno(errno) { }
  ~save_restore_errno() { errno = save_errno; }
};

bool safe_close(int& fd) {
  if(fd == -1) return true;
  while(close(fd) == -1) {
    if(errno == EINTR) continue;
    return false;
  }
  fd = -1;
  return true;
}

bool safe_dup2_no_close(int to, int from) {
  if(to == -1) return true;
  int res;
  while((res = dup2(to, from) == -1)) {
    if(errno == EINTR || errno == EBUSY) continue;
    return false;
  }
  return true;
}

bool safe_dup2(int& to, int from) {
  if(safe_dup2_no_close(to, from))
    return safe_close(to);
  return false;
}

bool fd_redirection::child_setup() {
  bool success   = true;
  bool collision = false;

  for(auto it : ft.from) {
    if(ft.to != it) {
      if(!safe_dup2_no_close(ft.to, it))
        success = false;
    } else {
      collision = true;
    }
  }

  if(!collision)
    if(!safe_close(ft.to))
      success = false;

  return success;
}

pipeline_redirection::~pipeline_redirection() {
  safe_close(pipe0[0]);
  safe_close(pipe0[1]);
  safe_close(pipe1[0]);
  safe_close(pipe1[1]);
}

bool pipeline_redirection::child_setup() {
  safe_close(pipe0[1]);
  safe_close(pipe1[0]);
  return safe_dup2(pipe0[0], 0) && safe_dup2(pipe1[1], 1);
}

bool pipeline_redirection::parent_setup(std::string& err) {
  safe_close(pipe0[0]);
  safe_close(pipe1[1]);
  return true;
}

process_setup* path_redirection_setter::make_setup(std::string& err) {
  if(type == READ) {
    int flags = O_RDONLY;
    int to = open(ft.to.c_str(), flags);
    if(to == -1) {
      save_restore_errno sre;
      err = "Failed to open the file '" + ft.to + "' for reading";
      return nullptr;
    }
    printf("path redirection %d %d\n", (int)ft.from[0], to);
    return new path_redirection(ft.from, to);
  } else {
    int flags = O_WRONLY|O_CREAT;
    if(type == APPEND)
      flags |= O_APPEND;
    else
      flags |= O_TRUNC;
    mode_t mode =  S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
    int to = open(ft.to.c_str(), flags, mode);
    if(to == -1) {
      save_restore_errno sre;
      err = "Failed to open the file '" + ft.to + "' for writing";
      return nullptr;
    }
    return new path_redirection(ft.from, to);
  }
}

bool path_redirection::parent_setup(std::string& err) { safe_close(ft.to); return true; }
path_redirection::~path_redirection() { safe_close(ft.to); }

process_setup* fd_pipe_redirection_setter::make_setup(std::string& err) {
  int fds[2];
  if(pipe(fds) == -1) {
    save_restore_errno sre;
    err = "Failed to create pipes for pipe redirection";
    return nullptr;
  }
  const int nb = (type != READ);
  ft.to        = fds[nb];
  return new fd_pipe_redirection(ft.from, fds[1 - nb], fds[nb]);
}

bool fd_pipe_redirection::child_setup() {
  bool success  = safe_close(pipe_close);
  for(auto it : from)
    if(!safe_dup2_no_close(pipe_dup, it))
      success = false;
  if(!safe_close(pipe_dup))
    success = false;
  return success;
}

bool fd_pipe_redirection::parent_setup(std::string& err) { safe_close(pipe_dup); return true; }
fd_pipe_redirection::~fd_pipe_redirection() { safe_close(pipe_dup); }

process_setup* stdio_pipe_redirection_setter::make_setup(std::string& err) {
  process_setup* setup = fd_pipe_redirection_setter::make_setup(err);
  if(!setup) return nullptr;
  file = fdopen(fd, fd_pipe_redirection_setter::type == READ ? "r" : "w");
  if(!file) {
    save_restore_errno sre;
    err = "fdopen failed to associate a stream from the file descriptor";
    delete setup;
    return nullptr;
  }
  return setup;
}
} // namespace noshell
