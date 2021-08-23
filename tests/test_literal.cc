#include <fstream>

#include <gtest/gtest.h>
#include <noshell/noshell.hpp>

namespace {
namespace NS = noshell;
using namespace NS::literal; // import ""_C syntax
static const char* tmpfile = "Literal_tmp";

TEST(Literal, Command) {
  NS::Exit e = ("./puts_to"_C("1", "yes", "indeed") | "wc"_C("-l")) > tmpfile;
  ASSERT_TRUE(e.success());

  std::ifstream is(tmpfile);
  std::string line;
  ASSERT_TRUE((bool)std::getline(is, line));
  EXPECT_EQ("2", line);
  ASSERT_FALSE((bool)std::getline(is, line));
}
} // empty namespace
