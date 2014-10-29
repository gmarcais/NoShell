#include <gtest/gtest.h>
#include <noshell/noshell.hpp>
#include "test_misc.hpp"


namespace {
namespace NS = noshell;
using namespace NS::literal;

TEST(Error, ReadFile) {
  check_fixed_fds check_fds;

  NS::Exit e = "cat"_C() < "doesntexists";

  EXPECT_TRUE(e[0].setup_error());
  const std::string expect = "Failed to open the file 'doesntexists' for reading";
  EXPECT_EQ(expect, e[0].message);
  EXPECT_EQ(ENOENT, e[0].data.err.value);
} // Error.ReadFile

TEST(Error, WriteFile) {
  check_fixed_fds check_fds;

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
  check_fixed_fds check_fds;

  NS::Exit e = "stupidcmd"_C();
  ASSERT_TRUE(e[0].setup_error());
  EXPECT_EQ("Child process setup error", e[0].message);
  EXPECT_EQ(ENOENT, e[0].data.err.value);
} // Error.BadCmd

TEST(Error, Failures) {
  check_fixed_fds check_fds;

  NS::Exit e = "notexists"_C() | "cat"_C("--badoption") | "cat"_C() > "/noallowed";
  EXPECT_FALSE(e.success());

  ssize_t i = 0;
  for(auto it = e.failures().begin(); it != e.failures().end(); ++it) {
    EXPECT_EQ(i, it.id());
    if(i == 0) {
      EXPECT_TRUE(it->setup_error());
      EXPECT_EQ(ENOENT, it->err().value);
    } else if(i == 1) {
      EXPECT_FALSE(it->setup_error());
      EXPECT_TRUE(it->have_status());
      EXPECT_NE(0, it->status().exit_status());
    } else if(i == 2) {
      EXPECT_TRUE(it->setup_error());
      EXPECT_EQ(EACCES, it->err().value);
    }
    ++i;
  }
} // Error.Failures


} // empty namespace
