#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <type_traits>

#include <gtest/gtest.h>
#include <noshell/setters.hpp>


namespace {
namespace NS = noshell;
TEST(FdType, BasicProperties) {
  EXPECT_TRUE(std::is_nothrow_move_constructible<NS::fd_type>::value);

  NS::fd_type fd = 5;
  NS::fd_type fdout = stdout;

  EXPECT_EQ(5, fd);
  EXPECT_EQ(1, fdout);

  fdout = 3;
  EXPECT_EQ(3, fdout);
}

TEST(FdType, move) {
  {
    std::ofstream os(getenv("TEST_TMP"));
    os << "42";
  }

  NS::fd_type fd1 = open(getenv("TEST_TMP"), O_RDONLY);
  NS::fd_type fd2 = open(getenv("TEST_TMP"), O_RDONLY|O_CLOEXEC);
  ASSERT_NE(-1, fd1);
  ASSERT_NE(-1, fd2);

  const int oflags1 = fcntl(fd1, F_GETFD);
  const int oflags2 = fcntl(fd2, F_GETFD);
  EXPECT_NE(-1, oflags1);
  EXPECT_NE(-1, oflags2);
  EXPECT_EQ(0, oflags1 & FD_CLOEXEC);
  EXPECT_NE(0, oflags2 & FD_CLOEXEC);

  const int save1 = fd1;
  const int save2 = fd2;

  EXPECT_TRUE(fd2.move());
  const int flags2 = fcntl(fd2, F_GETFD);
  ASSERT_NE(-1, flags2);
  EXPECT_NE(0, flags2 & FD_CLOEXEC);
  EXPECT_NE(save2, fd2);
  EXPECT_EQ(-1, close(save2));

  EXPECT_TRUE(fd1.move());
  const int flags1 = fcntl(fd1, F_GETFD);
  ASSERT_NE(-1, flags1);
  EXPECT_EQ(0, flags1 & FD_CLOEXEC);
  EXPECT_NE(save1, fd1);
  EXPECT_EQ(-1, close(save1));
}
} // empty namespace
