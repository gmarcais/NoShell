#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <cerrno>
#include <cctype>

#include <vector>
#include <system_error>
#include <stdexcept>
#include <algorithm>
#include <string>

#include <gtest/gtest.h>
#include "libtest_misc.hpp"

std::vector<int> open_fds() {
  static const int flags = O_RDONLY
#ifdef O_DIRECTORY
    |O_DIRECTORY
#endif
    ;

  std::vector<int> ret;

  int self_fd = open("/proc/self/fd", flags);
  if(self_fd == -1)
    throw std::system_error(errno, std::system_category(), "open");
  DIR* fds = fdopendir(self_fd);
  if(!fds) {
    close(self_fd);
    throw std::system_error(errno, std::system_category(), "fdopendir");
  }
  auto_close close_fds(fds);

  dirent* entry;
  for(errno = 0 ; (entry = readdir(fds)); errno = 0) {
    if(entry->d_name[0] == '.') continue;
    std::string s(entry->d_name);
    if(!std::all_of(s.cbegin(), s.cend(), [](const char c) { return isdigit(c); }))
      throw std::runtime_error("Invalid entry in fd directory" + s);
    int fd = atoi(entry->d_name);
    if(fd != self_fd)
      ret.push_back(fd);
  }
  if(errno) throw std::system_error(errno, std::system_category(), "readdir");

  return ret;
}
