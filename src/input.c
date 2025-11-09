#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

static struct termios oldt;
static int input_initialized = 0;

void init_input(void) {
    if (input_initialized) return;
    struct termios newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    input_initialized = 1;
}

void restore_input(void) {
    if (!input_initialized) return;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, 0);
    input_initialized = 0;
}

int poll_input(void) {
    int ch = getchar();
    if (ch == EOF) {
        clearerr(stdin);
        return -1;
    }
    return ch;
}
