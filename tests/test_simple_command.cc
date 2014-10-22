#include <gtest/gtest.h>
#include <noshell/noshell.hpp>

namespace {
namespace NS = noshell;
using namespace NS::literal;

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

TEST(Status, Bad) {
  NS::Exit e = "false"_C();

  EXPECT_FALSE(e.success());
  EXPECT_TRUE(e[0].have_status());
  EXPECT_TRUE(e[0].status().exited());
  EXPECT_NE(0, e[0].status().exit_status());
} // Status.Bad


TEST(Status, Signal) {
  NS::Exit e = "sleep"_C("10").run();
  kill(e[0].pid, SIGTERM);
  e.wait();
  EXPECT_FALSE(e.success());
  EXPECT_TRUE(e[0].have_status());
  EXPECT_FALSE(e[0].status().exited());
  EXPECT_TRUE(e[0].status().signaled());
  EXPECT_EQ(SIGTERM, e[0].status().term_sig());
} // Status.Signal
} // empty namespace
