#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <cstdlib>
#include <gtest/gtest.h>
#include <noshell/noshell.hpp>

namespace {
template<typename T>
noshell::Status fork_run(T func) {
  pid_t pid = fork();
  EXPECT_NE((pid_t)-1, pid);
  if(pid) {
    noshell::Status st;
    pid_t res = waitpid(pid, &st.value, 0);
    EXPECT_NE((pid_t)-1, res);
    return st;
  } else { // Child
    func();
    exit(255);
  }
}

TEST(Status, Exit) {
  static const int ret = 4;
  noshell::Status st = fork_run([]{ exit(ret); });
  EXPECT_TRUE(st.exited());
  EXPECT_EQ(ret, st.exit_status());
} // Status.Exit

TEST(Status, Signal) {
  auto status = fork_run([]{ kill(getpid(), SIGKILL); });
  EXPECT_TRUE(status.signaled());
  EXPECT_EQ(SIGKILL, status.term_sig());
} // Status.Signal

} // empty namespace
