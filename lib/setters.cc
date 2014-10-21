#include <unistd.h>
#include <fcntl.h>
#include <iterator>

#include <noshell/setters.hpp>

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

bool safe_dup2(int& to, int from) {
  if(to == -1) return true;
  int res;
  while((res = dup2(to, from) == -1)) {
    if(errno == EINTR || errno == EBUSY) continue;
    return false;
  }
  safe_close(to);
  return true;
}

bool fd_redirection::child_setup() {
  return safe_dup2(to, from);
}

pipeline_redirection::~pipeline_redirection() {
  safe_close(pipe0[0]);
  safe_close(pipe0[1]);
  safe_close(pipe1[0]);
  safe_close(pipe1[1]);
}

bool pipeline_redirection::child_setup() {
  safe_close(pipe0[1]);
  safe_close(pipe1[0]);
  return safe_dup2(pipe0[0], 0) && safe_dup2(pipe1[1], 1);
}

bool pipeline_redirection::parent_setup() {
  safe_close(pipe0[0]);
  safe_close(pipe1[1]);
  return true;
}
} // namespace noshell
