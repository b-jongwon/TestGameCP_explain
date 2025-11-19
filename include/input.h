// input.h
// ----------------------------------------------------
// 터미널에서 키보드 입력을 비차단(non-blocking)으로 읽어오기 위한 인터페이스.
// - init_input: 터미널 모드 변경 (버퍼링/에코 끄기 등)
// - restore_input: 프로그램 종료 시 터미널 설정 복구
// - poll_input: 키가 눌렸는지 확인하고, 눌렸으면 해당 문자 반환

#ifndef INPUT_H
#define INPUT_H


// 입력 초기화
void init_input(void);

// 입력 복구
void restore_input(void);

// raw input
int read_input(void);
// 현재 키보드 입력을 비동기적으로 검사하는 함수.
// - 키가 눌리지 않았으면 -1을 반환.
// - 키가 눌렸으면 해당 키의 ASCII 코드(char)를 int로 반환.
// - 메인 게임 루프에서 매 프레임마다 호출해서 "현재 눌린 키"를 체크하는 용도로 사용.
//   예: int ch = poll_input(); if (ch == 'w') 위로 이동, 등.
int poll_input(void); // returns -1 if no key, otherwise char

#endif // INPUT_H
