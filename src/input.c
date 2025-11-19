#include <ncurses.h>

void init_input(void)
{
    nodelay(stdscr, TRUE);  // getch non-blocking
    noecho();               // 입력 문자 표시 X
    cbreak();               // 줄 단위 버퍼링 끄기
    keypad(stdscr, TRUE);   // 화살표 등 특수키 허용
}

void restore_input(void)
{
    echo();
    nocbreak();
    nodelay(stdscr, FALSE);
    keypad(stdscr, FALSE);
}

int read_input(void)
{
    int ch = getch();
    if (ch == ERR) return -1;
    return ch;
}

int poll_input(void)
{
    return read_input();
}