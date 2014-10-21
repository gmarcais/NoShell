#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>
#include <noshell/noshell.hpp>

namespace {
namespace NS = noshell;
TEST(CmdRedirection, OutputFD) {
  int fd = open(getenv("TEST_TMP"), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
  ASSERT_NE(-1, fd);
  NS::Handle handle = NS::Command({"./puts_to", "1", "coucou"}) > fd;
  close(fd);

  EXPECT_TRUE(handle.success());
  std::ifstream tmp(getenv("TEST_TMP"));
  std::string line;
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("coucou", line);
  EXPECT_FALSE(std::getline(tmp, line));
}
} // empty namespace
