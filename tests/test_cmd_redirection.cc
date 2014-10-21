#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
// #include <ext/stdio_filebuf.h>
#include <gtest/gtest.h>
#include <noshell/noshell.hpp>

namespace {
namespace NS = noshell;

class CmdRedirection : public ::testing::Test {
protected:
  virtual void SetUp() {
    while(true) {
      if(truncate(getenv("TEST_TMP"), 0) != -1) break;
      if(errno == EINTR) continue;
      throw std::runtime_error("Failed to truncate tmp file");
    }
  }
};

TEST_F(CmdRedirection, OutputFD) {
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

TEST_F(CmdRedirection, OutputBadFile) {
  NS::Exit e = NS::C("./puts_to", "1", "coucou") > "/cantwritethere";

  EXPECT_FALSE(e.success());
  EXPECT_TRUE(e[0].setup_error());
}

TEST_F(CmdRedirection, OutputTmpFile) {
  NS::Exit e = NS::C("./puts_to", "1", "coucou") > getenv("TEST_TMP");

  EXPECT_TRUE(e.success());
  std::ifstream tmp(getenv("TEST_TMP"));
  std::string line;
  EXPECT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("coucou", line);
  EXPECT_FALSE(std::getline(tmp, line));
}

TEST_F(CmdRedirection, AppendTmpFile) {
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

TEST_F(CmdRedirection, InputFD) {
  int fd = open("text_file.txt", O_RDONLY);
  ASSERT_NE(-1, fd);
  NS::Exit e = NS::C("cat") > getenv("TEST_TMP") < fd;
  close(fd);

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

TEST_F(CmdRedirection, InputFile) {
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

TEST_F(CmdRedirection, OutputPipe) {
  FILE* f = nullptr;
  auto cmd = NS::C("./puts_to", "1", "coucou", "hello") | f;
  auto e = cmd.run();
  ASSERT_FALSE(e[0].setup_error());
  ASSERT_NE(nullptr, f);
  ASSERT_NE(-1, fileno(f));

  char buf[1024];
  EXPECT_TRUE(fgets(buf, sizeof(buf) - 1, f) != nullptr);
  EXPECT_STREQ("coucou\n", buf);
  EXPECT_TRUE(fgets(buf, sizeof(buf) - 1, f) != nullptr);
  EXPECT_STREQ("hello\n", buf);
  EXPECT_TRUE(fgets(buf, sizeof(buf) - 1, f) == nullptr);
  fclose(f);

  e.wait();
  EXPECT_TRUE(e.success());
}

TEST_F(CmdRedirection, InputPipe) {
  FILE* f = nullptr;
  auto cmd = f | NS::C("cat") > getenv("TEST_TMP");
  auto e = cmd.run();

  ASSERT_FALSE(e[0].setup_error());
  ASSERT_NE(nullptr, f);
  ASSERT_NE(-1, fileno(f));

  fprintf(f, "hello\nthe world");
  fclose(f);

  e.wait();
  EXPECT_TRUE(e.success());

  std::ifstream tmp(getenv("TEST_TMP"));
  std::string line;
  ASSERT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("hello", line);
  ASSERT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("the world", line);
  ASSERT_FALSE(std::getline(tmp, line));
}

TEST_F(CmdRedirection, OutputStream) {
  NS::istream is;
  auto cmd = NS::C("./puts_to", "1", "coucou", "hello") | is;
  auto e = cmd.run();
  ASSERT_FALSE(e[0].setup_error());
  ASSERT_TRUE(is.good());

  std::string line;
  EXPECT_TRUE(std::getline(is, line));
  EXPECT_EQ("coucou", line);
  EXPECT_TRUE(std::getline(is, line));
  EXPECT_EQ("hello", line);
  EXPECT_FALSE(std::getline(is, line));
  is.close();

  e.wait();
  EXPECT_TRUE(e.success());
}

TEST_F(CmdRedirection, InputStream) {
  NS::ostream os;
  auto cmd = os | NS::C("cat") > getenv("TEST_TMP");
  auto e = cmd.run();

  ASSERT_FALSE(e[0].setup_error());
  ASSERT_TRUE(os.good());

  os << "hello\nthe world";
  os.close();

  e.wait();
  EXPECT_TRUE(e.success());

  std::ifstream tmp(getenv("TEST_TMP"));
  std::string line;
  ASSERT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("hello", line);
  ASSERT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("the world", line);
  ASSERT_FALSE(std::getline(tmp, line));
}
} // empty namespace
