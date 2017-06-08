#include <stdio.h>
#include <stdlib.h>

int main(){
  char* env = "PATH";
  char* c = getenv(env);
  printf("%s\n", c);
}
