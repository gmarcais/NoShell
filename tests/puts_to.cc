#include <cstdlib>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  if(argc < 2) return 1;
  int fd = std::atoi(argv[1]);
  FILE* f = fdopen(fd, "w");
  if(!f) {
    perror("fdopen");
    return 1;
  }

  for(int i = 2; i < argc; ++i)
    fprintf(f, "%s\n", argv[i]);

  return 0;
}
