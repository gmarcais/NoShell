#ifndef __TEST_MISC_H__
#define __TEST_MISC_H__

#include <dirent.h>

#include <vector>

std::vector<int> open_fds();

struct auto_close {
  DIR* dirp;
  auto_close(DIR* p) : dirp(p) { }
  ~auto_close() { closedir(dirp); }
};

struct check_fixed_fds {
  const std::vector<int> opened_before;
  check_fixed_fds() : opened_before(open_fds()) { }
  void run_check() {
    const auto opened_after = open_fds();
    ASSERT_EQ(opened_before.size(), opened_after.size()) << "Number of file descriptors changed";
    EXPECT_TRUE(std::equal(opened_before.cbegin(), opened_before.cend(), opened_after.cbegin()))
      << "Set of opened file descriptors changed";
  }
  ~check_fixed_fds() { run_check(); }
};

#endif /* __TEST_MISC_H__ */
