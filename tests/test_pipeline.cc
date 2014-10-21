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
  NS::Exit e = NS::C("./puts_to", "1", "line1", "line2") | NS::C("wc", "-l");
  EXPECT_TRUE(e.success());

  EXPECT_FALSE(e[0].setup_error());
  EXPECT_TRUE(e[0].have_status());
  EXPECT_EQ(0, e[0].status().exit_status());

  EXPECT_FALSE(e[1].setup_error());
  EXPECT_TRUE(e[1].have_status());
  EXPECT_EQ(0, e[1].status().exit_status());
} // PipeLine.TwoCommands

} // empty namespace
