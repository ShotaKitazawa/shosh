#include <stdio.h>
#include <unistd.h>

int main()
{
  pid_t pid;
  if ((pid = fork()) == 0)
    printf("Child: %d\n", getpid());
  else if (0 < pid)
    printf("Parent: %d\n", getpid());
  else
    printf("Error\n");

  return 0;
  }
