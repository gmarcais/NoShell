#ifndef __UTILS_H__
#define __UTILS_H__

#include <cerrno>

namespace noshell {
// Save the current value of errno and restores it on destruction.
struct save_restore_errno {
  const int save_errno;
  save_restore_errno() : save_errno(errno) { }
  ~save_restore_errno() { errno = save_errno; }
};

// Duplicate file descriptor fd. Return true if successful.
bool safe_dup(int fd, int& new_fd, bool set_cloexec = false, int above = 0);

// Close the file descriptor fd (unless its value is -1) and set it to
// -1 if successful. Return true if successful.
bool safe_close(int& fd);

// Dup the file descriptor <to> into the file descriptor <from>,
// unless <to> is -1 (then it is a noop). Returns true if successful.
bool safe_dup2_no_close(int to, int from);

// Same as safe_dup2_no_close but in addition closes the file
// descriptor <to> and sets it to -1. Returns true if successful.
bool safe_dup2(int& to, int from);

// Automatically close a file descriptor on destruction
struct auto_close {
  int fd;
  auto_close(int i) : fd(i) { }
  ~auto_close() { safe_close(fd); }
};

} // namespace noshell
#endif /* __UTILS_H__ */
