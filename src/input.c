#include <termios.h>    // 터미널 입출력 속성을 다루는 구조체/함수 (tcgetattr, tcsetattr 등)
#include <unistd.h>     // STDIN_FILENO 상수, 기타 POSIX 함수
#include <fcntl.h>      // fcntl(), O_NONBLOCK 플래그 설정에 필요
#include <stdio.h>      // getchar(), clearerr(), FILE* 관련

// 원래 터미널 설정을 저장해둘 전역(static) 변수.
// - init_input에서 현재 설정을 oldt에 저장해두고,
//   restore_input에서 다시 복구하는 구조.
static struct termios oldt;

// 입력 초기화가 이미 되었는지 확인하기 위한 플래그.
// - 중복으로 init_input() 호출하는 것을 방지.
static int input_initialized = 0;


// ----------------------------------------------------------
// init_input()
// ----------------------------------------------------------
// 터미널 입력을 게임에 맞게 세팅하는 함수.
// - 표준 입력(STDIN)을 non-canonical, no-echo 모드로 변경.
// - 즉, 엔터 없이 즉시 입력이 들어오고, 입력한 글자가 화면에 보이지 않게 셋업.
// - 또한 fcntl로 stdin을 non-blocking 모드로 바꿔 poll_input에서 기다리지 않게 한다.
void init_input(void) {
    // 이미 초기화된 상태면 다시 할 필요 없음
    if (input_initialized) return;

    struct termios newt;   // 새로 적용할 터미널 설정 구조체

    // 현재 터미널 설정을 oldt에 저장
    tcgetattr(STDIN_FILENO, &oldt);

    // oldt를 복사해서 newt를 만들고, 그걸 수정해 사용
    newt = oldt;

    // newt.c_lflag에서 ICANON, ECHO 비트를 끈다.
    // - ICANON: canonical 모드 (줄 단위 입력) → 끄면 문자 단위로 바로바로 읽힘
    // - ECHO: 사용자가 입력한 글자를 화면에 자동으로 에코 → 끄면 입력이 화면에 안 나타남
    newt.c_lflag &= ~(ICANON | ECHO);

    // 수정된 newt를 즉시(TCSANOW) 적용한다.
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // 표준 입력(STDIN_FILENO)에 non-blocking 플래그 설정.
    // - O_NONBLOCK: 읽을 데이터가 없어도 read/getchar가 블록되지 않고 즉시 반환.
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    // 이제 초기화 완료 플래그 설정
    input_initialized = 1;
}


// ----------------------------------------------------------
// restore_input()
// ----------------------------------------------------------
// 프로그램 종료 시, 터미널 설정을 원래대로 되돌리는 함수.
// - init_input에서 저장해둔 oldt를 다시 적용.
// - non-blocking 플래그도 제거하여 정상 상태로 돌린다.
void restore_input(void) {
    // 초기화되지 않았으면 복구할 것도 없음
    if (!input_initialized) return;

    // oldt에 저장해둔 원래 터미널 설정 복구
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    // stdin의 파일 상태 플래그를 0으로 설정 (기본 블로킹 모드)
    fcntl(STDIN_FILENO, F_SETFL, 0);

    // 플래그 리셋
    input_initialized = 0;
}


// ----------------------------------------------------------
// poll_input()
// ----------------------------------------------------------
// 현재 입력 버퍼에 문자가 들어와 있으면 바로 읽어서 반환,
// 아무 입력도 없다면 -1을 반환하는 함수.
// - getchar() 사용하지만, stdin이 non-blocking으로 설정되어 있기 때문에
//   입력이 없으면 EOF를 반환하게 됨.
// - 메인 루프에서 매 프레임마다 호출해 "지금 눌린 키"를 체크.
int poll_input(void) {
    int ch = getchar();  // stdin에서 한 글자 읽기 시도

    // 입력이 없거나 에러일 경우 EOF 반환
    if (ch == EOF) {
        // 에러/EOF 상태 플래그 초기화
        clearerr(stdin);
        return -1;       // "입력 없음"을 의미
    }

    // 실제로 읽힌 문자 코드 반환
    return ch;
}
