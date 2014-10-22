#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>

#include <cerrno>
#include <string>
#include <iostream>
#include <set>
#include <system_error>

static const int flags = O_RDONLY
#ifdef O_DIRECTORY
    |O_DIRECTORY
#endif
    ;

int main(int argc, char *argv[]) {
  std::set<int> existing_fds;
  for(int i = 1; i < argc; ++i)
    existing_fds.insert(atoi(argv[i]));

  int self_fd = open("/proc/self/fd", flags);
  if(self_fd == -1)
    throw std::system_error(errno, std::system_category(), "open");
  existing_fds.insert(self_fd);
  DIR* fds = fdopendir(self_fd);
  if(!fds)
    throw std::system_error(errno, std::system_category(), "fdopendir");

  errno          = 0;
  bool extra_fds = false;
  while(true) {
    dirent* entry = readdir(fds);
    if(!entry) {
      if(errno) {
        perror("readdir");
        return 1;
      }
      break;
    }
    if(entry->d_name[0] == '.') continue;
    int fd = atoi(entry->d_name);
    if(existing_fds.find(fd) == existing_fds.end()) {
      std::cerr << fd << "\n";
      extra_fds = true;
    }
  }

  return extra_fds ? 1 : 0;
}
