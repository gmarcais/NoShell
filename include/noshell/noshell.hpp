#ifndef __NOSHELL_H__
#define __NOSHELL_H__

#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <forward_list>
#include <initializer_list>

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
  bool                 auto_wait;

public:
  PipeLine() : auto_wait(true) {
    static_assert(std::is_nothrow_move_constructible<Command>::value, "Command must be movable noexcept");
  }
  PipeLine(Command&& c) : auto_wait(true) { push_command(std::move(c)); }
  Exit run();
  Exit run_wait();
  Exit run_wait_auto();

  void push_command(Command&& c) { commands.push_back(std::move(c)); }

  friend PipeLine& operator|(PipeLine& p1, PipeLine&& p2);
  friend PipeLine& operator|(PipeLine& pl, int& fd);
  friend PipeLine& operator|(int& fd, PipeLine& pl);
  friend PipeLine& operator|(PipeLine& pl, FILE*& f);
  friend PipeLine& operator|(FILE*& f, PipeLine& pl);
  friend PipeLine& operator|(PipeLine& pl, istream& is);
  friend PipeLine& operator|(ostream& os, PipeLine& pl);
  friend PipeLine& operator>(PipeLine& pl, const from_to& ft);
  friend PipeLine& operator>(PipeLine& pl, const char* path);
  friend PipeLine& operator>>(PipeLine& pl, const char* path);
  friend PipeLine& operator<(PipeLine& pl, const from_to& ft);
  friend PipeLine& operator<(PipeLine& pl, const char* path);
};

// Structure to create pipeline object. Works with arbitrary number of
// arguments, either string, const char* or anything that can be
// transformed to a string with std::to_string.
template<typename T>
inline std::string convert_to_string(T x) { return std::to_string(x); }
template<>
inline std::string convert_to_string(std::string x) { return x; }
template<>
inline std::string convert_to_string(const char* x) { return std::string(x); }

template<typename T, typename... Args>
struct create_pipe {
  static PipeLine append(std::vector<std::string>& cmds, T x, Args... args) {
    cmds.push_back(convert_to_string(x));
    return create_pipe<Args...>::append(cmds, args...);
  }
};
template<typename T>
struct create_pipe<T> {
  static PipeLine append(std::vector<std::string>& cmds, T x) {
    cmds.push_back(convert_to_string(x));
    return PipeLine(Command(std::move(cmds)));
  }
};

inline PipeLine C(const std::vector<std::string>& l) { return PipeLine(Command(l.cbegin(), l.cend())); }
inline PipeLine C(std::initializer_list<std::string> l) { return PipeLine(Command(l)); }
template<typename... Args>
PipeLine C(Args... args) {
  std::vector<std::string> cmds;
  return create_pipe<Args...>::append(cmds, args...);
}

// Define a literal operator
namespace literal {
struct literal_create_pipe {
  std::vector<std::string> cmds;
  literal_create_pipe(std::string&& c) { cmds.push_back(std::move(c)); }
  template<typename... Args>
  PipeLine operator()(Args... args) { return create_pipe<Args...>::append(cmds, args...); }
  PipeLine operator()() { return PipeLine(Command(std::move(cmds))); }
};
inline literal_create_pipe operator""_C(const char* c, size_t s) { return literal_create_pipe(std::string(c, s)); }
};

PipeLine& operator>(PipeLine& cmd, const from_to& ft);
inline PipeLine&& operator>(PipeLine&& cmd, const from_to& ft) { return std::move(cmd > ft); }
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
inline PipeLine&& operator<(PipeLine&& cmd, const from_to& ft) { return std::move(cmd < ft); }
inline PipeLine& operator<(PipeLine& cmd, int fd) { return cmd < from_to(0, fd); }
inline PipeLine&& operator<(PipeLine&& cmd, int fd) { return std::move(cmd < fd); }

PipeLine& operator<(PipeLine& pl, const char* path);
inline PipeLine&& operator<(PipeLine&& pl, const char* path) { return std::move(pl < path); }
inline PipeLine& operator<(PipeLine& pl, std::string& path) { return pl < path.c_str(); }
inline PipeLine&& operator<(PipeLine&& pl, std::string& path) { return std::move(pl < path.c_str()); }

PipeLine& operator|(PipeLine& p1, PipeLine&& p2);
inline PipeLine&& operator|(PipeLine&& p1, PipeLine&& p2) { return std::move(p1 | std::move(p2)); }

PipeLine& operator|(PipeLine& pl, int& fd);
inline PipeLine&& operator|(PipeLine&& pl, int& fd) { return std::move(pl | fd); }

PipeLine& operator|(int& fd, PipeLine& pl);
inline PipeLine&& operator|(int& fd, PipeLine&& pl) { return std::move(fd | pl); }

PipeLine& operator|(PipeLine& pl, FILE*& f);
inline PipeLine&& operator|(PipeLine&& pl, FILE*& f) { return std::move(pl | f); }

PipeLine& operator|(FILE*& f, PipeLine& pl);
inline PipeLine&& operator|(FILE*& f, PipeLine&& pl)  { return std::move(f | pl); }

PipeLine& operator|(PipeLine& pl, istream& is);
inline PipeLine&& operator|(PipeLine&& pl, istream& is) { return std::move(pl | is); }

PipeLine& operator|(ostream& os, PipeLine& pl);
inline PipeLine&& operator|(ostream& os, PipeLine&& pl) { return std::move(os | pl); }
} // namespace noshell

#endif /* __NOSHELL_H__ */
