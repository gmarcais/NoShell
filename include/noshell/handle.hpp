#ifndef __NOSHELL_HANDLE_H__
#define __NOSHELL_HANDLE_H__

#include <algorithm>
#include <noshell/setters.hpp>

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

  std::string what() const;
};

class Command;
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
  Handle(Handle&& rhs) noexcept : pid(rhs.pid), error(rhs.error), data(rhs.data), setups(std::move(rhs.setups)) { }
  Handle(Command&& rhs);

  bool setup_error() const { return error == SETUP_ERROR; }
  const Errno& err() const { return data.err; }
  bool have_status() const { return error == STATUS; }
  const Status& status() const { return data.status; }
  bool success() const { return !setup_error() && have_status() && status().exited() && status().exit_status() == 0; }

  Handle& set_error(error_types et) { asm("int3"); error = et; return *this; }
  Handle&& return_error(error_types et) { return std::move(set_error(et)); }
  Handle& set_status(int st) { error = STATUS; data.status.value = st; return *this; }
  Handle&& return_status(int st) { return std::move(set_status(st)); }
  Handle& set_errno(int e = errno) { error = SETUP_ERROR; data.err.value = e; return *this; }
  Handle&& return_errno(int e = errno) { return std::move(set_errno(e)); }

  void wait();
};

class PipeLine;
// Return status of a pipeline
class Exit {
  std::vector<Handle> handles;

public:
  Exit() = default;
  Exit(Exit&& rhs) : handles(std::move(rhs.handles)) { }
  Exit(PipeLine&& pipeline);
  bool success() const {
    return std::all_of(handles.begin(), handles.end(), [](const Handle& h) { return h.success(); });
  }
  const Handle& operator[](int i) { return handles[i]; }

  void push_handle(Handle&& h) { handles.push_back(std::move(h)); }
  void wait() { for(auto& h : handles) h.wait(); }
};
} // namespace noshell

#endif /* __NOSHELL_HANDLE_H__ */
