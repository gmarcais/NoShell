#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <noshell/noshell.hpp>

namespace noshell {
// Select the correct version (GNU or XSI) version of
// ::strerror_r. noshell::strerror_r behaves like the GNU version of strerror_r,
// regardless of which version is provided by the system.
inline const char* strerror__(char* buf, int res) {
  return res != -1 ? buf : "error";
}
inline const char* strerror__(char* buf, char* res) {
  return res;
}
inline const char* strerror_r(int err, char* buf, size_t buflen) {
  return strerror__(buf, ::strerror_r(err, buf, buflen));
}

std::string Errno::what() const {
  char buf[256];
  return std::string(strerror_r(value, buf, sizeof(buf)));
}

void send_errno_to_pipe(int fd) {
  int save_errno = errno;

  while(true) {
    ssize_t bytes = write(fd, &save_errno, sizeof(save_errno));
    if(bytes != -1 || errno != EINTR) return; // Failure we can't handle! Should not happen!
  }
}

bool setup_exec_child(std::forward_list<std::unique_ptr<process_setup> >& setups, const std::vector<std::string>& cmd) {
  for(auto& it : setups) {
    if(!it->child_setup())
      return false;
  }

  std::vector<const char*> argv(cmd.size() + 1);
  for(size_t i = 0; i < cmd.size(); ++i)
    argv[i] = cmd[i].data();
  argv[cmd.size()] = nullptr;
  execvp(argv[0], (char**)argv.data());
  return false;
}

struct auto_close {
  int fd;
  auto_close(int i) : fd(i) { }
  ~auto_close() { close(fd); }
};

Handle Command::run(process_setup* last_setup) {
  Handle ret;

  // Create the setups
  // TODO: error catching
  if(last_setup)
    ret.setups.push_front(std::unique_ptr<process_setup>(last_setup));
  for(auto& it : setters) {
    process_setup* new_setup = it->make_setup();
    if(!new_setup) return ret.return_errno();
    ret.setups.push_front(std::unique_ptr<process_setup>(new_setup));
  }

  // Create communication pipe and fork, setup and exec child
  int pipe_fds[2];
  if(pipe2(pipe_fds, O_CLOEXEC) == -1)
    return ret.return_errno();

  switch(ret.pid = fork()) {
  case -1: return ret.return_errno();

  case 0:
    close(pipe_fds[0]);
    setup_exec_child(ret.setups, cmd);
    send_errno_to_pipe(pipe_fds[1]);
    exit(0);

  default: break;
  }

  // Parent setup and wait for child exec
  close(pipe_fds[1]);
  auto_close close_pipe0(pipe_fds[0]);

  for(auto& it : ret.setups) {
    if(!it->parent_setup())
      return ret.return_errno();
  }

  int recv_errno;
  while(true) {
    ssize_t bytes = read(pipe_fds[0], &recv_errno, sizeof(recv_errno));
    switch(bytes) {
    case -1:
      if(errno == EINTR) break;
      return ret.return_errno();

    case 0: return ret;

    default: return ret.return_errno(recv_errno);
    }
  }
}

Handle Command::run_wait() {
  Handle res = this->run();
  if(!res.setup_error())
    res.wait();
  return res;
}

void Command::push_setter(process_setter* setter) {
  setters.push_front(std::unique_ptr<process_setter>(setter));
}

Handle::Handle(Command&& rhs) : Handle(rhs.run_wait()) { }

void Handle::wait() {
  if(error != NO_ERROR) return;
  pid_t res;
  int   status;
  while(true) {
    res = waitpid(pid, &status, 0);
    if(res != -1 || errno != EINTR) break;
  }
  if(res == -1)
    set_errno();
  else
    set_status(status);
}

Exit::Exit(PipeLine&& rhs) : Exit(rhs.run_wait()) { }

Exit PipeLine::run() {
  Exit ret;

  auto it = commands.begin();
  if(it == commands.end()) return ret;
  auto pit = it;
  int pfds[2] = { -1, -1 };
  for(++it; it != commands.end(); pit = it, ++it) {
    int fds[2];
    if(pipe(fds) == -1) exit(1); // TODO: handle error
    ret.push_handle(pit->run(new pipeline_redirection(pfds, fds)));
    std::copy(fds, fds + 2, pfds);
  }
  int fds[2] = { -1, -1 };
  ret.push_handle(pit->run(new pipeline_redirection(pfds, fds)));

  return ret;
}

Exit PipeLine::run_wait() {
  Exit ret = run();
  ret.wait();
  return ret;
}

// Redirection operators
PipeLine& operator>(PipeLine& pl, const from_to& ft) {
  if(!pl.commands.empty())
    pl.commands.back().push_setter(new fd_redirection_setter(ft.from, ft.to));
  return pl;
}

PipeLine& operator|(PipeLine& p1, PipeLine&& p2) {
  for(auto& it : p2.commands)
    p1.push_command(std::move(it));
  return p1;
}

PipeLine& operator>(PipeLine& pl, const char* path) {
  pl.commands.back().push_setter(new path_redirection_setter(1, path, path_redirection_setter::WRITE));
  return pl;
}

PipeLine& operator>>(PipeLine& pl, const char* path) {
  pl.commands.back().push_setter(new path_redirection_setter(1, path, path_redirection_setter::APPEND));
  return pl;
}

PipeLine& operator<(PipeLine& pl, const from_to& ft) {
  pl.commands.back().push_setter(new fd_redirection_setter(ft.from, ft.to));
  return pl;
}

PipeLine& operator<(PipeLine& pl, const char* path) {
  pl.commands.back().push_setter(new path_redirection_setter(0, path, path_redirection_setter::READ));
  return pl;
}
} // namespace noshell
