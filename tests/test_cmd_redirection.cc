#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <gtest/gtest.h>
#include <noshell/noshell.hpp>
#include "libtest_misc.hpp"

namespace {
namespace NS = noshell;
using namespace NS::literal;
static const char* tmpfile = "CmdRedirection_tmp";
static const char* textfile = "text_file_tmp";

class CmdRedirection : public ::testing::Test {
protected:
  virtual void SetUp() {
    std::ofstream os(tmpfile, std::ios::trunc | std::ios::out);
  }

  static const std::vector<const char*>lines;
  static void SetUpTestSuite() {
    std::ofstream os(textfile);
    for(auto str : lines)
      os << str << '\n';
  }

  static void TestOutput(const char* path) {
    std::ifstream tmp(path);
    std::string line;
    for(auto str : lines) {
      EXPECT_TRUE((bool)std::getline(tmp, line));
      EXPECT_EQ(str, line);
    }
    EXPECT_FALSE((bool)std::getline(tmp, line));
  }
};
const std::vector<const char*> CmdRedirection::lines{"hello", "the", "", "world!"};

TEST_F(CmdRedirection, OutputFD) {
  check_fixed_fds check_fds;

  int fd = open(tmpfile, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
  ASSERT_NE(-1, fd);
  NS::Exit e = NS::C("./puts_to", 1, "coucou") > fd;
  close(fd);

  EXPECT_TRUE(e.success());
  std::ifstream tmp(tmpfile);
  std::string line;
  EXPECT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("coucou", line);
  EXPECT_FALSE(std::getline(tmp, line));
}

TEST_F(CmdRedirection, OutputBadFile) {
  check_fixed_fds check_fds;

  NS::Exit e = NS::C("./puts_to", "1", "coucou") > "/cantwritethere";

  EXPECT_FALSE(e.success());
  EXPECT_TRUE(e[0].setup_error());
}

TEST_F(CmdRedirection, OutputTmpFile) {
  check_fixed_fds check_fds;

  NS::Exit e = NS::C("./puts_to", 1, "coucou") > tmpfile;

  EXPECT_TRUE(e.success());
  std::ifstream tmp(tmpfile);
  std::string line;
  EXPECT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("coucou", line);
  EXPECT_FALSE((bool)std::getline(tmp, line));
}

TEST_F(CmdRedirection, AppendTmpFile) {
  check_fixed_fds check_fds;

  {
    NS::Exit e = NS::C("./puts_to", "1", "coucou", "hi") > tmpfile;
    EXPECT_TRUE(e.success());
  }

  {
    NS::Exit e = NS::C("./puts_to", "1", "toto", "tata") >> tmpfile;
    EXPECT_TRUE(e.success());
  }

  std::ifstream tmp(tmpfile);
  std::string line;
  EXPECT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("coucou", line);
  EXPECT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("hi", line);
  EXPECT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("toto", line);
  EXPECT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("tata", line);
  EXPECT_FALSE((bool)std::getline(tmp, line));
}

TEST_F(CmdRedirection, InputFD) {
  check_fixed_fds check_fds;

  int fd = open(textfile, O_RDONLY);
  ASSERT_NE(-1, fd);
  NS::Exit e = NS::C("cat") > tmpfile < fd;
  close(fd);

  EXPECT_TRUE(e.success());
  TestOutput(tmpfile);
}

TEST_F(CmdRedirection, InputFile) {
  check_fixed_fds check_fds;

  NS::Exit e = NS::C("cat") > tmpfile < textfile;

  EXPECT_TRUE(e.success());
  TestOutput(tmpfile);
}

TEST_F(CmdRedirection, InputFileString) {
  std::string input(textfile);
  NS::Exit e = NS::C("cat") > tmpfile < input;

  EXPECT_TRUE(e.success());
  TestOutput(tmpfile);
}

TEST_F(CmdRedirection, OutputPipe) {
  check_fixed_fds check_fds;

  FILE* f = nullptr;
  NS::Exit e = NS::C("./puts_to", "1", "coucou", "hello") | f;
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
  check_fixed_fds check_fds;

  FILE* f = nullptr;
  NS::Exit e = (f | NS::C("cat")) > tmpfile;

  ASSERT_FALSE(e[0].setup_error());
  ASSERT_NE(nullptr, f);
  ASSERT_NE(-1, fileno(f));

  fprintf(f, "hello\nthe world");
  fclose(f);

  e.wait();
  EXPECT_TRUE(e.success());

  std::ifstream tmp(tmpfile);
  std::string line;
  ASSERT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("hello", line);
  ASSERT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("the world", line);
  ASSERT_FALSE((bool)std::getline(tmp, line));
}

#if defined(__GLIBCXX__) || defined(HAVE_STDIO_FILEBUF_H)
TEST_F(CmdRedirection, OutputStream) {
  check_fixed_fds check_fds;

  NS::istream is;
  NS::Exit e = NS::C("./puts_to", 1, "coucou", "hello") | is;
  ASSERT_FALSE(e[0].setup_error());
  ASSERT_TRUE(is.good());

  std::string line;
  EXPECT_TRUE((bool)std::getline(is, line));
  EXPECT_EQ("coucou", line);
  EXPECT_TRUE((bool)std::getline(is, line));
  EXPECT_EQ("hello", line);
  EXPECT_FALSE((bool)std::getline(is, line));
  is.close();

  e.wait();
  EXPECT_TRUE(e.success());
}

TEST_F(CmdRedirection, InputStream) {
  check_fixed_fds check_fds;

  NS::ostream os;
  NS::Exit e = (os | NS::C("cat")) > tmpfile;

  ASSERT_FALSE(e[0].setup_error());
  ASSERT_TRUE(os.good());

  os << "hello\nthe world";
  os.close();

  e.wait();
  EXPECT_TRUE(e.success());

  std::ifstream tmp(tmpfile);
  std::string line;
  ASSERT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("hello", line);
  ASSERT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("the world", line);
  ASSERT_FALSE((bool)std::getline(tmp, line));
}
#endif // defined(__GLIBCXX__) || defined(HAVE_STDIO_FILEBUF_H)

TEST_F(CmdRedirection, OutErr1) {
  check_fixed_fds check_fds;

  NS::Exit e = "./puts_to"_C(1, "bah") > NS::R(2, 1).to(tmpfile);

  ASSERT_TRUE(e.success());
  std::ifstream tmp(tmpfile);
  std::string line;
  ASSERT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("bah", line);
  ASSERT_FALSE((bool)std::getline(tmp, line));
} // CmdRedirection.OutErr1

TEST_F(CmdRedirection, OutErr2) {
  check_fixed_fds check_fds;

  NS::Exit e = "./puts_to"_C(2, "bou") > NS::R(2, 1).to(tmpfile);

  ASSERT_TRUE(e.success());
  std::ifstream tmp(tmpfile);
  std::string line;
  ASSERT_TRUE((bool)std::getline(tmp, line));
  EXPECT_EQ("bou", line);
  ASSERT_FALSE((bool)std::getline(tmp, line));
} // CmdRedirection.OutErr2

TEST_F(CmdRedirection, Path3) {
  check_fixed_fds check_fds;

  FILE* st;
  NS::Exit e = ("cat"_C("/dev/fd/3") < 3_R(textfile)) | st;

  EXPECT_FALSE(e[0].setup_error());

  char*  line      = nullptr;
  size_t line_size = 0;
  EXPECT_LT(0, getline(&line, &line_size, st));
  EXPECT_STREQ("hello\n", line);
  EXPECT_LT(0, getline(&line, &line_size, st));
  EXPECT_STREQ("the\n", line);
  EXPECT_LT(0, getline(&line, &line_size, st));
  EXPECT_STREQ("\n", line);
  EXPECT_LT(0, getline(&line, &line_size, st));
  EXPECT_STREQ("world!\n", line);
  errno = 0;
  EXPECT_EQ(-1, getline(&line, &line_size, st));
  EXPECT_EQ(0, errno);
  fclose(st);
  free(line);

  e.wait();
  ASSERT_TRUE(e.success());
} // CmdRedirection.Path3

TEST_F(CmdRedirection, OutPipe1) {
  check_fixed_fds check_fds;

  FILE* out;
  NS::Exit e = "./puts_to"_C(1, "youpie") | NS::R(2, 1).to(out);

  char* buf = nullptr;
  size_t n = 0;
  ASSERT_NE(-1, getline(&buf, &n, out));
  EXPECT_STREQ("youpie\n", buf);
  ASSERT_EQ(-1, getline(&buf, &n, out));
  free(buf);
  fclose(out);

  e.wait();
  ASSERT_TRUE(e.success());
} // CmdRedirection.OutPipe1

TEST_F(CmdRedirection, OutPipe2) {
  check_fixed_fds check_fds;

  int out;
  NS::Exit e = "./puts_to"_C(2, "voici") | NS::R(2, 1).to(out);

  char* buf = nullptr;
  size_t n = 0;
  FILE* fout = fdopen(out, "r");
  ASSERT_NE(-1, getline(&buf, &n, fout));
  EXPECT_STREQ("voici\n", buf);
  ASSERT_EQ(-1, getline(&buf, &n, fout));
  free(buf);
  fclose(fout);

  e.wait();
  ASSERT_TRUE(e.success());
} // CmdRedirection.OutPipe2

#if defined(__GLIBCXX__) || defined(HAVE_STDIO_FILEBUF_H)
TEST_F(CmdRedirection, OutStream1) {
  check_fixed_fds check_fds;

  NS::istream is;
  NS::Exit e = "./puts_to"_C(1, "yougadie") | NS::R(1, 2).to(is);
  ASSERT_NE(-1, is.fd());

  std::string line;
  EXPECT_TRUE((bool)std::getline(is, line));
  EXPECT_EQ("yougadie", line);
  EXPECT_FALSE((bool)std::getline(is, line));

  e.wait();
  ASSERT_TRUE(e.success());
} // CmdRedirection.OutPipe1

TEST_F(CmdRedirection, OutStream2) {
  check_fixed_fds check_fds;

  NS::istream is;
  NS::Exit e = "./puts_to"_C(2, "pouf pouf") | NS::R(1, 2).to(is);
  ASSERT_NE(-1, is.fd());

  std::string line;
  EXPECT_TRUE((bool)std::getline(is, line));
  EXPECT_EQ("pouf pouf", line);
  EXPECT_FALSE((bool)std::getline(is, line));

  e.wait();
  ASSERT_TRUE(e.success());
} // CmdRedirection.OutPipe1
#endif // defined(__GLIBCXX__) || defined(HAVE_STDIO_FILEBUF_H)


} // empty namespace
