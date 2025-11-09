#include <signal.h>
#include <stdio.h>

#include "signal_handler.h"

volatile sig_atomic_t g_running = 1;

static void handle_signal(int signo) {
    (void)signo;
    g_running = 0;
}

void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}
