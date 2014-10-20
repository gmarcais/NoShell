#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  kill(getpid(), SIGTERM);
  return 1;
}
