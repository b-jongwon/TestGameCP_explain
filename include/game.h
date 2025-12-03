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

#define SUBPIXELS_PER_TILE 10
#define PLAYER_MOVE_STEP_SUBPIXELS 2

#define CONSTANT_PROJECTILE_RANGE 7 // 투사체 사거리
#define AMMO_REFILL_AMOUNT 5 //추가 되는 탄약 수

// 한 스테이지에서 허용하는 최대 장애물 개수.
// - Stage.obstacles 배열의 크기.
// - 스테이지 설계 시 장애물 개수를 이 값 이하로 제한해야 함.
#define MAX_OBSTACLES 64

// 한 스테이지에 배치 가능한 최대 아이템 개수
#define MAX_ITEMS 32

// 한 스테이지에서 동시에 존재할 수 있는 최대 투사체 개수
#define MAX_PROJECTILES 64

// 아이템 종류
typedef enum {
    ITEM_TYPE_SHIELD = 0,   // 한 번 죽을 상황을 무효화해주는 보호막 아이템
    ITEM_TYPE_SCOOTER,       // 이동 속도를 높여주는 E-scooter
    ITEM_TYPE_SUPPLY
} ItemType;

// 맵에 놓이는 아이템 하나
typedef struct {
    int world_x, world_y; // SUBPIXELS_PER_TILE 기준 좌표
    ItemType type;        // 아이템 종류
    int active;           // 1: 아직 남아 있음, 0: 먹혔거나 제거됨
} Item;

// 플레이어가 발사하는 투사체
typedef struct {
    int world_x, world_y; // 현재 위치 (서브픽셀)
    int dir_x, dir_y;     // 이동 방향 단위 벡터 (-1,0,1)
    int active;           // 1: 살아 있는 투사체, 0: 소멸
    int distance_traveled; // 이동 거리 기억
} Projectile;

// 플레이어가 바라보는 방향
typedef enum {
    PLAYER_FACING_DOWN = 0,
    PLAYER_FACING_UP,
    PLAYER_FACING_LEFT,
    PLAYER_FACING_RIGHT,
    PLAYER_FACING_COUNT
} PlayerFacing;

typedef enum {
    PLAYER_ANIM_PHASE_IDLE_A = 0,
    PLAYER_ANIM_PHASE_STEP_A,
    PLAYER_ANIM_PHASE_IDLE_B,
    PLAYER_ANIM_PHASE_STEP_B,
    PLAYER_ANIM_PHASE_COUNT
} PlayerAnimPhase;

// Player 구조체
// - 플레이어 캐릭터의 상태를 표현.
// - 서브픽셀(world_x/world_y) 위치, 생존 여부, 가방 보유 여부와 애니메이션 정보를 들고 있음.
typedef struct {
    int world_x, world_y; // SUBPIXELS_PER_TILE 기준 세분화 좌표
    int target_world_x, target_world_y;
    double move_speed;   // 초당 이동하는 서브픽셀 수 (현재 배율 적용치)
    double base_move_speed; // 스테이지 난이도 기반 기본 속도
    double speed_multiplier; // 이동 속도 배율 (스쿠터 등 아이템 효과)
    double move_accumulator; // 프레임 간 남은 이동량(서브픽셀)
    int moving;          // 이동 중 여부
    int alive;         // 플레이어 생존 여부.
    int has_backpack;  // 가방(G)을 획득했는지 여부
    int has_scooter;   // E-scooter 획득 여부
    double scooter_expire_time; // 스쿠터 효과 종료 시각 (스테이지 경과 시간 기준)
    PlayerFacing facing; // 현재 바라보는 방향
    PlayerAnimPhase anim_phase; // 현재 애니메이션 단계 (idle/step)
    int anim_pixel_progress;    // 다음 프레임 전환까지 누적된 이동 픽셀 수
    int is_moving;     // 1이면 이동 중, 0이면 정지 상태
    double last_move_time; // 마지막으로 실제 이동한 시각(초)
    int shield_count;  // 한 번 살려주는 보호막 개수 (아이템 먹으면 증가, 충돌 시 소비)
} Player;


// 장애물의 "역할" 종류
// - 기존 type(0/1)은 단순히 수평/수직 이동 구분용으로 그대로 두고,
//   kind는 이 장애물이 어떤 캐릭터/패턴인지 구분하는 용도.
typedef enum {
    OBSTACLE_KIND_LINEAR = 0,    // 기존: 상하/좌우로 왔다갔다 하는 일반 장애물
    OBSTACLE_KIND_SPINNER,       // 새로 추가할: 중심을 기준으로 빙글빙글 도는 장애물
    OBSTACLE_KIND_PROFESSOR,      // 교수님: 시야/추격 AI를 가질 장애물

} ObstacleKind;

// Obstacle 구조체
// - 움직이는 장애물 하나를 표현.
// - dir: 이동 방향 (수평/수직에 따라 의미가 달라짐)
// - type: 장애물의 이동 타입 (0 = 가로로 이동, 1 = 세로로 이동)
// - kind: 이 장애물이 어떤 "역할"인지 (일반 / 회전 / 교수님)
// - hp: 투사체에 맞으면 줄어드는 체력 (0 이하이면 비활성화)
// - active: 0이면 죽은 장애물(렌더/충돌에서 무시)
typedef struct {
    int dir;       // 이동 방향. +1이면 오른쪽/아래, -1이면 왼쪽/위 방향으로 이동하도록 사용.
    int type;      // 장애물 타입. 0 = horizontal(수평 이동), 1 = vertical(수직 이동)

    ObstacleKind kind;  // 장애물 역할 종류 (linear / spinner / professor)
    int hp;             // 체력 (투사체에 맞으면 감소)
    int active;         // 1: 활성, 0: 비활성(죽은 상태)

    int world_x, world_y;          // 서브픽셀 좌표
    int target_world_x, target_world_y;
    double move_speed;             // 초당 이동할 서브픽셀 (선형형에 사용)
    double move_accumulator;       // 남은 이동량(서브픽셀)
    int moving;                    // 이동 중 여부

    // 회전형(spinner) 장애물용 필드 (서브픽셀 좌표)
    int center_world_x, center_world_y;  // 회전 중심 좌표
    int orbit_radius_world;              // 중심으로부터의 거리(서브픽셀)
    int angle_step;          // 각도 변화 단위(몇 틱마다 한 칸 회전)
    int angle_index;         // 현재 각도 인덱스(0,1,2,3 같은 식으로 사용 예정)

    // 교수님(Professor) AI용 필드
    int alert;               // 0: 평상시/순찰, 1: 플레이어를 발견하고 추격 중
    int sight_range;         // 시야 거리 (몇 칸까지 보는지)

   
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

     // 아이템 관련 필드
    int num_items;                       // 아이템 개수
    Item items[MAX_ITEMS];               // 아이템 배열

    // 투사체 관련 필드
    int num_projectiles;                 // 사용 중인 투사체 개수 (혹은 전체 슬롯 수 관리용)
    Projectile projectiles[MAX_PROJECTILES]; // 투사체 배열
   
    int width;                           // 실제 사용 중인 맵 가로 길이
    int height;                          // 실제 사용 중인 맵 세로 길이

    double difficulty_player_speed;      // 플레이어 이동 속도 (타일당 초)
    int remaining_ammo; // 게임당 발사 가능한 투사체 수 제한
} Stage;

#endif // GAME_H
