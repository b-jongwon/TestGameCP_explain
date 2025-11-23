// game.h
// --------------------------------------------------
// 게임 전체에서 공통으로 사용하는 상수와 핵심 구조체(Player, Obstacle, Stage)를 정의하는 헤더.
// 이 헤더는 사실상 "게임 도메인 모델"을 정의하는 역할을 한다고 보면 됨.
// 대부분의 다른 모듈들이 이 파일을 include해서 Player/Stage 정보를 공유한다.

#ifndef GAME_H                 // game.h 중복 include 방지용 include guard 시작
#define GAME_H

// 게임 맵의 가로 길이(열 수). 
// - Stage.map에서 한 줄에 들어가는 문자 개수와 동일해야 함.
// - 렌더링, 충돌 판정, 스테이지 파일 파싱 등에서 공통 기준으로 사용.
#define MAX_X 255

// 게임 맵의 세로 길이(행 수).
// - Stage.map에서 전체 줄의 개수와 동일.
// - 터미널 화면에서 y 좌표 범위, 플레이어 이동, 장애물 이동, 맵 출력에 사용.
#define MAX_Y 100

// 한 스테이지에서 허용하는 최대 장애물 개수.
// - Stage.obstacles 배열의 크기.
// - 스테이지 설계 시 장애물 개수를 이 값 이하로 제한해야 함.
#define MAX_OBSTACLES 64

// 플레이어가 바라보는 방향
typedef enum {
    PLAYER_FACING_DOWN = 0,
    PLAYER_FACING_UP,
    PLAYER_FACING_LEFT,
    PLAYER_FACING_RIGHT,
    PLAYER_FACING_COUNT
} PlayerFacing;

// Player 구조체
// - 플레이어 캐릭터의 상태를 표현.
// - 위치(x, y)와 생존 여부 외에도 가방 보유 여부와 애니메이션 정보를 들고 있음.
typedef struct {
    int x, y;          // 플레이어의 현재 위치 (맵 좌표)
    int alive;         // 플레이어 생존 여부.
    int has_backpack;  // 가방(G)을 획득했는지 여부
    PlayerFacing facing; // 현재 바라보는 방향
    int anim_step;     // 이동 중 프레임 토글 (0/1)
    int is_moving;     // 1이면 이동 중, 0이면 정지 상태
    double last_move_time; // 마지막으로 실제 이동한 시각(초)
} Player;

// Obstacle 구조체
// - 움직이는 장애물 하나를 표현.
// - x, y: 현재 위치
// - dir: 이동 방향 (수평/수직에 따라 의미가 달라짐)
// - type: 장애물의 이동 타입 (0 = 가로로 이동, 1 = 세로로 이동)
typedef struct {
    int x, y;      // 장애물 현재 위치 (맵 좌표)
    int dir;       // 이동 방향. +1이면 오른쪽/아래, -1이면 왼쪽/위 방향으로 이동하도록 사용.
    int type;      // 장애물 타입. 0 = horizontal(수평 이동), 1 = vertical(수직 이동)
} Obstacle;

// Stage 구조체
// - 한 스테이지에 대한 거의 모든 정보를 담는 구조체.
// - 스테이지 ID, 이름, 맵 데이터, 시작/목표 위치, 장애물 목록 등을 포함.
// - 사실상 "게임 한 판"에 대한 모든 환경 정보를 가지고 있음.
typedef struct {
    int id;                              // 스테이지 ID 번호 (1, 2, 3 ... 이런 식으로 구분)
    char name[32];                       // 스테이지 이름

    char map[MAX_Y][MAX_X + 1];          // 실제 맵 데이터 (최대 크기)

    int start_x, start_y;                // 시작 위치
    int goal_x, goal_y;                  // 목표 위치

    int num_obstacles;                   // 장애물 개수
    Obstacle obstacles[MAX_OBSTACLES];   // 장애물 배열

    // 🔥 새로 추가
    int width;                           // 실제 사용 중인 맵 가로 길이
    int height;                          // 실제 사용 중인 맵 세로 길이
} Stage;

#endif // GAME_H
