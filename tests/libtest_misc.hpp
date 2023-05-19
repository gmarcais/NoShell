#ifndef __TEST_MISC_H__
#define __TEST_MISC_H__

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <vector>
#include <stdexcept>
#include <iostream>

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

struct not_allowed_dir {
  const char* path;
  not_allowed_dir(const char* p)
    : path(p) {
    if(mkdir(path, 0) == -1 && errno != EEXIST) throw std::runtime_error("Can't create not allowed dir");
    if(chmod(path, S_IRUSR | S_IXUSR) == -1) throw std::runtime_error("Can't set rights on dir");
  }
  ~not_allowed_dir() {
    if(rmdir(path) == -1)
      std::cerr << "Can't unlink not writable dir " << strerror(errno) << std::endl;
  }
};

#endif /* __TEST_MISC_H__ */
