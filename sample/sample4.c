#include <stdio.h>
#include <unistd.h>

int main()
{
  pid_t pid, pid_wait;

  if ((pid = fork()) == 0){
    printf("Child: %d\n", getpid());
    return 0;
  }else if (pid < 0){
    printf("Error: %d\n", getpid());
    return -1;
  }
  pid_wait = wait(NULL);
  printf("pid_wait: %d\n", pid_wait);

  return 0;
  }
