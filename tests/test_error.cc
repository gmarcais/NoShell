#include <gtest/gtest.h>
#include <noshell/noshell.hpp>


namespace {
namespace NS = noshell;
using namespace NS::literal;

TEST(Error, ReadFile) {
  NS::Exit e = "cat"_C() < "doesntexists";

  EXPECT_TRUE(e[0].setup_error());
  const std::string expect = "Failed to open the file 'doesntexists' for reading";
  EXPECT_EQ(expect, e[0].message);
  EXPECT_EQ(ENOENT, e[0].data.err.value);
} // Error.ReadFile

TEST(Error, WriteFile) {
  {
    NS::Exit e = "cat"_C() > "/noallowed";
    ASSERT_TRUE(e[0].setup_error());
    const std::string expect = "Failed to open the file '/noallowed' for writing";
    EXPECT_EQ(expect, e[0].message);
    EXPECT_EQ(EACCES, e[0].data.err.value);
  }

  {
    NS::Exit e = "cat"_C() > "/doesntexists/stupid";
    ASSERT_TRUE(e[0].setup_error());
    const std::string expect = "Failed to open the file '/doesntexists/stupid' for writing";
    EXPECT_EQ(expect, e[0].message);
    EXPECT_EQ(ENOENT, e[0].data.err.value);
  }
} // Error.ReadFile

TEST(Error, BadCmd) {
  NS::Exit e = "stupidcmd"_C();
  ASSERT_TRUE(e[0].setup_error());
  EXPECT_EQ("Child process setup error", e[0].message);
  EXPECT_EQ(ENOENT, e[0].data.err.value);
} // Error.BadCmd


} // empty namespace
