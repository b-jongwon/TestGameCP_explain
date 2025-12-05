// --------------------------------------------------------------
// stage.c
// --------------------------------------------------------------
// 이 파일은 스테이지(.map 파일)를 메모리로 로드하여
// Stage 구조체에 맵, 장애물, 플레이어 시작 위치 등을 채워넣는 기능을 담당.
//
// 이 함수는 아래 3개의 중요한 역할을 수행한다:
//
//   ① .map 파일의 실제 크기(가로 길이, 세로 줄 수)를 자동으로 측정한다.
//      -> stage->width, stage->height에 기록됨.
//   ② 파일에 등장하는 문자에 따라 start(S), goal(G), obstacle(X)을 Stage에 저장한다.
//   ③ stage->map[y][x]에 맵 문자들을 채우되,
//        - 플레이어는 ' ' 로 바꾸고 위치만 저장
//        - 장애물은 ' ' 로 바꾸고 struct Obstacle에 따로 저장
//
// 이 방식 덕분에 render()는 단순히 맵을 그린 뒤 장애물/플레이어만 덮어씌우면 된다.
//

#include <stdio.h>  // fopen, fgets, FILE, perror
#include <string.h> // memset, strlen, snprintf, strncpy

// ⚠️ 너가 절대 경로 include를 쓰는 이유는 아마 빌드 include 경로 문제 때문.
//    지금은 일단 유지하되, 나중엔 -I 옵션으로 바꾸는 게 좋음.
#include "../include/game.h"
#include "../include/stage.h"

typedef struct
{
    const char *filename;
    const char *name;
} StageFileInfo;

static const StageFileInfo kStageFiles[] = {
    {"b1.map", "B1"},
    {"1f.map", "1F"},
    {"2f.map", "2F"},
    {"3f.map", "3F"},
    {"4f.map", "4F"},
    {"5f.map", "5F"}};

typedef struct
{
    double player_sec_per_tile; // 플레이어가 한 타일 가는 데 걸리는 시간 (작을수록 빠름)
    double obs_sec_per_tile;    // 일반 장애물(V,H, R) 이동 속도
    double prof_sec_per_tile;   // 교수님(P) 이동 속도
    int obs_hp;                 // 장애물 체력
    int prof_sight;             // 교수님 시야 범위 (타일 수)
    int initial_ammo;           // 게임당 발사 가능 투사체 수
    double spinner_speed;       // 회전 속도 클수록 빠름
    int spinner_radius;         // 회전 반지름
} StageDifficulty;

static const StageDifficulty kDifficultySettings[] = {
    {
        0.0,
        0.0,
        0.0,
        0,
        0,
        0,
        0.0,
        0,
    }, // 0번 인덱스 (사용 안 함)

    // ★속도 관련 숫자는 작으면 빠름 (회전속도 제외).
    // Stage 1:
    /* {
       1. 플레이어 속도
       2.일반 장애물 속도    구조체 순서대로 설정하면 됩니다.
       3. 교수님 속도
       4. 장애물 체력         발사체 사거리는 game.h 에서  CONSTANT_PROJECTILE_RANGE 수정. , 투사체 증가 갯수는 game.h 에서 SUPPLY_REFILL_AMOUNT 수정
       5. 교수님 시야범위
       6. 게임당 투사체 수
       7. 스핀 속도
       8. 스핀 반지름}*/
    {0.20, 0.25, 0.35, 2, 8, 10, 0.05, 2},

    // Stage 2:
    {0.14, 0.20, 0.20, 3, 8, 7, 0.15, 2},

    // Stage 3:
    {0.16, 0.20, 0.22, 4, 12, 7, 0.15, 3},

    // Stage 4:
    {0.14, 0.15, 0.18, 5, 15, 7, 0.15, 3},

    // Stage 5:
    {0.12, 0.12, 0.3, 5, 8, 7, 0.15, 3},

    // Stage 6
    {0.12, 0.12, 0.3, 6, 20, 30, 0.1, 3}};

int get_stage_count(void)
{
    return (int)(sizeof(kStageFiles) / sizeof(kStageFiles[0]));
}

// --------------------------------------------------------------
// load_stage()
// --------------------------------------------------------------
// 입력:
//   - Stage *stage : 스테이지 정보를 저장할 구조체
//   - int stage_id : 스테이지 번호 (파일 이름 생성에 사용)
//
// 출력:
//   - 성공하면 0
//   - 실패하면 -1
//
// 작동 원리:
//   [1] Stage 구조체를 0으로 초기화
//   [2] 스테이지 순서 테이블에서 파일명을 찾아온다
//   [3] 파일 열기
//   [4] 파일의 각 줄을 읽으며:
//         - 가장 긴 줄 길이(max_width)를 측정
//         - S/G/X 등을 판단하여 stage 구조체에 기록
//         - 일반 문자(벽/빈공간)는 stage->map에 직접 저장
//   [5] stage->width = 가장 긴 줄 길이
//       stage->height = 읽은 줄 수
//   [6] 읽은 줄 아래는 모두 공백으로 초기화
//
// 결론: Stage 구조체가 해당 스테이지의 모든 정보를 갖게 된다.
//
int load_stage(Stage *stage, int stage_id)
{

    if (!stage)
    {
        return -1;
    }

    if (stage_id < 1 || stage_id > get_stage_count())
    {
        fprintf(stderr, "Invalid stage id: %d\n", stage_id);
        return -1;
    }

    const StageFileInfo *info = &kStageFiles[stage_id - 1];

    // 난이도 설정 가져오기 (범위 초과 시 기본값 사용 방지 로직 필요하면 추가)
    StageDifficulty diff = kDifficultySettings[1]; // 기본값
    if (stage_id < (int)(sizeof(kDifficultySettings) / sizeof(kDifficultySettings[0])))
    {
        diff = kDifficultySettings[stage_id];
    }

    // ----------------------------------------------------------
    // 1) Stage 구조체 전체 초기화
    // ----------------------------------------------------------
    memset(stage, 0, sizeof(Stage)); // memset쓰면 구조체 변수들 0으로 초기화 됩니다.

    stage->id = stage_id; // stage id 인자로 받고 구조체에 저장.

    // [설정 적용 1] 스테이지 전역 설정 저장 (플레이어/투사체용)
    stage->difficulty_player_speed = diff.player_sec_per_tile;
    stage->remaining_ammo = diff.initial_ammo;

    // ----------------------------------------------------------
    // 2) 스테이지 파일 이름 생성
    //    예: stage_id=1 → "assets/b1.map"
    // ----------------------------------------------------------
    char filename[64];
    snprintf(filename, sizeof(filename), "assets/%s", info->filename);
    strncpy(stage->name, info->name, sizeof(stage->name) - 1);
    stage->name[sizeof(stage->name) - 1] = '\0';
    // main 에서 stage_id는 계속 갱신

    // ----------------------------------------------------------
    // 3) 파일 열기 (읽기 모드)
    // ----------------------------------------------------------
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        perror("fopen"); // 왜 실패했는지 시스템 메시지 출력
        return -1;
    }

    char line[1024];   // 한 줄을 임시로 저장하는 버퍼
    int y = 0;         // 현재 맵의 y 위치
    int max_width = 0; // 가장 긴 줄의 길이를 저장

    // ----------------------------------------------------------
    // 4) 파일을 한 줄씩 읽으면서 맵을 채움
    // ----------------------------------------------------------
    while (y < MAX_Y && fgets(line, sizeof(line), fp))
    { // MAX_y는 game.h에 정의됨.

        int len = (int)strlen(line);

        // 줄 끝의 개행문자 제거
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            line[--len] = '\0';
        }

        // 가장 긴 줄 길이 추적
        if (len > max_width)
        {
            max_width = len;
        }

        // ------------------------------------------------------
        // 현재 줄(line) 데이터를 x=0~MAX_X-1까지 스캔하며
        // Stage.map[y][x] 채우기
        // ------------------------------------------------------
        for (int x = 0; x < MAX_X; x++)
        {

            // 파일의 현재 줄에 글자가 없다면 공백 취급
            char c = (x < len) ? line[x] : ' ';

            if (c == 'S')
            {
                // 플레이어 시작 위치
                stage->start_x = x;
                stage->start_y = y;

                // 맵에는 플레이어를 그리지 않음 → 빈 공간
                stage->map[y][x] = ' ';
            }
            else if (c == 'G')
            {
                // 가방 위치
                stage->goal_x = x;
                stage->goal_y = y;

                // 맵에는 실제로 'G' 표시 남겨 사용
                stage->map[y][x] = ' ';
            }
            else if (c == 'F')
            {
                stage->exit_x = x;
                stage->exit_y = y;
                stage->map[y][x] = ' ';
            }

            else if (c == 'V' || c == 'H' || c == 'P' || c == 'R' || c == 'B')
            {
                if (stage->num_obstacles < MAX_OBSTACLES)
                {
                    Obstacle *o = &stage->obstacles[stage->num_obstacles++];

                    o->world_x = x * SUBPIXELS_PER_TILE;
                    o->world_y = y * SUBPIXELS_PER_TILE;
                    o->target_world_x = o->world_x;
                    o->target_world_y = o->world_y;
                    o->move_accumulator = 0.0;
                    o->moving = 0;
                    o->dir = 1;
                    o->type = (stage_id + x + y) % 2;
                    o->active = 1;

                    // [설정 적용 2] 오브젝트 종류별 난이도 적용
                    if (c == 'P')
                    { // 교수님
                        o->kind = OBSTACLE_KIND_PROFESSOR;
                        o->sight_range = diff.prof_sight;
                        o->move_speed = SUBPIXELS_PER_TILE / diff.prof_sec_per_tile;
                        o->alert = 0;
                        if (stage_id == 6)
                        {
                            o->hp = 15; // 6 stage 보스 hp
                        }
                        else
                        {
                            o->hp = 999; // 나머지 스테이지 무적
                        }
                    }
                    else if (c == 'R')
                    { // 스피너
                        o->kind = OBSTACLE_KIND_SPINNER;
                        o->center_world_x = x * SUBPIXELS_PER_TILE;
                        o->center_world_y = y * SUBPIXELS_PER_TILE;
                        // 반지름= 타일 수 * 타일당 픽셀
                        o->orbit_radius_world = diff.spinner_radius * SUBPIXELS_PER_TILE;
                        // 속도= Obstacle 구조체의 move_speed 변수를 회전 속도로 활용
                        o->move_speed = diff.spinner_speed;
                        o->angle_index = 0;
                        o->world_x = o->center_world_x + o->orbit_radius_world;
                        o->world_y = o->center_world_y;
                    }
                    else if (c == 'V')
                    {
                        o->kind = OBSTACLE_KIND_LINEAR;
                        o->type = 1; // 1 = 세로 이동 고정
                        o->move_speed = SUBPIXELS_PER_TILE / diff.obs_sec_per_tile;
                        o->hp = diff.obs_hp;
                    }

                    else if (c == 'H')
                    {
                        o->kind = OBSTACLE_KIND_LINEAR;
                        o->type = 0; // 0 = 가로 이동 고정
                        o->move_speed = SUBPIXELS_PER_TILE / diff.obs_sec_per_tile;
                        o->hp = diff.obs_hp;
                    }
                    else if (c == 'B')

                    {
                        o->kind = OBSTACLE_KIND_BREAKABLE_WALL;
                        o->move_speed = 0.0; // 움직임 없음
                        o->hp = 3;
                        o->dir = 0;
                    }
                }
                stage->map[y][x] = ' ';
            }
            else if (c == 'I' || c == 'E' || c == 'A')
            {
                // 아이템 생성
                if (stage->num_items < MAX_ITEMS)
                {
                    Item *it = &stage->items[stage->num_items++];
                    it->world_x = x * SUBPIXELS_PER_TILE;
                    it->world_y = y * SUBPIXELS_PER_TILE;

                    //
                    if (c == 'I')
                    {
                        it->type = ITEM_TYPE_SHIELD;
                    }
                    else if (c == 'E')
                    {
                        it->type = ITEM_TYPE_SCOOTER;
                    }
                    else
                    {
                        it->type = ITEM_TYPE_SUPPLY;
                    }

                    it->active = 1;
                }
                // 맵에는 공백 처리
                stage->map[y][x] = ' ';
            }
            else
            {
                // '@', '#', ' ' 등 일반 문자는 그대로 기록
                stage->map[y][x] = c;
            }
        }

        stage->map[y][MAX_X] = '\0'; // 문자열 종단자 추가
        y++;
    }

    // ----------------------------------------------------------
    // 5) 자동으로 실제 맵 크기 기록
    // ----------------------------------------------------------
    stage->height = y;        // 총 몇 줄을 읽었는가?
    stage->width = max_width; // 가장 긴 줄의 길이

    // ----------------------------------------------------------
    // 6) 남은 줄은 공백으로 초기화
    // ----------------------------------------------------------
    for (; y < MAX_Y; y++)
    {
        for (int x = 0; x < MAX_X; x++)
        {
            stage->map[y][x] = ' ';
        }
        stage->map[y][MAX_X] = '\0';
    }

    fclose(fp);
    return 0;
}
