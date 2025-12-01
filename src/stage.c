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

    // ----------------------------------------------------------
    // 1) Stage 구조체 전체 초기화
    // ----------------------------------------------------------
    memset(stage, 0, sizeof(Stage)); // memset쓰면 구조체 변수들 0으로 초기화 됩니다.

    stage->id = stage_id; // stage id 인자로 받고 구조체에 저장.

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
                // 골 위치
                stage->goal_x = x;
                stage->goal_y = y;

                // 맵에는 실제로 'G' 표시 남겨 사용
                stage->map[y][x] = 'G';
            }
            else if (c == 'X' || c == 'P' || c == 'R')
            {
                if (stage->num_obstacles < MAX_OBSTACLES)
                {
                    Obstacle *o = &stage->obstacles[stage->num_obstacles++];

                    o->world_x = x * SUBPIXELS_PER_TILE;
                    o->world_y = y * SUBPIXELS_PER_TILE;
                    o->target_world_x = o->world_x;
                    o->target_world_y = o->world_y;
                    o->move_speed = SUBPIXELS_PER_TILE / 0.25;
                    o->move_accumulator = 0.0;
                    o->moving = 0;
                    o->dir = 1;
                    o->type = (stage_id + x + y) % 2; // 이동 방향(가로/세로) 랜덤성 부여
                    o->hp = 3;
                    o->active = 1;

                    // --- 🔥 여기가 핵심: 문자에 따라 종류(kind) 결정 ---
                    if (c == 'P')
                    {
                        o->kind = OBSTACLE_KIND_PROFESSOR;
                        o->sight_range = 8; // 교수님은 시야가 넓음 (8칸)
                        o->alert = 0;
                    }
                    else if (c == 'R')
                    {
                        o->kind = OBSTACLE_KIND_SPINNER;
                        o->center_world_x = x * SUBPIXELS_PER_TILE;
                        o->center_world_y = y * SUBPIXELS_PER_TILE;
                        o->orbit_radius_world = 4 * SUBPIXELS_PER_TILE;
                        o->angle_index = 0; // 0도부터 시작

                        o->world_x = o->center_world_x + o->orbit_radius_world;
                        o->world_y = o->center_world_y;
                        o->target_world_x = o->world_x;
                        o->target_world_y = o->world_y;
                    }

                    else
                    {
                        // 'X' 인 경우
                        o->kind = OBSTACLE_KIND_LINEAR;
                    }
                }
                stage->map[y][x] = ' '; // 맵 상에서는 지워서 이동 가능 공간으로 만듦
            }
            else if (c == 'I')
            {
                // 아이템 생성
                if (stage->num_items < MAX_ITEMS)
                {
                    Item *it = &stage->items[stage->num_items++];
                    it->world_x = x * SUBPIXELS_PER_TILE;
                    it->world_y = y * SUBPIXELS_PER_TILE;
                    it->type = ITEM_TYPE_SHIELD;
                    it->active = 1;
                }
                // 맵에는 아이템 표시 대신 공간
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
