// game.h

#ifndef GAME_H // game.h 중복 include 방지용 include guard 시작
#define GAME_H

#define MAX_X 255             // 게임 맵의 가로 길이(열 수).
#define MAX_Y 100             // 게임 맵의 세로 길이(행 수).
#define SUBPIXELS_PER_TILE 10 // 1프레임당 이동거리
#define PLAYER_MOVE_STEP_SUBPIXELS 2
#define CONSTANT_PROJECTILE_RANGE 10 // 투사체 사거리
#define SUPPLY_REFILL_AMOUNT 5       // 추가 되는 탄약 수
#define MAX_OBSTACLES 64             // 한 스테이지에서 허용하는 최대 장애물 개수.
#define MAX_ITEMS 32                 // 한 스테이지에 배치 가능한 최대 아이템 개수
#define MAX_PROJECTILES 64           // 한 스테이지에서 동시에 존재할 수 있는 최대 투사체 개수
#define MAX_PROFESSOR_CLONES 48      // 교수 패턴에서 동시에 관리할 수 있는 분신 수

// 아이템 종류
typedef enum
{
    ITEM_TYPE_SHIELD = 0, // 한 번 죽을 상황을 무효화해주는 보호막 아이템
    ITEM_TYPE_SCOOTER,    // 이동 속도를 높여주는 E-scooter
    ITEM_TYPE_SUPPLY
} ItemType;

// 맵에 놓이는 아이템 하나
typedef struct
{
    int world_x, world_y; // SUBPIXELS_PER_TILE 기준 좌표
    ItemType type;        // 아이템 종류
    int active;           // 1: 아직 남아 있음, 0: 먹혔거나 제거됨
} Item;

// 플레이어가 발사하는 투사체
typedef struct
{
    int world_x, world_y;  // 현재 위치 (서브픽셀)
    int dir_x, dir_y;      // 이동 방향 단위 벡터 (-1,0,1)
    int active;            // 1: 살아 있는 투사체, 0: 소멸
    int distance_traveled; // 이동 거리 기억
} Projectile;

// 플레이어가 바라보는 방향
typedef enum
{
    PLAYER_FACING_DOWN = 0,
    PLAYER_FACING_UP,
    PLAYER_FACING_LEFT,
    PLAYER_FACING_RIGHT,
    PLAYER_FACING_COUNT
} PlayerFacing;

typedef enum
{
    PLAYER_ANIM_PHASE_IDLE_A = 0,
    PLAYER_ANIM_PHASE_STEP_A,
    PLAYER_ANIM_PHASE_IDLE_B,
    PLAYER_ANIM_PHASE_STEP_B,
    PLAYER_ANIM_PHASE_COUNT
} PlayerAnimPhase;

// Player 구조체
typedef struct
{
    int world_x, world_y;               // SUBPIXELS_PER_TILE 기준 세분화 좌표
    int target_world_x, target_world_y; // 목표 도달 검사 좌표
    double move_speed;                  // 초당 이동하는 서브픽셀 수 (현재 배율 적용치)
    double base_move_speed;             // 스테이지 난이도 기반 기본 속도
    double speed_multiplier;            // 이동 속도 배율 (스쿠터 등 아이템 효과)
    double move_accumulator;            // 프레임 간 남은 이동량(서브픽셀)
    int moving;                         // 이동 중 여부
    int alive;                          // 플레이어 생존 여부.
    int has_backpack;                   // 가방(G)을 획득했는지 여부
    int has_scooter;                    // E-scooter 획득 여부
    double scooter_expire_time;         // 스쿠터 효과 종료 시각 (스테이지 경과 시간 기준)
    PlayerFacing facing;                // 현재 바라보는 방향
    PlayerAnimPhase anim_phase;         // 현재 애니메이션 단계 (idle/step)
    int anim_pixel_progress;            // 다음 프레임 전환까지 누적된 이동 픽셀 수
    int is_moving;                      // 1이면 이동 중, 0이면 정지 상태
    double last_move_time;              // 마지막으로 실제 이동한 시각(초)
    int shield_count;                   // 한 번 살려주는 보호막 개수 (아이템 먹으면 증가, 충돌 시 소비)

    int is_confused; // 1: 혼란 상태(이동 방향 반대), 0: 정상
} Player;

typedef enum // 장애물 역할구분 구조체
{
    OBSTACLE_KIND_LINEAR = 0,     // 기존: 상하/좌우로 왔다갔다 하는 일반 장애물
    OBSTACLE_KIND_SPINNER,        // 새로 추가할: 중심을 기준으로 빙글빙글 도는 장애물
    OBSTACLE_KIND_PROFESSOR,      // 교수님: 시야/추격 AI를 가질 장애물
    OBSTACLE_KIND_BREAKABLE_WALL, // 부서지는 벽
} ObstacleKind;

typedef struct // Obstacle 구조체
{
    int dir;  // 이동 방향. +1이면 오른쪽/아래, -1이면 왼쪽/위 방향으로 이동하도록 사용.
    int type; // 장애물 타입. 0 = horizontal(수평 이동), 1 = vertical(수직 이동)

    ObstacleKind kind; // 장애물 역할 종류 (linear / spinner / professor)
    int hp;            // 체력 (투사체에 맞으면 감소)
    int active;        // 1: 활성, 0: 비활성(죽은 상태)

    int world_x, world_y; // 서브픽셀 좌표
    int target_world_x, target_world_y;
    double move_speed;       // 초당 이동할 서브픽셀 (선형형에 사용)
    double move_accumulator; // 남은 이동량(서브픽셀)
    int moving;              // 이동 중 여부

    // 회전형(spinner) 장애물용 필드 (서브픽셀 좌표)
    int center_world_x, center_world_y; // 회전 중심 좌표
    int orbit_radius_world;             // 중심으로부터의 거리(서브픽셀)
    int angle_step;                     // 각도 변화 단위(몇 틱마다 한 칸 회전)
    int angle_index;                    // 현재 각도 인덱스(0,1,2,3 같은 식으로 사용 예정)

    // 교수님(Professor) 추적용 필드
    int alert;       // 0: 평상시/순찰, 1: 플레이어를 발견하고 추격 중
    int sight_range; // 시야 거리 (몇 칸까지 보는지)

    int p_state;    // 상태 관리 (예: 0=추격, 1=스킬준비, 2=패턴발동)
    double p_timer; // 패턴용 타이머 (예: 스킬 시전 시간, 쿨타임 등)
    int p_counter;  // 카운터 (예: 분신 소환 횟수, 피격 횟수 등)
    int p_misc;     // 여유분 변수 (필요하면 사용)

} Obstacle;

typedef struct
{
    int tile_x;
    int tile_y;
    double remaining_time; // 남은 생존 시간(초)
    int active;
} ProfessorClone;

// Stage 구조체
// - 한 스테이지에 대한 거의 모든 정보를 담는 구조체.
// - 스테이지 ID, 이름, 맵 데이터, 시작/목표 위치, 장애물 목록 등을 포함.
typedef struct
{
    int id;        // 스테이지 ID 번호 (1, 2, 3 ... 이런 식으로 구분)
    char name[32]; // 스테이지 이름

    char map[MAX_Y][MAX_X + 1];        // 실제 맵 데이터 (최대 크기)
    char render_map[MAX_Y][MAX_X + 1]; // 시각적 렌더링에 사용하는 별도 지층 (없으면 map을 복제해 사용)

    int start_x, start_y; // 시작 위치
    int goal_x, goal_y;   // 가방 위치
    int exit_x, exit_y;   // 목적지 위치

    int num_obstacles;                 // 장애물 개수
    Obstacle obstacles[MAX_OBSTACLES]; // 장애물 배열

    // 아이템 관련 필드
    int num_items;         // 아이템 개수
    Item items[MAX_ITEMS]; // 아이템 배열

    // 투사체 관련 필드
    int num_projectiles;                     // 사용 중인 투사체 개수 (혹은 전체 슬롯 수 관리용)
    Projectile projectiles[MAX_PROJECTILES]; // 투사체 배열

    int num_professor_clones;                     // 교수 분신 수 (활성 상태)
    ProfessorClone professor_clones[MAX_PROFESSOR_CLONES]; // 교수 분신 정보

    int width;  // 실제 사용 중인 맵 가로 길이
    int height; // 실제 사용 중인 맵 세로 길이

    double difficulty_player_speed; // 플레이어 이동 속도 (타일당 초)
    int remaining_ammo;             // 게임당 발사 가능한 투사체 수 제한

    int boss_exists;
    int boss_defeated;

} Stage;

// 벽 검사 함수
static inline int is_tile_opaque_char(char cell)
{
    return (cell == '#' || cell == '@');
}

static inline int is_tile_impassable_char(char cell)
{
    switch (cell)
    {
    case 'w':
    case 'W':
    case 'm':
    case 'M':
    case 'l':
    case 'L':
        return 1;
    default:
        return is_tile_opaque_char(cell);
    }
}

#endif // GAME_H
