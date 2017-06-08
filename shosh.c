#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <limits.h>
//#include <sys/ioctl.h>
//#include <asm/termbits.h>

#define LENGTH 64
const char* Exit = "exit\n";

/* char* 型の文字列をスペース区切りの char** 型に変換する */
void input_analysis(char buf[], char** argv) {
  char* cp = buf;
  cp++;  // buf 先頭 = NULL のため
  int n = 0;
  *argv = (char*)malloc(LENGTH);
  while (*cp != '\0') {
    if (*cp == 0x20) {  // space
      **argv = '\0';
      while (n > 0) {
        n--;
        (*argv)--;
      }
      argv++;
      *argv = (char*)malloc(LENGTH);
    } else {
      **argv = *cp;
      (*argv)++;
      n++;
    }
    cp++;
  }
  **argv = '\0';
  while (n > 0) {
    n--;
    (*argv)--;
  }
  argv++;
  *argv = (char*)malloc(LENGTH);
  **argv = '\0';
}


int main() {
  char buf[LENGTH];  // 0番目はNULL (BackSpace判定のため)
  char* c;
  char* argv[LENGTH];
  //char argc[LENGTH];
  char *argc;
  int fd_stdin = fileno(stdin);
  pid_t pid, pid_wait;
  int ret;

  while (1) {
    /* 初期化 */
    printf("$ ");
    fflush(stdout);
    memset(buf, '\0', LENGTH);
    memset(argv, '\0', LENGTH);
    c = buf;
    *c = '\0';

    /* Enter まで読み込み */
    do {
      c++;
      read(fd_stdin, c, 1);
      if (*c == 0x7f) {    // backspace
        printf("\b\b\b");  // 3文字戻る (消したい1文字+BS記号2文字)
        printf("   ");     // 3文字埋める
        printf("\b\b\b");  // 3文字戻る
        c--;
        if (*c == '\0') c++;
      }
      fflush(stdout);
    } while (*c != 0x0a);
    *c = '\0';

    /* exit が入力されたら return 0 */
    char* cp = buf;
    cp++;
    if (strcmp(cp, Exit) == 0) return 0;

    /* char* 型の文字列をスペース区切りの char** 型に変換する */
    //argv = (char**)malloc(LENGTH);
    input_analysis(buf, argv);

    /* char* argc = argv[0] */
    argc = (char*)malloc(LENGTH);
    //argv0_to_argc(*argv, argc);
    strcpy(argc,argv[0]);

    /* test */
    printf("argc   : %s\n", argc);
    printf("argv[0]: %s\n", argv[0]);
    printf("argv[1]: %s\n", argv[1]);
    printf("argv[2]: %s\n", argv[2]);

    /* 実行 */
    if ((pid = fork()) == 0) {
      if ((ret = execvp(argc, argv)) == 0) printf("Error\n");
      fflush(stdout);
    }
    pid_wait = wait(NULL);

    /* free */
    free(argc);
    while(**argv == '\0'){
      free((*argv)++);
    }
  }

  return 0;
}
