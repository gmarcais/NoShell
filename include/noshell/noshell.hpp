#ifndef __NOSHELL_H__
#define __NOSHELL_H__

#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <forward_list>
#include <initializer_list>

// #include <noshell/command.hpp>
#include <noshell/setters.hpp>
#include <noshell/handle.hpp>

namespace noshell {
class Command {
  std::vector<std::string> cmd;
  std::forward_list<std::unique_ptr<process_setter> > setters;

public:
  Command(Command& rhs) = delete;
  Command(const Command& rhs) = delete;
  Command(Command&& rhs) noexcept : cmd(std::move(rhs.cmd)), setters(std::move(rhs.setters)) { }
  explicit Command(std::vector<std::string>&& c) : cmd(std::move(c)) { }
  template<typename Iterator>
  Command(Iterator begin, Iterator end) : cmd(begin, end) { }
  Command(std::initializer_list<std::string> l) : cmd(l) { }

  void push_setter(process_setter* setter);

  Handle run(process_setup* setup = nullptr);
  Handle run_wait();
};

class PipeLine {
  std::vector<Command> commands;

public:
  PipeLine() { static_assert(std::is_nothrow_move_constructible<Command>::value, "Command must be movable noexcept"); }
  PipeLine(Command&& c) { push_command(std::move(c)); }
  Exit run();
  Exit run_wait();

  void push_command(Command&& c) { commands.push_back(std::move(c)); }

  friend PipeLine& operator|(PipeLine& p1, PipeLine&& p2);
  friend PipeLine& operator>(PipeLine& pl, const from_to& ft);
  friend PipeLine& operator>(PipeLine& pl, const char* path);
  friend PipeLine& operator>>(PipeLine& pl, const char* path);
  friend PipeLine& operator<(PipeLine& pl, const from_to& ft);
  friend PipeLine& operator<(PipeLine& pl, const char* path);
};

inline PipeLine C(std::initializer_list<std::string> l) { return PipeLine(Command(l)); }
// inline PipeLine C(std::string s) { return PipeLine(Command(std::vector<std::string>(1, s))); }
inline PipeLine create_pipe(std::vector<std::string>& cmds) { return PipeLine(Command(std::move(cmds))); }
template<typename... Args>
PipeLine create_pipe(std::vector<std::string>& cmds, std::string s, Args... args) {
  cmds.push_back(s);
  return create_pipe(cmds, args...);
}
template<typename... Args>
PipeLine C(Args... args) {
  std::vector<std::string> cmds;
  return create_pipe(cmds, args...);
}

PipeLine& operator>(PipeLine& cmd, const from_to& ft);
inline PipeLine&& operator>(PipeLine&& cmd, const from_to& ft) { cmd > ft; return std::move(cmd); }
inline PipeLine& operator>(PipeLine& cmd, int fd) { return cmd > from_to(1, fd); }
inline PipeLine&& operator>(PipeLine&& cmd, int fd) { return std::move(cmd > fd); }

PipeLine& operator>(PipeLine& pl, const char* path);
inline PipeLine&& operator>(PipeLine&& pl, const char* path) { return std::move(pl > path); }
inline PipeLine& operator>(PipeLine& pl, std::string& path) { return pl > path.c_str(); }
inline PipeLine&& operator>(PipeLine&& pl, std::string& path) { return std::move(pl > path.c_str()); }

PipeLine& operator>>(PipeLine& pl, const char* path);
inline PipeLine&& operator>>(PipeLine&& pl, const char* path) { return std::move(pl >> path); }
inline PipeLine& operator>>(PipeLine& pl, const std::string& path) { return pl >> path.c_str(); }
inline PipeLine&& operator>>(PipeLine&& pl, const std::string& path) { return std::move(pl >> path.c_str()); }

PipeLine& operator<(PipeLine& cmd, const from_to& ft);
inline PipeLine&& operator<(PipeLine&& cmd, const from_to& ft) { cmd < ft; return std::move(cmd); }
inline PipeLine& operator<(PipeLine& cmd, int fd) { return cmd < from_to(0, fd); }
inline PipeLine&& operator<(PipeLine&& cmd, int fd) { return std::move(cmd < fd); }

PipeLine& operator<(PipeLine& pl, const char* path);
inline PipeLine&& operator<(PipeLine&& pl, const char* path) { return std::move(pl < path); }
inline PipeLine& operator<(PipeLine& pl, std::string& path) { return pl < path.c_str(); }
inline PipeLine&& operator<(PipeLine&& pl, std::string& path) { return std::move(pl < path.c_str()); }

PipeLine& operator|(PipeLine& p1, PipeLine&& p2);
inline PipeLine&& operator|(PipeLine&& p1, PipeLine&& p2) { return std::move(p1 | std::move(p2)); }
} // namespace noshell

#endif /* __NOSHELL_H__ */
