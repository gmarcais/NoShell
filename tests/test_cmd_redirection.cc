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
  NS::Exit e = NS::C("./puts_to", "1", "coucou") > fd;
  close(fd);

  EXPECT_TRUE(e.success());
  std::ifstream tmp(getenv("TEST_TMP"));
  std::string line;
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("coucou", line);
  EXPECT_FALSE(std::getline(tmp, line));
}

TEST(CmdRedirection, OutputBadFile) {
  NS::Exit e = NS::C("./puts_to", "1", "coucou") > "/cantwritethere";

  EXPECT_FALSE(e.success());
  EXPECT_TRUE(e[0].setup_error());
}

TEST(CmdRedirection, OutputTmpFile) {
  NS::Exit e = NS::C("./puts_to", "1", "coucou") > getenv("TEST_TMP");

  EXPECT_TRUE(e.success());
  std::ifstream tmp(getenv("TEST_TMP"));
  std::string line;
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("coucou", line);
  EXPECT_FALSE(std::getline(tmp, line));
}

TEST(CmdRedirection, AppendTmpFile) {
  {
    NS::Exit e = NS::C("./puts_to", "1", "coucou", "hi") > getenv("TEST_TMP");
    EXPECT_TRUE(e.success());
  }

  {
    NS::Exit e = NS::C("./puts_to", "1", "toto", "tata") >> getenv("TEST_TMP");
    EXPECT_TRUE(e.success());
  }

  std::ifstream tmp(getenv("TEST_TMP"));
  std::string line;
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("coucou", line);
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("hi", line);
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("toto", line);
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("tata", line);
  EXPECT_FALSE(std::getline(tmp, line));
}

TEST(CmdRedirection, InputFile) {
  NS::Exit e = NS::C("cat") > getenv("TEST_TMP") < "text_file.txt";

  EXPECT_TRUE(e.success());
  std::ifstream tmp(getenv("TEST_TMP"));
  std::string line;
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("hello", line);
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("the", line);
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("", line);
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("world!", line);
  EXPECT_FALSE(std::getline(tmp, line));
}
} // empty namespace
