#include <signal.h>
#include <stddef.h>
#include "signal_handler.h"


// 게임 실행 여부를 나타내는 전역 플래그.
// - volatile sig_atomic_t: 시그널 핸들러와 메인 코드 양쪽에서 안전하게 접근 가능.
// - 1이면 계속 실행, 0이면 메인 루프들이 종료를 향해 감.
volatile sig_atomic_t g_running = 1;



static void handle_signal(int signo)
{
    (void)signo;
    g_running = 0;
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
