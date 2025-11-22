#include <signal.h>   // sigaction, sig_atomic_t, SIGINT, SIGTERM
#include <stdio.h>    // (현재는 사용 안 하지만 디버깅용으로 쓸 수도 있음)
#include <unistd.h>   // ← _exit() 선언 추가
#include <ncurses.h>  // ← endwin() 선언 추가
#include "input.h"    // ← restore_input() 선언 추가
#include "signal_handler.h"  // g_running, setup_signal_handlers 선언


// 게임 실행 여부를 나타내는 전역 플래그.
// - volatile sig_atomic_t: 시그널 핸들러와 메인 코드 양쪽에서 안전하게 접근 가능.
// - 1이면 계속 실행, 0이면 메인 루프들이 종료를 향해 감.
volatile sig_atomic_t g_running = 1;


// ----------------------------------------------------------
// handle_signal()
// ----------------------------------------------------------
// 실제 시그널이 발생했을 때 호출되는 핸들러 함수.
// - signo: 어떤 시그널인지(예: SIGINT, SIGTERM)지만 여기서는 실제 값은 사용 안 함.
// - 해야 할 일: g_running을 0으로 바꾸어 메인 루프 탈출 유도.
//static void handle_signal(int signo) {
   // (void)signo;   // 매개변수를 사용하지 않으므로 경고 방지 용

  //  g_running = 0; // 게임 종료 요청 플래그
//}
static void handle_signal(int signo) {
    (void)signo;
    g_running = 0;
    restore_input();
    endwin();
    _exit(0);
}

// ----------------------------------------------------------
// setup_signal_handlers()
// ----------------------------------------------------------
// SIGINT(Ctrl+C), SIGTERM 등 프로세스 종료 관련 시그널에 대해
// handle_signal 함수가 호출되도록 설정.
void setup_signal_handlers(void) {

    struct sigaction sa;

    // sa_handler에 우리가 만든 handle_signal 지정
    sa.sa_handler = handle_signal;

    // 추가로 막을 시그널 없음 (빈 마스크)
    sigemptyset(&sa.sa_mask);

    // 특별한 플래그 사용 안 함
    sa.sa_flags = 0;

    // SIGINT, SIGTERM 시그널에 대해 핸들러 등록
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}
