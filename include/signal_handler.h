// signal_handler.h
// ------------------------------------------------------------------
// SIGINT(Ctrl + C) 같은 OS 시그널을 처리하기 위한 인터페이스.
// - g_running: 게임 메인 루프를 돌릴지 말지 결정하는 전역 플래그.
// - setup_signal_handlers: SIGINT 등이 오면 g_running을 0으로 만들어
//   루프를 자연스럽게 빠져나오도록 설정.

#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>    // sigaction, sig_atomic_t 등 시그널 관련 타입/함수

// 메인 루프에서 계속 체크하는 전역 플래그.
// - 보통 while (g_running) { ... } 이런 식으로 사용.
// - 시그널 핸들러에서 이 값을 0으로 바꾸어 "이제 끝내라"는 신호를 보냄.
// - volatile sig_atomic_t:
//   * 시그널 핸들러와 메인 코드 사이에서 안전하게 접근할 수 있는 정수 타입.
//   * 컴파일러 최적화나 CPU 캐시 때문에 값이 꼬이지 않도록 보장.
extern volatile sig_atomic_t g_running;

// 실제 시그널 핸들러를 등록하는 함수.
// - 내부에서 sigaction 등을 사용해 SIGINT(CTRL+C) 수신 시 호출될 함수를 지정.
// - 그 핸들러에서는 g_running = 0; 을 세팅해서 메인 루프 종료를 유도.
// - 메인 함수 시작 부분에서 한 번 호출해 두면,
//   사용자나 OS가 강제로 프로그램을 끊을 때도 깔끔하게 정리할 수 있음.
void setup_signal_handlers(void);

#endif // SIGNAL_HANDLER_H
