#include <unistd.h>
#include <fcntl.h>
#include <iterator>

#include <ext/stdio_filebuf.h>

#include <noshell/utils.hpp>
#include <noshell/setters.hpp>

namespace noshell {
bool move_fd(int& fd, int above) {
  const int flags = fcntl(fd, F_GETFD);
  if(flags == -1) return false;
  int new_fd;
  if(!safe_dup(fd, new_fd, (flags & FD_CLOEXEC) != 0, above)) return false;
  safe_close(fd);
  fd = new_fd;
  return true;
}

bool fd_type::move(int above) { return move_fd(fd, above); }

bool fix_collision(int& fd, const std::set<int>& r) {
  if(r.empty()) return true;
  if(r.find(fd) == r.cend()) return true;
  return move_fd(fd, *--r.cend() + 1);
}

process_setup* fd_redirection_setter::make_setup(std::string& err, std::set<int>& rfds) {
  for(auto it : ft.from)
    rfds.insert(it);
  return new fd_redirection(ft);
}

bool fd_redirection::child_setup() {
  bool success   = true;

  for(auto it : ft.from)
    if(!safe_dup2_no_close(ft.to, it))
      success = false;
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

process_setup* path_redirection_setter::make_setup(std::string& err, std::set<int>& rfds) {
  for(auto it : ft.from)
    rfds.insert(it);
  if(type == READ) {
    int flags = O_RDONLY;
    int to = open(ft.to.c_str(), flags);
    if(to == -1) {
      save_restore_errno sre;
      err = "Failed to open the file '" + ft.to + "' for reading";
      return nullptr;
    }
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

process_setup* fd_pipe_redirection_setter::make_setup(std::string& err, std::set<int>& rfds) {
  for(auto it : ft.from)
    rfds.insert(it);
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

process_setup* stdio_pipe_redirection_setter::make_setup(std::string& err, std::set<int>& rfds) {
  process_setup* setup = fd_pipe_redirection_setter::make_setup(err, rfds);
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
