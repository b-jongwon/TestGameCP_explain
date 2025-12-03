// projectile.c
#include "../include/game.h"
#include <stdio.h>



void fire_projectile(Stage *stage, const Player *player)
{
    if (!stage || !player) return;

    // ============================================================
    // 1. [심플함] 남은 탄약만 검사!
    // ============================================================
    // 화면에 몇 개가 있든 상관없음. 그냥 내 주머니에 총알이 있냐 없냐만 중요.
    
    if (stage->remaining_ammo <= 0) {
        // 탄약 없음 -> 발사 불가
        return; 
    }

    // ============================================================
    // 2. 빈 슬롯 찾기 (메모리 관리용)
    // ============================================================
    int slot_index = -1;
    
    // 빈 자리(active==0) 찾기
    for (int i = 0; i < stage->num_projectiles; i++) {
        if (stage->projectiles[i].active == 0) {
            slot_index = i;
            break;
        }
    }

    // 빈 자리 없으면 배열 늘리기
    if (slot_index == -1) {
        if (stage->num_projectiles < MAX_PROJECTILES) {
            slot_index = stage->num_projectiles++;
        } else {
            return; // 64발 슬롯이 꽉 참 (거의 일어날 일 없음)
        }
    }

    // ============================================================
    // 3. 발사 및 차감
    // ============================================================
    
    // 탄약 1발 소모
    stage->remaining_ammo--;
    // printf("탕! 남은 탄약: %d\n", stage->remaining_ammo); // 디버깅용

    int dir_x = 0, dir_y = 0;
    switch (player->facing) {
        case PLAYER_FACING_UP:    dir_y = -1; break;
        case PLAYER_FACING_DOWN:  dir_y = 1; break;
        case PLAYER_FACING_LEFT:  dir_x = -1; break;
        case PLAYER_FACING_RIGHT: dir_x = 1; break;
        default: return; 
    }

    Projectile *p = &stage->projectiles[slot_index];
    p->world_x = player->world_x;
    p->world_y = player->world_y;
    p->dir_x = dir_x;
    p->dir_y = dir_y;
    p->active = 1;
    p->distance_traveled = 0;
}

static int is_wall_cell(const Stage *stage, int tile_x, int tile_y)
{
    if (!stage)
        return 1;
    if (tile_x < 0 || tile_y < 0)
        return 1;

    int stage_width = (stage->width > 0) ? stage->width : MAX_X;
    int stage_height = (stage->height > 0) ? stage->height : MAX_Y;
    if (tile_x >= stage_width || tile_y >= stage_height)
        return 1;

    char cell = stage->map[tile_y][tile_x];
    return (cell == '#' || cell == '@');
}

void move_projectiles(Stage *stage)
{
    if (!stage)
        return;

    const int step = SUBPIXELS_PER_TILE;
    int max_range_pixels = CONSTANT_PROJECTILE_RANGE * SUBPIXELS_PER_TILE;

    for (int i = 0; i < stage->num_projectiles; i++)
    {
        Projectile *p = &stage->projectiles[i];
        if (!p->active)
            continue;

        int next_world_x = p->world_x + p->dir_x * step;
        int next_world_y = p->world_y + p->dir_y * step;

        p->distance_traveled += step; // 이동 거리 누적
        if (p->distance_traveled >= max_range_pixels)
        {
            p->active = 0; // 사거리 초과 -> 소멸
            continue;
        }

        int next_tile_x = next_world_x / SUBPIXELS_PER_TILE;
        int next_tile_y = next_world_y / SUBPIXELS_PER_TILE;
        if (is_wall_cell(stage, next_tile_x, next_tile_y))
        {
            p->active = 0;
            continue;
        }

        // 장애물 충돌 체크
        for (int j = 0; j < stage->num_obstacles; j++)
        {
            Obstacle *o = &stage->obstacles[j];
            if (!o->active)
                continue;
            if (o->kind == OBSTACLE_KIND_PROFESSOR)
                continue;

            int obstacle_tile_x = o->world_x / SUBPIXELS_PER_TILE;
            int obstacle_tile_y = o->world_y / SUBPIXELS_PER_TILE;
            if (obstacle_tile_x == next_tile_x && obstacle_tile_y == next_tile_y)
            {
                o->hp--;
                if (o->hp <= 0)
                {
                    o->active = 0;
                }
                p->active = 0;
                break;
            }
        }

        if (!p->active)
            continue;

        p->world_x = next_world_x;
        p->world_y = next_world_y;
    }
}
