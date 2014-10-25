#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <gtest/gtest.h>
#include <noshell/noshell.hpp>

namespace {
namespace NS = noshell;
using namespace NS::literal;

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
  NS::Exit e = NS::C("./puts_to", 1, "coucou") > fd;
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
  NS::Exit e = NS::C("./puts_to", 1, "coucou") > getenv("TEST_TMP");

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
  FILE* f = nullptr;
  NS::Exit e = f | NS::C("cat") > getenv("TEST_TMP");

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
  NS::Exit e = NS::C("./puts_to", 1, "coucou", "hello") | is;
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
  NS::Exit e = os | NS::C("cat") > getenv("TEST_TMP");

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

TEST_F(CmdRedirection, OutErr1) {
  NS::Exit e = "./puts_to"_C(1, "bah") > NS::R(2, 1).to(getenv("TEST_TMP"));

  ASSERT_TRUE(e.success());
  std::ifstream tmp(getenv("TEST_TMP"));
  std::string line;
  ASSERT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("bah", line);
  ASSERT_FALSE(std::getline(tmp, line));
} // CmdRedirection.OutErr1

TEST_F(CmdRedirection, OutErr2) {
  NS::Exit e = "./puts_to"_C(2, "bou") > NS::R(2, 1).to(getenv("TEST_TMP"));

  ASSERT_TRUE(e.success());
  std::ifstream tmp(getenv("TEST_TMP"));
  std::string line;
  ASSERT_TRUE(std::getline(tmp, line));
  EXPECT_EQ("bou", line);
  ASSERT_FALSE(std::getline(tmp, line));
} // CmdRedirection.OutErr2

TEST_F(CmdRedirection, Path3) {
  NS::istream is;
  NS::Exit e = "cat"_C("/dev/fd/3") < 3_R("text_file.txt") | is;

  std::string line;
  EXPECT_TRUE(std::getline(is, line));
  EXPECT_EQ("hello", line);
  EXPECT_TRUE(std::getline(is, line));
  EXPECT_EQ("the", line);
  EXPECT_TRUE(std::getline(is, line));
  EXPECT_EQ("", line);
  EXPECT_TRUE(std::getline(is, line));
  EXPECT_EQ("world!", line);
  EXPECT_FALSE(std::getline(is, line));

  e.wait();
  ASSERT_TRUE(e.success());
} // CmdRedirection.Path3

TEST_F(CmdRedirection, OutPipe1) {
  FILE* out;
  NS::Exit e = "./puts_to"_C(1, "youpie") | NS::R(2, 1).to(out);

  char* buf = nullptr;
  size_t n = 0;
  ASSERT_NE(-1, getline(&buf, &n, out));
  EXPECT_STREQ("youpie\n", buf);
  ASSERT_EQ(-1, getline(&buf, &n, out));
  free(buf);

  e.wait();
  ASSERT_TRUE(e.success());
} // CmdRedirection.OutPipe1

TEST_F(CmdRedirection, OutPipe2) {
  int out;
  NS::Exit e = "./puts_to"_C(2, "voici") | NS::R(2, 1).to(out);

  char* buf = nullptr;
  size_t n = 0;
  FILE* fout = fdopen(out, "r");
  ASSERT_NE(-1, getline(&buf, &n, fout));
  EXPECT_STREQ("voici\n", buf);
  ASSERT_EQ(-1, getline(&buf, &n, fout));
  free(buf);

  e.wait();
  ASSERT_TRUE(e.success());
} // CmdRedirection.OutPipe2

TEST_F(CmdRedirection, OutStream1) {
  NS::istream is;
  NS::Exit e = "./puts_to"_C(1, "yougadie") | NS::R(1, 2).to(is);

  std::string line;
  EXPECT_TRUE(std::getline(is, line));
  EXPECT_EQ("yougadie", line);
  EXPECT_FALSE(std::getline(is, line));

  e.wait();
  ASSERT_TRUE(e.success());
} // CmdRedirection.OutPipe1

TEST_F(CmdRedirection, OutStream2) {
  NS::istream is;
  NS::Exit e = "./puts_to"_C(2, "pouf pouf") | NS::R(1, 2).to(is);

  std::string line;
  EXPECT_TRUE(std::getline(is, line));
  EXPECT_EQ("pouf pouf", line);
  EXPECT_FALSE(std::getline(is, line));

  e.wait();
  ASSERT_TRUE(e.success());
} // CmdRedirection.OutPipe1


} // empty namespace
