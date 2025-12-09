#include <stdio.h>
#include "../include/game.h"
#include "../include/player.h"


int is_goal_reached(const Stage *stage, const Player *player) {
    if (!stage || !player) return 0;
    
     
    if (!player->has_backpack) return 0;  //가방검사

    // 6 스테이면 교수님 없어야 탈출가능
    if (stage->id == 6) {
        for (int i = 0; i < stage->num_obstacles; i++) {
            const Obstacle *o = &stage->obstacles[i];
            if (o->active && o->kind == OBSTACLE_KIND_PROFESSOR) {
                return 0; 
            }
        }
    }

    return is_tile_center_inside_player(player, stage->start_x, stage->start_y);
}


int check_collision(Stage *stage, Player *player) // 장애물 충돌 조건  return 1이면 사망 return 0이면 생존
{
    for (int i = 0; i < stage->num_obstacles; i++)
    {
        Obstacle *o = &stage->obstacles[i]; // 장애물 구조체 내부 배열에서 정보 초기화

        if (!o->active)
            continue;

        if (o->kind == OBSTACLE_KIND_BREAKABLE_WALL) // 꺠지는 벽은 충돌 제외
            continue;

        int cx = o->world_x + SUBPIXELS_PER_TILE / 2; // 장애물 좌표 초기화
        int cy = o->world_y + SUBPIXELS_PER_TILE / 2;

        if (!is_world_point_inside_player(player, cx, cy)) 
            continue;

        // 교수 충돌 (Stage 6 보스전 포함)

        if (o->kind == OBSTACLE_KIND_PROFESSOR) 
        {

            // Stage 6 : 쉴드 있어도 무조건 즉사 (보스전)
            if (stage->id == 6)
            {
                return 1; // 무조건 사망
            }

            // Stage 1~5 : 쉴드 있으면 방어 + 교수 제거

            if (player->shield_count > 0)
            {
                player->shield_count--;
                o->active = 0; // 교수 제거
                return 0;      // 플레이어 생존
            }

            return 1;
        }

        //   일반 장애물 충돌

        if (player->shield_count > 0)
        {
            player->shield_count--;
            o->active = 0; // 일반 장애물 제거
            return 0;
        }

        return 1; 
    }

    if (stage->num_professor_clones > 0)
    {
        const int tile_size = SUBPIXELS_PER_TILE;
        for (int i = 0; i < MAX_PROFESSOR_CLONES; ++i)
        {
            const ProfessorClone *clone = &stage->professor_clones[i];
            if (!clone->active)
            {
                continue;
            }

            int center_x = clone->tile_x * tile_size + tile_size / 2;
            int center_y = clone->tile_y * tile_size + tile_size / 2;
            if (is_world_point_inside_player(player, center_x, center_y))
            {
                return 1; // 분신 접촉: 즉시 게임오버 (쉴드 무시)
            }
        }
    }

    return 0;
}
