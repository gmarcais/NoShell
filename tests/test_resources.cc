#include <gtest/gtest.h>
#include <noshell/noshell.hpp>

namespace {
namespace NS = noshell;
using namespace NS::literal;
const std::chrono::microseconds zero_ms = std::chrono::microseconds::zero();

TEST(Resources, Test) {
  NS::Exit e = "date"_C() > "/dev/null";
  ASSERT_TRUE(e.success());
  EXPECT_LE(zero_ms, e[0].user_time());
  EXPECT_LE(zero_ms, e[0].system_time());
  EXPECT_LT(0, e[0].maximum_rss());
  EXPECT_LT(0, e[0].minor_faults() + e[0].major_faults());
}
} // empty namespace
