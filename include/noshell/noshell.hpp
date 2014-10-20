#ifndef __NOSHELL_H__
#define __NOSHELL_H__

#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <forward_list>
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

struct process_setup {
  virtual ~process_setup() { }
  virtual bool parent_setup(std::string* e) { return true; }
  virtual bool child_setup() { return true; }
  virtual bool parent_cleanup(std::string* e) { return true; }
};

struct process_setter {
  virtual process_setup* make_setup() = 0;
};

struct Handle {
  enum error_types { NO_ERROR, SETUP_ERROR, STATUS };
  pid_t       pid;
  error_types error;
  union {
    Status    status;
    Errno     err;
  } data;
  std::forward_list<std::unique_ptr<process_setup> > setups;

  Handle() : pid(-1), error(NO_ERROR) { }
  Handle(Handle&& rhs) : pid(rhs.pid), error(rhs.error), data(rhs.data), setups(std::move(rhs.setups)) { }

  bool setup_error() const { return error == SETUP_ERROR; }
  const Errno& err() const { return data.err; }
  bool have_status() const { return error == STATUS; }
  const Status& status() const { return data.status; }
  bool success() const { return !setup_error() && have_status() && status().exited() && status().exit_status() == 0; }

  Handle& set_error(error_types et) { error = et; return *this; }
  Handle&& return_error(error_types et) { return std::move(set_error(et)); }
  Handle& set_status(int st) { error = STATUS; data.status.value = st; return *this; }
  Handle&& return_status(int st) { return std::move(set_status(st)); }
  Handle& set_errno(int e = errno) { error = SETUP_ERROR; data.err.value = e; return *this; }
  Handle&& return_errno(int e = errno) { return std::move(set_errno(e)); }

  void wait();
};

struct fd_redirection : public process_setup {
  const int from, to;
  fd_redirection(int f, int t) : from(f), to(t) { }
  virtual bool child_setup();
};

struct fd_redirection_setter : public process_setter {
  const int from, to;
  fd_redirection_setter(int f, int t) : from(f), to(t) { }
  process_setup* make_setup() { return new fd_redirection(from, to); }
};

class Command {
  std::vector<std::string> cmd;
  std::forward_list<std::unique_ptr<process_setter> > setters;

public:
  Command(Command& rhs) = delete;
  Command(const Command& rhs) = delete;
  Command(Command&& rhs) : cmd(std::move(rhs.cmd)), setters(std::move(rhs.setters)) { }
  explicit Command(std::vector<std::string>&& c) : cmd(std::move(c)) { }
  template<typename Iterator>
  Command(Iterator begin, Iterator end) : cmd(begin, end) { }
  Command(std::initializer_list<std::string> l) : cmd(l) { }

  void push_setter(process_setter* setter);
  size_t nb_setters() const { return std::distance(setters.begin(), setters.end()); }

  Handle run();
  Handle run_wait();
};

// class PipeLine {
  
// };

//Command& operator>(Command& cmd, int fd)
struct from_to {
  int from, to;
  from_to(int f, int t) : from(f), to(t) { }
};
Command& operator>(Command& cmd, from_to ft);
inline Command&& operator>(Command&& cmd, from_to ft) { cmd > ft; printf("> nb_setters %lu\n", cmd.nb_setters()); return std::move(cmd); }
inline Command& operator>(Command& cmd, int fd) { return cmd > from_to(1, fd); }
inline Command&& operator>(Command&& cmd, int fd) { return std::move(cmd > fd); }
} // namespace noshell

#endif /* __NOSHELL_H__ */
