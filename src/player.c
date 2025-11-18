#include "player.h"   // Player, Stage, MAX_X, MAX_Y, 함수 선언 등 포함


// ----------------------------------------------------------
// init_player()
// ----------------------------------------------------------
// 플레이어를 특정 스테이지의 시작 위치로 초기화하는 함수.
// - p: 초기화할 Player 구조체 포인터
// - stage: 현재 스테이지. stage->start_x, start_y를 사용.
void init_player(Player *p, const Stage *stage) {

    // 스테이지에서 지정한 시작 위치로 좌표 설정
    p->x = stage->start_x;
    p->y = stage->start_y;

    // 플레이어를 "살아있는 상태"로 설정
    p->alive = 1;
}


// ----------------------------------------------------------
// move_player()
// ----------------------------------------------------------
// 입력된 키(input)에 따라 플레이어의 위치를 한 칸 이동시키는 함수.
// - W/w: 위로, S/s: 아래로, A/a: 왼쪽으로, D/d: 오른쪽으로 이동.
// - 맵 범위를 벗어나는지, 벽('@')인지 검사하여 이동 가능 여부를 판단.
// - 여기서는 단순히 이동만 하고, 충돌/골 도달 체크는 다른 함수에서 처리.
void move_player(Player *p, char input, const Stage *stage) {

    // 새로운 좌표 후보(nx, ny)를 현재 위치로 초기화
    int nx = p->x;
    int ny = p->y;

    // 입력 키에 따라 이동 방향 결정
    if (input == 'w' || input == 'W') ny--;   // 위로 한 칸
    else if (input == 's' || input == 'S') ny++; // 아래로 한 칸
    else if (input == 'a' || input == 'A') nx--; // 왼쪽 한 칸
    else if (input == 'd' || input == 'D') nx++; // 오른쪽 한 칸

    // 맵 범위를 벗어나면 이동 무시
    if (nx < 0 || nx >= MAX_X || ny < 0 || ny >= MAX_Y)
        return;

    // 새 위치에 해당하는 맵의 문자 확인
    char cell = stage->map[ny][nx];

    // '@' 문자이면 벽으로 간주 → 이동 불가
    if (cell == '@')
        return;

    // 위 조건을 모두 통과했다면 실제 플레이어 좌표를 새 위치로 업데이트
    p->x = nx;
    p->y = ny;
}
