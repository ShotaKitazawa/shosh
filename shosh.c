#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LENGTH 64

void input_read(char* bp) {
  int fd_stdin = fileno(stdin);
  do {
    bp++;  // 先頭がNULLのため
    read(fd_stdin, bp, 1);
    if (*bp == 0x7f) {   // backspace
      printf("\b\b\b");  // 3文字戻る (消したい1文字+BS記号2文字)
      printf("   ");     // 3文字埋める
      printf("\b\b\b");  // 3文字戻る
      bp--;
      if (*bp == '\0') bp++;
    }
    // printf("%x\n", *bp);  // デバッグ用
    fflush(stdout);
  } while (*bp != 0x0a);
  *bp = '\0';
}

/* char* 型の文字列をスペース区切りの char** 型に変換する
 * , {"|'} で囲った場合区切らない */
void input_analyse(char buf[], char* argv[]) {
  char* bp = buf;
  bp++;  // buf 先頭 = NULL のため
  *argv = (char*)malloc(LENGTH);
  int n = 0;
  int dq_flag = 0;
  int sq_flag = 0;
  int dollar_flag = 0;
  int env_flag = 0;
  char env_buf[LENGTH];
  char* envp = env_buf;
  char env_return[LENGTH];

  while (*bp != '\0') {
    if (*bp == 0x20 && !(dq_flag || sq_flag)) {  // space
      **argv = '\0';
      while (n > 0) {
        n--;
        (*argv)--;
      }
      argv++;
      *argv = (char*)malloc(LENGTH);
    } else if (*bp == 0x22) {  // "
      dq_flag ^= 1;
    } else if (*bp == 0x24 && !sq_flag) {  // $
      dollar_flag = 1;
    } else if (*bp == 0x27) {  // '
      sq_flag ^= 1;
    } else if (*bp == 0x7b && dollar_flag) {  // ${HOGE} の { の部分
      env_flag = 1;
    } else if (*bp == 0x7d && dollar_flag && env_flag) {  // ${HOGE} の } の部分
      strcpy(env_return, getenv(env_buf));
      envp = env_return;
      while (*envp != '\0') {
        **argv = *envp;
        (*argv)++;
        envp++;
        n++;
      }
      memset(env_buf, '\0', LENGTH);
      envp = env_buf;
      dollar_flag = 0;
      env_flag = 0;
    } else if (env_flag) {  // ${HOGE} の HOGE の部分
      *envp = *bp;
      envp++;
    } else {
      **argv = *bp;
      (*argv)++;
      n++;
    }
    bp++;
  }
  **argv = '\0';
  while (n > 0) {
    n--;
    (*argv)--;
  }
}

int main() {
  // 変数宣言・初期化
  char buf[LENGTH];  // 0番目はNULL (BackSpace判定のため)
  char* bp;
  char* argv[LENGTH];
  pid_t pid, pid_wait;
  int ret;
  char* username = getlogin();
  char hostname[LENGTH]; gethostname(hostname, sizeof(hostname));
  char cwd[LENGTH];


  while (1) {
    /* コマンド毎の初期化 */
    getcwd(cwd, sizeof(cwd));
    printf("[%s@%s %s]$ ", username, hostname, cwd);
    fflush(stdout);
    memset(buf, '\0', LENGTH);
    memset(argv, '\0', LENGTH);
    bp = buf;
    *bp = '\0';

    /* Enter まで読み込み */
    input_read(bp);

    /* 入力は buff[1] から */
    bp++;

    /* 入力がなかったら continue */
    if (*bp == '\0') continue;

    /* exit が入力されたら return 0 */
    if (strcmp(bp, "exit") == 0) return 0;

    /* char* 型の文字列をスペース区切りの char** 型に変換する */
    // argv = (char**)malloc(LENGTH);
    input_analyse(buf, argv);

    /* test */
    //printf("argv[0]: %s\n", argv[0]);
    //printf("argv[1]: %s\n", argv[1]);
    //printf("argv[2]: %s\n", argv[2]);

    /* 実行 */
    if (strcmp(argv[0], "cd") == 0)
      chdir(argv[1]);
    else {
      if ((pid = fork()) == 0) {
        if ((ret = execvp(argv[0], argv)) == 0)
          printf("Error\n");  // 動かない謎
      }
      pid_wait = wait(NULL);
    }
    fflush(stdout);

    /* free */
    while (**argv == '\0') free((*argv)++);
  }

  return 0;
}
