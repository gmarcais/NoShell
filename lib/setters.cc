#include <unistd.h>
#include <fcntl.h>
#include <iterator>
#include <noshell/noshell.hpp>

namespace noshell {
bool fd_redirection::child_setup() {
  int res;
  while((res = dup2(to, from) == -1)) {
    if(errno == EINTR || errno == EBUSY) continue;
    return false;
  }
  close(to);
  return true;
}

Command& operator>(Command& cmd, from_to ft) {
  cmd.push_setter(new fd_redirection_setter(ft.from, ft.to));
  return cmd;
}

} // namespace noshell
