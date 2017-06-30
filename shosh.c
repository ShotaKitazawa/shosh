#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define LENGTH 64

/* TODO 表示 */
const char* URL = "https://github.com/ShotaKitazawa/shosh";

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
void input_pipe_separate(char buf[], char*** argvs);
void input_analyse(char buf[], char** argv);
void argvs_execute(char*** argvs);
void argv_execute(char** argv);
void argvs_free(char*** argvs);

int main() {
  /* 変数宣言・初期化 */
  char buf[LENGTH];  // 0番目はNULL (BackSpace判定のため)
  char* bp;          // buf へのポインタ
  char*** argvs;      // buf をスペース区切りにしたもの

  /* シェルに対する SIGINT, SIGTSTP シグナルの無効化 */
  if (SIG_ERR == signal(SIGINT, sigcatch)) {
    printf("failed to set signal handler\n");
    exit(1);
  }
  if (SIG_ERR == signal(SIGTSTP, sigcatch)) {
    printf("failed to set signal handler\n");
    exit(1);
  }

  while (1) {
    /* コマンド毎の初期化 */
    print_env();
    memset(buf, '\0', LENGTH);
    argvs = (char***)malloc(LENGTH);
    *argvs = (char**)malloc(LENGTH);
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
    input_pipe_separate(buf, argvs);

    /* test */
    //printf("argvs[0][0]: %s\n", argvs[0][0]);
    //printf("argvs[0][1]: %s\n", argvs[0][1]);
    //printf("argvs[0][2]: %s\n", argvs[0][2]);

    /* test (セグフォるから要修正) */
    //int i, j;
    //for (i = 0;*argvs != '\0'; i++){
    //  for (j = 0;**argvs != '\0'; j++){
    //    printf("argvs[%d][%d]: %s\n", i, j, argvs[i][j]);
    //  (*argvs)++;
    //  }
    //  argvs++;
    //}

    /* 実行 */
    argvs_execute(argvs);

    /* free */
    argvs_free(argvs);
  }
}

void sigcatch(int sig) {
  // printf("called sigcatch: %d\n", sig);
  switch (sig) {
    case 2:  // C-c
      return;
    case 18:  // C-z
      return;
    default:
      break;
  }
  return;
}

void print_env() {
  char* username = getlogin();
  char hostname[LENGTH];
  char cwd[LENGTH];
  gethostname(hostname, sizeof(hostname));
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
      if (*bp == 0x0d) {  // enter
        enter_flag++;
      }
      if (*bp == 0x7f) {  // backspace
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
      if (*bp == 0x09) {  // Tab
        // TODO
      }
      if (*bp == 0x03) {  // Ctrl + c
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
      if (*bp == 0x04) {  // C-d && 文字入力なし
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

void input_pipe_separate(char buf[], char*** argvs) {
  char tmp_buf[LENGTH];
  char* tbp = tmp_buf;
  char* bp = buf;
  bp++;  // buf 先頭 = NULL のため

  while (*bp != '\0') {
    if (*bp == 0x7c) {  // pipe
      *tbp = '\0';
      input_analyse(tmp_buf, *argvs);
      memset(tmp_buf, '\0', LENGTH);
      tbp = tmp_buf;
      argvs++;
      *argvs = (char**)malloc(LENGTH);
    } else {
      *tbp = *bp;
      tbp++;
    }
    bp++;
  }
  *tbp = '\0';
  input_analyse(tmp_buf, *argvs);
  argvs++;
  *argvs = '\0';
}

/* char* 型の文字列をスペース区切りの char** 型に変換する,
 * 特殊文字を読んだらなにかする*/
void input_analyse(char buf[], char** argv) {
  char* bp = buf;
  *argv = (char*)malloc(LENGTH);
  int word_count = 0;   // spece で区切られた各文字列の文字数
  int space_flag = 0;
  int dq_flag = 0;      // " (double quotation) flag
  int sq_flag = 0;      // ' (single quotation) flag
  int dollar_flag = 0;  // $ flag
  int env_flag = 0;     // environment_variable flag
  char env_buf[LENGTH];
  char* envp = env_buf;
  char env_return[LENGTH];

  while (*bp != '\0') {
      space_flag = 0;
    if (*bp == 0x20 && !dq_flag && !sq_flag) {  // space
      space_flag = 1;
      if (!word_count) {
        bp++;
        continue;
      }
      **argv = '\0';
      while (word_count > 0) {
        word_count--;
        (*argv)--;
      }
      argv++;
      *argv = (char*)malloc(LENGTH);
      **argv = '\0';
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
        word_count++;
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
      word_count++;
    }
    bp++;
  }
  if (space_flag) *argv = '\0';
  else **argv = '\0';

  while (word_count > 0) {
    word_count--;
    (*argv)--;
  }
  argv++;
  *argv = '\0';
}

void argvs_execute(char*** argvs) {
  argv_execute(*argvs);
}

void argv_execute(char** argv) {
  if (strcmp(argv[0], "cd") == 0) {
    if (!argv[1])
      chdir(getenv("HOME"));
    else if ((ret = chdir(argv[1])) < 0)
      printf("-shosh: cd: %s: No such file or directory\n", argv[1]);
  } else {
    if ((pid = fork()) < 0)
      perror("### Fork failed! ###");
    else if (pid == 0) {
      if ((ret = execvp(argv[0], argv)) < 0) {
        printf("-shosh: %s: command not found\n", argv[0]);
      }
      exit(0);
    }
    pid_wait = wait(NULL);
  }
}


void argvs_free(char*** argvs) {
  //char*** argvs_copy;
  ////while(*argvs != '\0'){
  //  while(**argvs != '\0'){
  //    argvs_copy = argvs;
  //    argvs_copy++;
  //    free(**argvs);
  //    argvs = argvs_copy;
  //  }
  //}
}
