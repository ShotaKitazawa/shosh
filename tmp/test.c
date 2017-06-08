/* http://d.hatena.ne.jp/web_develop/20071113/1194971862 */

#include <sys/ioctl.h>
#include <stdio.h>
#include <asm/termbits.h>

int main() {
  char c;
  struct termio tm, tm_save;
  int fd_stdin;

  fd_stdin = fileno(stdin);

  /* 現在の設定を格納 */
  ioctl(fd_stdin, TCGETA, &tm);
  tm_save = tm;

  /* 端末に設定を反映 */
  tm.c_lflag &= ~ICANON;
  ioctl(fd_stdin, TCSETAF, &tm);

  printf("1文字入力してください？");
  fflush(stdout);
  read(fd_stdin, &c, 1);
  printf("\n入力文字 = %x\n", c);

  /* 設定を元に戻す */
  ioctl(fd_stdin, TCSETAF, &tm_save);

  return 0;
}
