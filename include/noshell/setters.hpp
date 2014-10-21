#ifndef __NOSHELL_SETTERS_H__
#define __NOSHELL_SETTERS_H__

namespace noshell {
struct from_to {
  int from, to;
  from_to(int f, int t) : from(f), to(t) { }
  from_to(FILE* f, int t) : from(fileno(f)), to(t) { }
  from_to(int f, FILE* t) : from(f), to(fileno(t)) { }
  from_to(FILE* f, FILE* t) : from(fileno(f)), to(fileno(t)) { }
};


struct process_setup {
  virtual ~process_setup() { }
  virtual bool parent_setup() { return true; }
  virtual bool child_setup() { return true; }
  virtual bool parent_cleanup() { return true; }
};

struct process_setter {
  virtual process_setup* make_setup() = 0;
};

struct fd_redirection : public process_setup {
  int from, to;
  fd_redirection(int f, int t) : from(f), to(t) { }
  virtual bool child_setup();
};

struct fd_redirection_setter : public process_setter {
  const int from, to;
  fd_redirection_setter(int f, int t) : from(f), to(t) { }
  virtual process_setup* make_setup() { return new fd_redirection(from, to); }
};

struct path_redirection : public fd_redirection {
  path_redirection(int f, int t) : fd_redirection(f, t) { }
  virtual ~path_redirection();
  virtual bool parent_setup();
};

struct path_redirection_setter : public process_setter {
  enum path_type { READ, WRITE, APPEND };
  const int         from;
  const std::string path;
  const path_type   type;
  path_redirection_setter(int f, const char* p, path_type t = READ) : from(f), path(p), type(t) { }
  path_redirection_setter(int f, const std::string& p, path_type t = READ) : from(f), path(p), type(t) { }
  virtual process_setup* make_setup();
};

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
} // namespace noshell

#endif /* __NOSHELL_SETTERS_H__ */
