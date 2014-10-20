#ifndef __NOSHELL_H__
#define __NOSHELL_H__

#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <initializer_list>

namespace noshell {
struct Status {
  int value;

  bool exited() const { return WIFEXITED(value); }
  int exit_status() const { return WEXITSTATUS(value); }
  bool signaled() const { return WIFSIGNALED(value); }
  int term_sig() const { return WTERMSIG(value); }
  bool core_dump() const {
#ifdef WCOREDUMP
    return WCOREDUMP(value);
#else
    return false;
#endif
  }
};

struct Errno {
  int value;

  std::string what();
};

struct Handle {
  enum error_types { NO_ERROR, SETUP_ERROR, STATUS };
  pid_t       pid;
  error_types error;
  union {
    Status    status;
    Errno     err;
  } data;

  Handle() : pid(-1), error(NO_ERROR) { }
  Handle(Handle&& rhs) : pid(rhs.pid), error(rhs.error), data(rhs.data) { }

  bool setup_error() const { return error == SETUP_ERROR; }
  const Errno& err() const { return data.err; }
  bool have_status() const { return error == STATUS; }
  const Status& status() const { return data.status; }

  Handle& set_error(error_types et) { error = et; return *this; }
  Handle&& return_error(error_types et) { return std::move(set_error(et)); }
  Handle& set_status(int st) { error = STATUS; data.status.value = st; return *this; }
  Handle&& return_status(int st) { return std::move(set_status(st)); }
  Handle& set_errno(int e = errno) { error = SETUP_ERROR; data.err.value = e; return *this; }
  Handle&& return_errno(int e = errno) { return std::move(set_errno(e)); }

  void wait();
};

// class Pipe {
//   std::vector<pid_t> pids_;
//   std::vector<int> statuses_;
// };


// PipeLine cmd();

class Command {
  std::vector<std::string> cmd;

public:
  Command(Command&& rhs) : cmd(std::move(rhs.cmd)) { }
  Command(std::vector<std::string>&& c) : cmd(std::move(c)) { }
  template<typename Iterator>
  Command(Iterator begin, Iterator end) : cmd(begin, end) { }
  Command(std::initializer_list<std::string> l) : cmd(l) { }

  Handle run();
  Handle run_wait();
};
} // namespace noshell

#endif /* __NOSHELL_H__ */
