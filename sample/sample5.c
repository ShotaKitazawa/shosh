#include <stdio.h>
#include <unistd.h>

int main(){
  int ret;

  char *cmd = "/bin/ls";
  char *arg[3] = {"/bin/ls", ".", NULL};
  if ((ret = execvp (cmd, arg)) == 0)
    printf("Error\n");

  return 0;
}
