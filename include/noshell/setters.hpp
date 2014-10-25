#ifndef __NOSHELL_SETTERS_H__
#define __NOSHELL_SETTERS_H__

#include <vector>

#include <ext/stdio_filebuf.h>

namespace noshell {
struct from_to_fd_ref {
  std::vector<int> from;
  int&             to;

  from_to_fd_ref(int f, int& t) : from(1, f), to(t) { }
  from_to_fd_ref(const std::vector<int>& f, int& t) : from(f), to(t) { }
};

struct from_to_stdio_ref {
  std::vector<int> from;
  FILE*&           to;

  from_to_stdio_ref(int f, FILE*& t) : from(1, f), to(t) { }
  from_to_stdio_ref(const std::vector<int>& f, FILE*& t) : from(f), to(t) { }
};

struct from_to_fd {
  std::vector<int> from;
  int              to;
  from_to_fd(int f, int t) : from(1, f), to(t) { }
  from_to_fd(FILE* f, int t) : from(1, fileno(f)), to(t) { }
  from_to_fd(int f, FILE* t) : from(1, f), to(fileno(t)) { }
  from_to_fd(FILE* f, FILE* t) : from(1, fileno(f)), to(fileno(t)) { }
  from_to_fd(const std::vector<int>& f, int t) : from(f), to(t) { }
  from_to_fd(from_to_fd_ref&& r) : from(std::move(r.from)), to(r.to) { }
  from_to_fd(from_to_stdio_ref&& r) : from(std::move(r.from)), to(fileno(r.to)) { }
};

struct from_to_path {
  std::vector<int> from;
  std::string to;
  from_to_path(std::vector<int>&& f, std::string&& p) : from(std::move(f)), to(std::move(p)) { }
  from_to_path(int f, std::string&& p) : from(1, f), to(std::move(p)) { }
  from_to_path(int f, const char* p) : from(1, f), to(p) { }
};

struct process_setup {
  virtual ~process_setup() { }
  virtual bool parent_setup() { return true; }
  virtual bool child_setup() { return true; }
  virtual bool parent_cleanup() { return true; }
};

struct process_setter {
  virtual ~process_setter() { }
  virtual process_setup* make_setup() = 0;
};

// Setup redirection to an already open file descriptor
struct fd_redirection : public process_setup {
  from_to_fd ft;
  fd_redirection(int f, int t) : ft(f, t) { }
  fd_redirection(const std::vector<int>& f, int t) : ft(f, t) { }
  fd_redirection(const from_to_fd& f) : ft(f) { }
  virtual bool child_setup();
};

struct fd_redirection_setter : public process_setter {
  const from_to_fd ft;
  fd_redirection_setter(int f, int t) : ft(f, t) { }
  fd_redirection_setter(std::vector<int>&& f, int t) : ft(std::move(f), t) { }
  fd_redirection_setter(from_to_fd&& f) : ft(std::move(f)) { }
  virtual process_setup* make_setup() { return new fd_redirection(ft); }
};

// Setup a redirection to a named file, either in input or output.
struct path_redirection : public fd_redirection {
  path_redirection(int f, int t) : fd_redirection(f, t) { }
  path_redirection(const std::vector<int>& f, int t) : fd_redirection(f, t) { }
  virtual ~path_redirection();
  virtual bool parent_setup();
};

struct path_redirection_setter : public process_setter {
  enum path_type { READ, WRITE, APPEND };
  const from_to_path     ft;
  const path_type        type;
  path_redirection_setter(int f, const char* p, path_type t = READ) : ft(f, std::string(p)), type(t) { }
  path_redirection_setter(int f, std::string&& p, path_type t = READ) : ft(f, std::move(p)), type(t) { }
  path_redirection_setter(from_to_path&& f, path_type t = READ) : ft(std::move(f)), type(t) { }
  //  path_redirection_setter(int f, const std::string& p, path_type t = READ) : from(f), path(p), type(t) { }
  ~path_redirection_setter() { }
  virtual process_setup* make_setup();
};

// Setups pipes on the stdin and stdout of process to create pipeline.
struct pipeline_redirection : public process_setup {
  int pipe0[2];
  int pipe1[2];
  pipeline_redirection(int p0[2], int p1[2]) {
    std::copy(p0, p0 + 2, pipe0);
    std::copy(p1, p1 + 2, pipe1);
  }
  virtual ~pipeline_redirection();
  virtual bool child_setup();
  virtual bool parent_setup();
};

// Setups a pipe from an output of the child process to a file
// descriptor of the parent process (or the converse from a file
// descriptor of the parent process to the input of the child).
struct fd_pipe_redirection : public process_setup {
  std::vector<int> from;
  int              pipe_dup;
  int              pipe_close;
  fd_pipe_redirection(int f, int d, int c) : from(1, f), pipe_dup(d), pipe_close(c) { }
  fd_pipe_redirection(const std::vector<int> f, int d, int c) : from(f), pipe_dup(d), pipe_close(c) { }
  virtual ~fd_pipe_redirection();
  virtual bool child_setup();
  virtual bool parent_setup();
};

struct fd_pipe_redirection_setter : public process_setter {
  enum pipe_type { READ, WRITE };
  const from_to_fd_ref ft;
  const pipe_type      type;
  fd_pipe_redirection_setter(int f, int& t, pipe_type p) : ft(f, t), type(p) { }
  fd_pipe_redirection_setter(const std::vector<int>& f, int& t, pipe_type p) : ft(std::move(f), t), type(p) { }
  fd_pipe_redirection_setter(from_to_fd_ref&& r, pipe_type p) : ft(std::move(r)), type(p) { }
  virtual process_setup* make_setup();
};

// Same as above but with a stdio FILE*.
struct stdio_pipe_redirection_setter : public fd_pipe_redirection_setter {
  int    fd;
  FILE*& file;
  stdio_pipe_redirection_setter(int f, FILE*& file_, fd_pipe_redirection_setter::pipe_type p)
    : fd_pipe_redirection_setter(f, fd, p)
    , fd(-1)
    , file(file_)
  { }
  stdio_pipe_redirection_setter(from_to_stdio_ref&& ft, fd_pipe_redirection_setter::pipe_type p)
    : fd_pipe_redirection_setter(std::move(ft.from), fd, p)
    , fd(-1)
    , file(ft.to)
  { }
  virtual process_setup* make_setup();
};

// Same as above but with a C++ stream
template<typename ST>
struct from_to_stream_ref {
  typedef ST stream_type;
  std::vector<int> from;
  ST&              to;

  from_to_stream_ref(int f, ST& t) : from(1, f), to(t) { }
  from_to_stream_ref(const std::vector<int>& f, ST& t) : from(f), to(t) { }
};
class istream : public std::istream {
public:
  istream() : std::istream(nullptr) { }
  ~istream() { close(); }
  void close() { delete this->rdbuf(nullptr); }
};
class ostream : public std::ostream {
public:
  ostream() : std::ostream(nullptr) { }
  ~ostream() { close(); }
  void close() { delete this->rdbuf(nullptr); }
};

template<typename ST> struct stream_traits;
template<>
struct stream_traits<istream> {
  static const fd_pipe_redirection_setter::pipe_type type = fd_pipe_redirection_setter::READ;
  static const std::ios::openmode                    mode = std::ios::in;
};
template<>
struct stream_traits<ostream> {
  static const fd_pipe_redirection_setter::pipe_type type = fd_pipe_redirection_setter::WRITE;
  static const std::ios::openmode                    mode = std::ios::out;
};
template<typename ST>
struct stream_pipe_redirection_setter : public fd_pipe_redirection_setter {
  int fd;
  ST&  stream;
  stream_pipe_redirection_setter(int f, ST& s)
    : fd_pipe_redirection_setter(f, fd, stream_traits<ST>::type)
    , fd(-1)
    , stream(s)
  { }
  stream_pipe_redirection_setter(from_to_stream_ref<ST>&& ft)
    : fd_pipe_redirection_setter(std::move(ft.from), fd, stream_traits<ST>::type)
    , fd(-1)
    , stream(ft.to)
  { }
  virtual process_setup* make_setup() {
    process_setup* setup = fd_pipe_redirection_setter::make_setup();
    if(!setup) return nullptr;
    delete stream.rdbuf(new __gnu_cxx::stdio_filebuf<char>(fd, stream_traits<ST>::mode));
    return setup;
  }
};
} // namespace noshell

#endif /* __NOSHELL_SETTERS_H__ */
