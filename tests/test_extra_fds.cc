#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#include <cerrno>
#include <cctype>
#include <system_error>
#include <stdexcept>
#include <algorithm>
#include <string>

#include <gtest/gtest.h>
#include <noshell/noshell.hpp>

namespace {
namespace NS = noshell;
struct auto_close {
  DIR* dirp;
  auto_close(DIR* p) : dirp(p) { }
  ~auto_close() { closedir(dirp); }
};

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

class ExtraFds : public ::testing::Test {
protected:
  virtual void SetUp() {
    while(true) {
      if(truncate(getenv("TEST_TMP"), 0) != -1) break;
      if(errno == EINTR) continue;
      throw std::runtime_error("Failed to truncate tmp file");
    }
  }
};

// Check that no extra file descriptor is passed to the child and that
// none are left in the parent.
TEST_F(ExtraFds, Redirection) {
  auto fds = open_fds();

  std::vector<std::string> cmd;
  cmd.push_back("./check_open_fd");
  for(auto fd : fds) cmd.push_back(std::to_string(fd));

  NS::Exit e = NS::C(cmd) | NS::C("cat") < "/dev/null" > getenv("TEST_TMP");
  ASSERT_TRUE(e.success());

  auto new_fds = open_fds();
  EXPECT_EQ(fds.size(), new_fds.size());
  EXPECT_TRUE(std::equal(fds.cbegin(), fds.cend(), new_fds.cbegin()));
}
} // empty namespace
