#ifndef __NOSHELL_SETTERS_H__
#define __NOSHELL_SETTERS_H__

#include <vector>

#include <ext/stdio_filebuf.h>

namespace noshell {
// File descriptor type. Behaves like an int, can be initialize from
// an int or FILE*.
struct fd_type {
  int fd;
  fd_type(int x) : fd(x) { }
  fd_type(FILE* x) : fd(fileno(x)) { }
  operator int() const { return fd; }
  operator int&() { return fd; }
};
typedef std::vector<fd_type> fd_list_type;

template<typename T>
struct from_to_ref {
  fd_list_type from;
  T&             to;

  from_to_ref(int f, T& t) : from(1, f), to(t) { }
  from_to_ref(const fd_list_type& f, T& t) : from(f), to(t) { }
};

struct from_to_fd {
  fd_list_type from;
  int          to;
  from_to_fd(int f, int t) : from(1, f), to(t) { }
  from_to_fd(FILE* f, int t) : from(1, fileno(f)), to(t) { }
  from_to_fd(int f, FILE* t) : from(1, f), to(fileno(t)) { }
  from_to_fd(FILE* f, FILE* t) : from(1, fileno(f)), to(fileno(t)) { }
  from_to_fd(const fd_list_type& f, int t) : from(f), to(t) { }
  from_to_fd(from_to_ref<int>&& r) : from(std::move(r.from)), to(r.to) { }
  from_to_fd(from_to_ref<FILE*>&& r) : from(std::move(r.from)), to(fileno(r.to)) { }
};

struct from_to_path {
  fd_list_type from;
  std::string to;
  from_to_path(fd_list_type&& f, std::string&& p) : from(std::move(f)), to(std::move(p)) { }
  from_to_path(int f, std::string&& p) : from(1, f), to(std::move(p)) { }
  from_to_path(int f, const char* p) : from(1, f), to(p) { }
};

struct process_setup {
  virtual ~process_setup() { }
  virtual bool parent_setup(std::string& err) { return true; }
  virtual bool child_setup() { return true; }
  virtual bool parent_cleanup() { return true; }
};

struct process_setter {
  virtual ~process_setter() { }
  virtual process_setup* make_setup(std::string& err) = 0;
};

// Setup redirection to an already open file descriptor
struct fd_redirection : public process_setup {
  from_to_fd ft;
  fd_redirection(int f, int t) : ft(f, t) { }
  fd_redirection(const fd_list_type& f, int t) : ft(f, t) { }
  fd_redirection(const from_to_fd& f) : ft(f) { }
  virtual bool child_setup();
};

struct fd_redirection_setter : public process_setter {
  const from_to_fd ft;
  fd_redirection_setter(int f, int t) : ft(f, t) { }
  fd_redirection_setter(fd_list_type&& f, int t) : ft(std::move(f), t) { }
  fd_redirection_setter(from_to_fd&& f) : ft(std::move(f)) { }
  virtual process_setup* make_setup(std::string& err) { return new fd_redirection(ft); }
};

// Setup a redirection to a named file, either in input or output.
struct path_redirection : public fd_redirection {
  path_redirection(int f, int t) : fd_redirection(f, t) { }
  path_redirection(const fd_list_type& f, int t) : fd_redirection(f, t) { }
  virtual ~path_redirection();
  virtual bool parent_setup(std::string& err);
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
  virtual process_setup* make_setup(std::string& err);
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
  virtual bool parent_setup(std::string& err);
};

// Setups a pipe from an output of the child process to a file
// descriptor of the parent process (or the converse from a file
// descriptor of the parent process to the input of the child).
struct fd_pipe_redirection : public process_setup {
  fd_list_type from;
  int              pipe_dup;
  int              pipe_close;
  fd_pipe_redirection(int f, int d, int c) : from(1, f), pipe_dup(d), pipe_close(c) { }
  fd_pipe_redirection(const fd_list_type f, int d, int c) : from(f), pipe_dup(d), pipe_close(c) { }
  virtual ~fd_pipe_redirection();
  virtual bool child_setup();
  virtual bool parent_setup(std::string& err);
};

struct fd_pipe_redirection_setter : public process_setter {
  enum pipe_type { READ, WRITE };
  const from_to_ref<int> ft;
  const pipe_type        type;
  fd_pipe_redirection_setter(int f, int& t, pipe_type p) : ft(f, t), type(p) { }
  fd_pipe_redirection_setter(const fd_list_type& f, int& t, pipe_type p) : ft(std::move(f), t), type(p) { }
  fd_pipe_redirection_setter(from_to_ref<int>&& r, pipe_type p) : ft(std::move(r)), type(p) { }
  virtual process_setup* make_setup(std::string& err);
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
  stdio_pipe_redirection_setter(from_to_ref<FILE*>&& ft, fd_pipe_redirection_setter::pipe_type p)
    : fd_pipe_redirection_setter(std::move(ft.from), fd, p)
    , fd(-1)
    , file(ft.to)
  { }
  virtual process_setup* make_setup(std::string& err);
};

// Same as above but with a C++ stream
template<typename T>
class base_stream : public T {
public:
  base_stream() : T(nullptr) { }
  ~base_stream() { close(); }
  void open(int fd, std::ios::openmode mode) {
    delete this->rdbuf(new __gnu_cxx::stdio_filebuf<char>(fd, mode));
  }

  void close() { delete this->rdbuf(nullptr); }
  int fd() const {
    auto buf = this->rdbuf();
    return buf ? static_cast<__gnu_cxx::stdio_filebuf<char>*>(buf)->fd() : -1;
  }
};
typedef base_stream<std::istream> istream;
typedef base_stream<std::ostream> ostream;

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
  typedef fd_pipe_redirection_setter super;
  int fd;
  ST&  stream;
  stream_pipe_redirection_setter(int f, ST& s)
    : fd_pipe_redirection_setter(f, fd, stream_traits<ST>::type)
    , fd(-1)
    , stream(s)
  { }
  stream_pipe_redirection_setter(from_to_ref<ST>&& ft)
    : fd_pipe_redirection_setter(std::move(ft.from), fd, stream_traits<ST>::type)
    , fd(-1)
    , stream(ft.to)
  { }
  stream_pipe_redirection_setter(from_to_ref<ST>&& ft, super::pipe_type p)
    : fd_pipe_redirection_setter(std::move(ft.from), fd, stream_traits<ST>::type)
    , fd(-1)
    , stream(ft.to)
  { }
  virtual process_setup* make_setup(std::string& err) {
    process_setup* setup = fd_pipe_redirection_setter::make_setup(err);
    if(!setup) return nullptr;
    stream.open(fd, stream_traits<ST>::mode);
    return setup;
  }
};


// Traits to select the proper setter
template<typename T>
struct setter_traits { };

template<>
struct setter_traits<int> {
  typedef fd_pipe_redirection_setter setter_type;
};

template<>
struct setter_traits<FILE*> {
  typedef stdio_pipe_redirection_setter setter_type;
};

template<>
struct setter_traits<istream> {
  typedef stream_pipe_redirection_setter<istream> setter_type;
};

template<>
struct setter_traits<ostream> {
  typedef stream_pipe_redirection_setter<ostream> setter_type;
};

} // namespace noshell

#endif /* __NOSHELL_SETTERS_H__ */
