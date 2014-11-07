#include <fstream>

#include <gtest/gtest.h>
#include <noshell/noshell.hpp>

namespace {
namespace NS = noshell;
using namespace NS::literal; // import ""_C syntax

TEST(Literal, Command) {
  NS::Exit e = ("./puts_to"_C("1", "yes", "indeed") | "wc"_C("-l")) > getenv("TEST_TMP");
  ASSERT_TRUE(e.success());

  std::ifstream is(getenv("TEST_TMP"));
  std::string line;
  ASSERT_TRUE((bool)std::getline(is, line));
  EXPECT_EQ("2", line);
  ASSERT_FALSE((bool)std::getline(is, line));
}
} // empty namespace
