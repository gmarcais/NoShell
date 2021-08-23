#ifndef __NOSHELL_HANDLE_H__
#define __NOSHELL_HANDLE_H__

#include <forward_list>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <cstring>
#include <noshell/setters.hpp>

namespace noshell {
struct Status {
  int value;

  bool exited() const { return WIFEXITED(value); }
  int exit_status() const { return WEXITSTATUS(value); }
  bool signaled() const { return WIFSIGNALED(value); }
  int term_sig() const { return WTERMSIG(value); }
  std::string signal() const { return std::string(strsignal(WTERMSIG(value))); }
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
inline std::chrono::microseconds to_microseconds(const struct timeval& tp) {
  return std::chrono::microseconds((uint64_t)tp.tv_sec * (uint64_t)1000000 + (uint64_t)tp.tv_usec);
}
typedef std::forward_list<std::unique_ptr<process_setup> > setup_list_type;
struct Handle {

  enum error_types { NO_ERROR, SETUP_ERROR, STATUS };
  pid_t       pid;
  error_types error;
  union {
    Status    status;
    Errno     err;
  } data;
  setup_list_type setups;       // setup hooks
  struct rusage   resources;
  std::string     message;      // error message

  Handle() : pid(-1), error(NO_ERROR) { }
  Handle(Handle&& rhs) noexcept
    : pid(rhs.pid)
    , error(rhs.error)
    , data(rhs.data)
    , setups(std::move(rhs.setups))
    , message(std::move(rhs.message))
  { }
  Handle(Command&& rhs);

  bool setup_error() const { return error == SETUP_ERROR; }
  const Errno& err() const { return data.err; }
  std::string what() const { return message + ": " + data.err.what(); }
  bool have_status() const { return error == STATUS; }
  const Status& status() const { return data.status; }
  bool success(const bool ignore_sigpipe = false) const {
    return !setup_error() && have_status() &&
      ((status().exited() && status().exit_status() == 0) ||
       (ignore_sigpipe && status().signaled() && status().term_sig() == SIGPIPE));
  }

  Handle& set_error(error_types et) { asm("int3"); error = et; return *this; }
  Handle&& return_error(error_types et) { return std::move(set_error(et)); }
  Handle& set_status(int st) { error = STATUS; data.status.value = st; return *this; }
  Handle&& return_status(int st) { return std::move(set_status(st)); }
  Handle& set_errno(int e = errno) { error = SETUP_ERROR; data.err.value = e; return *this; }
  Handle&& return_errno(int e = errno) { return std::move(set_errno(e)); }

  // Some easy access to resource usage statistics
  std::chrono::microseconds user_time() const { return to_microseconds(resources.ru_utime); }
  std::chrono::microseconds system_time() const { return to_microseconds(resources.ru_stime); }
  long maximum_rss() const { return resources.ru_maxrss; }
  long minor_faults() const { return resources.ru_minflt; }
  long major_faults() const { return resources.ru_majflt; }

  void wait();
};

class PipeLine;
class Failures {
  const std::vector<Handle>& handles;

  class iterator : public std::iterator<std::forward_iterator_tag, Handle> {
    typedef std::vector<Handle>::const_iterator handle_iterator;
    handle_iterator            it;
    const std::vector<Handle>& handles;
  public:
    iterator(const std::vector<Handle>& h) : it(h.cbegin()), handles(h) { }
    iterator(const std::vector<Handle>& h, handle_iterator i) : it(i), handles(h) { }
    //    iterator(const handle_iterator i) : it(i) { }
    iterator(const iterator& rhs) : it(rhs.it), handles(rhs.handles) { }
    iterator& operator++() { for(++it; it != handles.cend() && it->success(); ++it) { } return *this; }
    iterator operator++(int) { iterator cit(*this); operator++(); return cit; }
    bool operator==(const iterator& rhs) { return it == rhs.it; }
    bool operator!=(const iterator& rhs) { return it != rhs.it; }
    const Handle& operator*() const { return *it; }
    const Handle* operator->() const { return &*it; }
    ssize_t id() const { return it - handles.cbegin(); }
  };

public:
  Failures(const std::vector<Handle>& h) : handles(h) { }
  iterator begin() const { return iterator(handles); }
  iterator end() const { return iterator(handles, handles.cend()); }
};
// Return status of a pipeline
class Exit {
  std::vector<Handle>                         handles;
  typedef std::vector<Handle>::const_iterator const_iterator;

public:
  Exit() = default;
  Exit(Exit&& rhs) : handles(std::move(rhs.handles)) { }
  Exit(PipeLine&& pipeline);
  bool success(const bool ignore_sigpipe = false) const {
    return std::all_of(handles.begin(), handles.end(), [=](const Handle& h) { return h.success(ignore_sigpipe); });
  }
  Failures failures() const { return Failures(handles); }
  const Handle& operator[](int i) { return handles[i]; }

  ssize_t id(const Handle& h) { return &h - handles.data(); }
  const_iterator begin() const { return handles.begin(); }
  const_iterator end() const { return handles.end(); }


  void push_handle(Handle&& h) { handles.push_back(std::move(h)); }
  void wait() { for(auto& h : handles) h.wait(); }
};
} // namespace noshell

#endif /* __NOSHELL_HANDLE_H__ */
