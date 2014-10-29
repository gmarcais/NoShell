#include <unistd.h>
#include <fcntl.h>
#include <cerrno>


namespace noshell {
bool safe_close(int& fd) {
  if(fd == -1) return true;
  while(close(fd) == -1) {
    if(errno == EINTR) continue;
    return false;
  }
  fd = -1;
  return true;
}

bool safe_dup(int fd, int& newfd, bool set_cloexec, int above) {
#ifdef F_DUPFD_CLOEXEC
  const int cmd = set_cloexec ? F_DUPFD_CLOEXEC : F_DUPFD;
#else
  const int cmd = F_DUPFD;
#endif
  while(true) {
    if((newfd = fcntl(fd, cmd, above)) != -1) break;
    if(errno != EINTR) return false;
  }

#ifndef F_DUPFD_CLOEXEC
  if(set_cloexec) {
    int flags = fcntl(newfd, F_GETFD);
    if(flags == -1) return false;
    flags |= FD_CLOEXEC;
    fcntl(newfd, F_SETFD, flags);
  }
#endif
  return true;
}

bool safe_dup2_no_close(int to, int from) {
  if(to == -1) return true;
  int res;
  while((res = dup2(to, from) == -1)) {
    if(errno == EINTR || errno == EBUSY) continue;
    return false;
  }
  return true;
}

bool safe_dup2(int& to, int from) {
  if(safe_dup2_no_close(to, from))
    return safe_close(to);
  return false;
}

} // namespace noshell
