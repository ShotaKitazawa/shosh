#include <stdio.h>
#include <unistd.h>

int main() {
  int ret;

  char *cmd = "";
  char *arg[3] = {"", ".", NULL};
  printf("%s\n", arg[0]);
  if ((ret = execvp(cmd, arg)) == 0) printf("Error\n");

  return 0;
}
