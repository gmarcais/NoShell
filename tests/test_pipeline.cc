#include <stdio.h>

#include <gtest/gtest.h>
#include <noshell/noshell.hpp>
#include <sys/resource.h>

namespace {
namespace NS = noshell;
TEST(PipeLine, OneCommand) {
  {
    NS::Exit e = NS::C("true");
    EXPECT_FALSE(e[0].setup_error());
    EXPECT_TRUE(e[0].have_status());
    EXPECT_EQ(0, e[0].status().exit_status());
    EXPECT_TRUE(e.success());
  }

  {
    NS::Exit e = NS::C("false");
    EXPECT_FALSE(e[0].setup_error());
    EXPECT_TRUE(e[0].have_status());
    EXPECT_NE(0, e[0].status().exit_status());
    EXPECT_FALSE(e.success());
  }
} // PipeLine.OneCommand

TEST(PipeLine, TwoCommands) {
  FILE* st;
  auto cmd = NS::C("./puts_to", "1", "line1", "line2") | NS::C("wc", "-l") | st;
  auto e = cmd.run();

  char*  line      = nullptr;
  size_t line_size = 0;

  EXPECT_LT(0, getline(&line, &line_size, st));
  EXPECT_STREQ("2\n", line);
  errno = 0;
  EXPECT_EQ(-1, getline(&line, &line_size, st));
  EXPECT_EQ(0, errno);
  fclose(st);
  free(line);
  e.wait();
  EXPECT_TRUE(e.success());

  EXPECT_FALSE(e[0].setup_error());
  EXPECT_TRUE(e[0].have_status());
  EXPECT_EQ(0, e[0].status().exit_status());

  EXPECT_FALSE(e[1].setup_error());
  EXPECT_TRUE(e[1].have_status());
  EXPECT_EQ(0, e[1].status().exit_status());
} // PipeLine.TwoCommands

TEST(PipeLine, Setup) {
  int normal, niced;
  { NS::istream is;
    NS::Exit e = NS::C("nice") | is;
    is >> normal;
  }
  { NS::istream is;
    NS::Exit e = NS::C("nice")([]() -> bool { errno = 0; if(nice(1) == -1 && errno != 0) { return false; }; return true; }) | is;
    is >> niced;
  }
  EXPECT_EQ(normal, normal == 19 ? normal : niced - 1);
}

TEST(PipeLine, FailedSetup) {
  NS::Exit e = NS::C("date")([]() -> bool { return false; }) | NS::C("head", "-n", 1);
  EXPECT_FALSE(e.success());
  EXPECT_TRUE(e[0].setup_error());
  EXPECT_FALSE(e[0].have_status());
}

TEST(PipeLine, SuccessSetup) {
  NS::Exit e = NS::C("date")([]() -> bool { return true; }) > "/dev/null";
  EXPECT_TRUE(e.success());
  EXPECT_FALSE(e[0].setup_error());
  EXPECT_TRUE(e[0].have_status());
}

TEST(PipeLine, LongPipeline) {
  constexpr int nbmax = 100;
  const char* output = "long_pipeline_tmp";

  for(int len = 1; len <= 10; ++len) {
      noshell::ostream os;
      auto pipeline = os | NS::C("cat");
      for(int j = len - 1; j < len; ++j)
        pipeline | NS::C("cat");
      pipeline > output;

      NS::Exit e = pipeline.run();
      for(int i = 0; i < nbmax; ++i)
        os << i << '\n';
      os.close();
      e.wait();

      EXPECT_TRUE(e.success()) << "len: " << len;
      std::ifstream is(output);
      int x;
      for(int i = 0; i < nbmax; ++i) {
        is >> x;
        EXPECT_TRUE(is.good());
        EXPECT_EQ(i, x);
      }
      is >> x;
      EXPECT_FALSE(is.good());
  }
  // NS::Exit e = (os | NS::C("cat") | NS::C("cat") | NS::C("cat") | NS::C("cat")) > "long_pipeline_tmp";


  // e.wait();
  // EXPECT_TRUE(e.success());
}
} // empty namespace
