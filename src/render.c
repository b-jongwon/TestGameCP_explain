#include <stdio.h>    // printf, putchar
#include <string.h>   // strncpy

#include "render.h"   // render 함수 선언, Stage/Player, MAX_X, MAX_Y
#include "obstacle.h" // Obstacle 타입 사용 (배열 읽어오기용)


// ----------------------------------------------------------
// render()
// ----------------------------------------------------------
// 현재 스테이지, 장애물, 플레이어 위치를 기반으로 화면을 그려주는 함수.
// - stage->map을 로컬 buffer로 복사한 뒤,
//   장애물(X), 골(G), 플레이어(P)를 덮어씌운 다음 출력.
// - elapsed_time, 현재/전체 스테이지 정보도 상단에 표시.
void render(const Stage *stage, const Player *player, double elapsed_time,
            int current_stage, int total_stages) {

    // 화면에 출력할 임시 버퍼.
    // - 원본 stage->map을 직접 수정하지 않고, 이 buffer에만 그림.
    char buffer[MAX_Y][MAX_X + 1];

    // 1) 맵 복사: stage->map 내용을 buffer로 그대로 복사
    for (int y = 0; y < MAX_Y; y++) {
        // 각 줄을 최대 MAX_X 문자까지 복사
        strncpy(buffer[y], stage->map[y], MAX_X);
        // 마지막에 널 문자 추가해서 문자열로 만듦
        buffer[y][MAX_X] = '\0';
    }

    // 2) 장애물 표시: 장애물 위치에 'X' 문자로 표시
    for (int i = 0; i < stage->num_obstacles; i++) {
        Obstacle o = stage->obstacles[i];

        // 화면 범위 안에 있는 장애물만 표시
        if (o.x >= 0 && o.x < MAX_X &&
            o.y >= 0 && o.y < MAX_Y) {

            buffer[o.y][o.x] = 'X';
        }
    }

    // 3) 골 위치 표시: goal_x, goal_y에 'G' 문자
    if (stage->goal_x >= 0 && stage->goal_y >= 0) {
        buffer[stage->goal_y][stage->goal_x] = 'G';
    }

    // 4) 플레이어 표시: 플레이어 좌표에 'P' 문자
    if (player->x >= 0 && player->x < MAX_X &&
        player->y >= 0 && player->y < MAX_Y) {

        buffer[player->y][player->x] = 'P';
    }

    // 5) 상단 정보 출력: 게임 제목, 현재 스테이지, 시간
    printf("=== Stealth Adventure ===  Stage %d/%d   Time: %.2fs\n",
           current_stage, total_stages, elapsed_time);

    // 6) 버퍼 내용을 화면에 줄 단위로 출력
    for (int y = 0; y < MAX_Y; y++) {
        for (int x = 0; x < MAX_X; x++) {
            putchar(buffer[y][x]);
        }
        putchar('\n');
    }

    // 7) 하단 안내 메시지
    printf("Controls: W/A/S/D to move, q to quit. Avoid X and reach G.\n");
}
