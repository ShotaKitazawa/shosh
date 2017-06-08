#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LENGTH 64
const char* EXIT = "exit\n";
const char* CD = "cd";

/* char* 型の文字列をスペース区切りの char** 型に変換する, {"|'} で囲った場合区切らない */
void input_analysis(char buf[], char* argv[]) {
  char* cp = buf;
  cp++;  // buf 先頭 = NULL のため
  int n = 0;
  int flag = 0;
  *argv = (char*)malloc(LENGTH);
  while (*cp != '\0') {
    if (*cp == 0x20 && flag == 0) {  // space
      **argv = '\0';
      while (n > 0) {
        n--;
        (*argv)--;
      }
      argv++;
      *argv = (char*)malloc(LENGTH);
    }
    else if (*cp == 0x22){ // "
      flag ^= 1;
    }
    else {
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
}

int main() {
  char buf[LENGTH];  // 0番目はNULL (BackSpace判定のため)
  char* c;
  char* argv[LENGTH];
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
      printf("%x\n", *c); // デバッグ用
      fflush(stdout);
    } while (*c != 0x0a);
    *c = '\0';

    /* exit が入力されたら return 0 */
    char* cp = buf;
    cp++;
    if (strcmp(cp, EXIT) == 0) return 0;

    /* char* 型の文字列をスペース区切りの char** 型に変換する */
    // argv = (char**)malloc(LENGTH);
    input_analysis(buf, argv);

    /* test */
    printf("argv[0]: %s\n", argv[0]);
    printf("argv[1]: %s\n", argv[1]);
    printf("argv[2]: %s\n", argv[2]);

    /* 実行 */
    if (strcmp(argv[0], CD) == 0)
      chdir(argv[1]);
    else {
      if ((pid = fork()) == 0) {
        if ((ret = execvp(argv[0], argv)) == 0) printf("Error\n"); // 動かない謎
        fflush(stdout);
      }
      pid_wait = wait(NULL);

      /* free */
      while (**argv == '\0') {
        free((*argv)++);
      }
    }
  }

  return 0;
}
