#include <stdio.h>

#include <gtest/gtest.h>
#include <noshell/noshell.hpp>

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

} // empty namespace
