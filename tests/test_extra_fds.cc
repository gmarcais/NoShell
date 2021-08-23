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
#include "libtest_misc.hpp"

namespace {
namespace NS = noshell;
static const char *tmpfile = "ExtraFds_tmp";
class ExtraFds : public ::testing::Test {
protected:
  virtual void SetUp() {
    std::ofstream os(tmpfile, std::ios::trunc | std::ios::out);
  }
};

// Check that no extra file descriptor is passed to the child and that
// none are left in the parent.
TEST_F(ExtraFds, Redirection) {
  const auto fds = open_fds();

  std::vector<std::string> cmd;
  cmd.push_back("./check_open_fd");
  for(auto fd : fds) cmd.push_back(std::to_string(fd));

  NS::Exit e = (NS::C(cmd) | NS::C("cat")) < "/dev/null" > tmpfile;
  ASSERT_TRUE(e.success());

  auto new_fds = open_fds();
  EXPECT_EQ(fds.size(), new_fds.size());
  EXPECT_TRUE(std::equal(fds.cbegin(), fds.cend(), new_fds.cbegin()));
}
} // empty namespace
