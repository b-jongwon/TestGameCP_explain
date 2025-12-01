// projectile.c
#include "../include/game.h"
#include <stdio.h>

void fire_projectile(Stage *stage, const Player *player)
{

    if (!stage || !player)
        return;

    int dir_x = 0;
    int dir_y = 0;
    switch (player->facing)
    {
    case PLAYER_FACING_UP:
        dir_y = -1;
        break;
    case PLAYER_FACING_DOWN:
        dir_y = 1;
        break;
    case PLAYER_FACING_LEFT:
        dir_x = -1;
        break;
    case PLAYER_FACING_RIGHT:
        dir_x = 1;
        break;
    default:
        return;
    }

    if (stage->num_projectiles >= MAX_PROJECTILES)
        return;

    Projectile *p = &stage->projectiles[stage->num_projectiles++];

    p->world_x = player->world_x;
    p->world_y = player->world_y;
    p->dir_x = dir_x;
    p->dir_y = dir_y;

    p->active = 1;
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

    for (int i = 0; i < stage->num_projectiles; i++)
    {
        Projectile *p = &stage->projectiles[i];
        if (!p->active)
            continue;

        int next_world_x = p->world_x + p->dir_x * step;
        int next_world_y = p->world_y + p->dir_y * step;

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
