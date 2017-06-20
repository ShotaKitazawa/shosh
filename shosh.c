#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define LENGTH 64

/* Terminal mode 定義*/
struct termios CookedTermIos;  // cooked モード用
struct termios RawTermIos;     // raw モード用

/* グローバル変数 */
pid_t pid, pid_wait;  // fork, wait 用
int ret;              // 子プロセス exec 時の値

/* 関数の定義 */
void sigcatch(int sig);
void print_env();
void input_read(char* bp);
void input_analyse(char buf[], char* argv[]);
void argv_execute(char* argv[]);
void argv_free(char* argv[]);

int main() {
  /* 変数宣言・初期化 */
  char buf[LENGTH];    // 0番目はNULL (BackSpace判定のため)
  char* bp;            // buf へのポインタ
  char* argv[LENGTH];  // buf をスペース区切りにしたもの

  /* シェルに対する SIGINT, SIGTSTP シグナルの無効化 */
  if (SIG_ERR == signal(SIGINT, sigcatch)) {
    printf("failed to set signal handler.n");
    exit(1);
  }
  if (SIG_ERR == signal(SIGTSTP, sigcatch)) {
    printf("failed to set signal handler.n");
    exit(1);
  }

  while (1) {
    /* コマンド毎の初期化 */
    print_env();
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

    /* exit が入力されたら正常終了 */
    if (strcmp(bp, "exit") == 0) exit(0);

    /* char* 型の文字列をスペース区切りの char** 型に変換する */
    // argv = (char**)malloc(LENGTH);
    input_analyse(buf, argv);

    /* test */
    // printf("argv[0]: %s\n", argv[0]);
    // printf("argv[1]: %s\n", argv[1]);
    // printf("argv[2]: %s\n", argv[2]);

    /* 実行 */
    argv_execute(argv);

    /* free */
    argv_free(argv);
  }
}

void sigcatch(int sig) {
  // printf("called sigcatch\n");
  switch (sig) {
    case 2:  // C-c
      return;
    case 20:  // C-z
      return;
    default:
      break;
  }
  return;
}

void print_env() {
  char* username = getlogin();
  char hostname[LENGTH];
  gethostname(hostname, sizeof(hostname));
  char cwd[LENGTH];
  getcwd(cwd, sizeof(cwd));
  printf("[%s@%s %s]$ ", username, hostname, cwd);
}

void input_read(char* bp) {
  /* 初期化 */
  int enter_flag = 0;

  /* cooked, raw モードの状態を保存 */
  tcgetattr(STDIN_FILENO, &CookedTermIos);
  RawTermIos = CookedTermIos;
  cfmakeraw(&RawTermIos);

  /* 端末状態変更: Cooked > Raw */
  tcsetattr(STDIN_FILENO, 0, &RawTermIos);

  int fd_stdin = fileno(stdin);
  do {
    bp++;  // 先頭がNULLのため

    /* 入力 */
    // if (*bp == '\0'){
    *bp = getchar();
    // } else
    // TODO: *bp が NULL でない > Ctrl+f でカーソルを移動してきた > 1文字ずつ
    // *bp を前にずらす

    /* デバッグ用 */
    // printf("%x", *bp);
    // fflush(stdout);

    /* もし図形文字ならば出力 */
    if ((*bp & 0xe0) > 0x00 && *bp != 0x7f) {
      putchar(*bp);
    }

    /* 出力文字以外の場合、キーごとの処理後 *bp='\0';bp--; */
    else {
      /* enter */
      if (*bp == 0x0d) enter_flag++;
      /* backspace */
      if (*bp == 0x7f) {
        bp--;
        if (*bp != '\0') {
          *bp = '\0';
          printf("\b");  // 1文字戻る
          printf(" ");   // 1文字埋める
          printf("\b");  // 1文字戻る
        } else {
          bp++;
        }
      }
      /* C-c */
      if (*bp == 0x03) {
        tcsetattr(STDIN_FILENO, 0, &CookedTermIos);
        printf("\n");
        print_env();
        while (*bp != '\0') {
          *bp = '\0';
          bp--;
        }
        bp++;
        tcsetattr(STDIN_FILENO, 0, &RawTermIos);
      }
      /* C-d && 文字入力なし */
      if (*bp == 0x04) {
        bp--;
        if (*bp == '\0') {
          /* 端末状態変更: Raw > Cooked */
          tcsetattr(STDIN_FILENO, 0, &CookedTermIos);
          printf("\n");
          exit(0);
        }
        bp++;
      }
      *bp = '\0';
      bp--;
    }
  } while (!enter_flag);  // Enter

  /* NULL Terminate */
  bp++;
  *bp = '\0';

  /* 端末状態変更: Raw > Cooked */
  tcsetattr(STDIN_FILENO, 0, &CookedTermIos);
  printf("\n");

  return;
}

/* char* 型の文字列をスペース区切りの char** 型に変換する,
 * {",'} で囲った場合区切らない,
 * ${HOGE} の展開 */
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
  char*** argvs;

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
    } else if (*bp == 0x7c) { // |
      *argvs = (char**)malloc(LENGTH);
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

void argv_execute(char* argv[]) {
  if (strcmp(argv[0], "cd") == 0)
    chdir(argv[1]);
  else {
    if ((pid = fork()) == 0) {
      if ((ret = execvp(argv[0], argv)) < 0) {
        printf("-shosh: %s: command not found\n", argv[0]);
      }
      exit(0);
    }
    pid_wait = wait(NULL);
  }
}

void argv_free(char* argv[]) {
  while (**argv == '\0') free((*argv)++);
}

void argvs_free(char*** argvs){
}
