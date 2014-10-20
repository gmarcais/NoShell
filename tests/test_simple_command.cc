#include <gtest/gtest.h>
#include <noshell/noshell.hpp>

namespace {
TEST(SimpleCommand, BadCommand) {
  auto handle = noshell::Command({"/doesnt_exists"}).run();
  EXPECT_TRUE(handle.setup_error());
  EXPECT_FALSE(handle.have_status());
}

TEST(SimpleCommand, FalseCommand) {
  auto handle = noshell::Command({"false"}).run_wait();
  EXPECT_FALSE(handle.setup_error());
  EXPECT_TRUE(handle.have_status());
  EXPECT_TRUE(handle.status().exited());
  EXPECT_NE(0, handle.status().exit_status());
} // SimpleCommand.FalseCommand

TEST(SimpleCommand, TrueCommand) {
  auto handle = noshell::Command({"true"}).run_wait();
  EXPECT_FALSE(handle.setup_error());
  EXPECT_TRUE(handle.have_status());
  EXPECT_TRUE(handle.status().exited());
  EXPECT_EQ(0, handle.status().exit_status());
} // SimpleCommand.TrueCommand

TEST(SimpleCommand, KillSelf) {
  auto handle = noshell::Command({"./kill_self"}).run_wait();
  EXPECT_FALSE(handle.setup_error());
  EXPECT_TRUE(handle.have_status());
  EXPECT_FALSE(handle.status().exited());
  EXPECT_TRUE(handle.status().signaled());
  EXPECT_EQ(SIGTERM, handle.status().term_sig());
} // SimpleCommand.KillSelf
} // empty namespace
