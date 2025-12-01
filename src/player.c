#include <math.h>
#include <stdlib.h>

#include "../include/player.h"
#include "../include/collision.h"

void init_player(Player *p, const Stage *stage) {
    p->x = stage->start_x;
    p->y = stage->start_y;
    p->world_x = stage->start_x * SUBPIXELS_PER_TILE;
    p->world_y = stage->start_y * SUBPIXELS_PER_TILE;
    p->target_world_x = p->world_x;
    p->target_world_y = p->world_y;
    p->move_speed = SUBPIXELS_PER_TILE / 0.18; // 약 0.18초에 한 타일 이동
    p->move_accumulator = 0.0;
    p->moving = 0;
    p->alive = 1;
    p->has_backpack = 0;
    p->facing = PLAYER_FACING_DOWN;
    p->anim_step = 0;
    p->is_moving = 0;
    p->last_move_time = 0.0;
    p->shield_count = 0; // 아이템 초기값 추가.
}

static int is_wall(char cell) {
    return (cell == '#' || cell == '@');
}

void update_player_idle(Player *p, double current_time) {
    if (!p) return;

    if (p->moving)
        return;

    if (p->is_moving && current_time - p->last_move_time >= 0.5) {
        p->is_moving = 0;
        p->anim_step = 0;
    }
}


static int apply_slide_correction(Player *p,
                                  const Stage *stage,
                                  int delta_world_x,
                                  int delta_world_y,
                                  int blocked_world_x,
                                  int blocked_world_y,
                                  const CollisionInfo *info,
                                  double current_time)
{
    if (!p || !stage || !info)
        return 0;

    const int tile_size = SUBPIXELS_PER_TILE;
    const int max_offset = tile_size / 2;
    const int stage_width = (stage->width > 0) ? stage->width : MAX_X;
    const int stage_height = (stage->height > 0) ? stage->height : MAX_Y;
    const int world_limit_x = stage_width * tile_size;
    const int world_limit_y = stage_height * tile_size;

    if (delta_world_x != 0)
    {
        int preferred = 0;
        if (info->overlap_top > 0)
            preferred = 1;
        if (info->overlap_bottom > 0)
        {
            if (preferred == 0 || info->overlap_bottom < info->overlap_top)
                preferred = -1;
        }
        if (preferred == 0)
        {
            preferred = ((p->world_y % tile_size) < max_offset) ? 1 : -1;
        }

        int dirs[2] = {preferred, -preferred};
        for (int d = 0; d < 2; ++d)
        {
            int dir = dirs[d];
            if (dir == 0)
                continue;

            for (int offset = 1; offset <= max_offset; ++offset)
            {
                int candidate_y = p->world_y + dir * offset;
                if (candidate_y < 0 || candidate_y + tile_size > world_limit_y)
                    break;

                if (is_world_position_blocked(stage, p->world_x, candidate_y, NULL))
                    continue;

                p->target_world_x = p->world_x;
                p->target_world_y = candidate_y;
                p->moving = 1;
                p->facing = (dir > 0) ? PLAYER_FACING_DOWN : PLAYER_FACING_UP;
                p->is_moving = 1;
                p->last_move_time = current_time;
                return 1;
            }
        }
    }
    else if (delta_world_y != 0)
    {
        int preferred = 0;
        if (info->overlap_left > 0)
            preferred = 1;
        if (info->overlap_right > 0)
        {
            if (preferred == 0 || info->overlap_right < info->overlap_left)
                preferred = -1;
        }
        if (preferred == 0)
        {
            preferred = ((p->world_x % tile_size) < max_offset) ? 1 : -1;
        }

        int dirs[2] = {preferred, -preferred};
        for (int d = 0; d < 2; ++d)
        {
            int dir = dirs[d];
            if (dir == 0)
                continue;

            for (int offset = 1; offset <= max_offset; ++offset)
            {
                int candidate_x = p->world_x + dir * offset;
                if (candidate_x < 0 || candidate_x + tile_size > world_limit_x)
                    break;

                if (is_world_position_blocked(stage, candidate_x, p->world_y, NULL))
                    continue;

                p->target_world_x = candidate_x;
                p->target_world_y = p->world_y;
                p->moving = 1;
                p->facing = (dir > 0) ? PLAYER_FACING_RIGHT : PLAYER_FACING_LEFT;
                p->is_moving = 1;
                p->last_move_time = current_time;
                return 1;
            }
        }
    }

    return 0;
}

void move_player(Player *p, char input, const Stage *stage, double current_time) {
    if (!p || !stage) return;

    if (p->moving) {
        return; // 현재 이동 중이면 새로운 명령 무시
    }

    const int step = PLAYER_MOVE_STEP_SUBPIXELS; // 서브픽셀 단위 이동
    int delta_world_x = 0;
    int delta_world_y = 0;
    PlayerFacing new_facing = p->facing;

    switch (input) {
        case 'w':
        case 'W':
            delta_world_y = -step;
            new_facing = PLAYER_FACING_UP;
            break;
        case 's':
        case 'S':
            delta_world_y = step;
            new_facing = PLAYER_FACING_DOWN;
            break;
        case 'a':
        case 'A':
            delta_world_x = -step;
            new_facing = PLAYER_FACING_LEFT;
            break;
        case 'd':
        case 'D':
            delta_world_x = step;
            new_facing = PLAYER_FACING_RIGHT;
            break;
        default:
            update_player_idle(p, current_time);
            return;
    }

    if (delta_world_x == 0 && delta_world_y == 0) {
        update_player_idle(p, current_time);
        return;
    }

    int new_world_x = p->world_x + delta_world_x;
    int new_world_y = p->world_y + delta_world_y;

    CollisionInfo info;
    if (is_world_position_blocked(stage, new_world_x, new_world_y, &info))
    {
        if (apply_slide_correction(p, stage, delta_world_x, delta_world_y,
                                   new_world_x, new_world_y, &info, current_time))
        {
            return;
        }

        update_player_idle(p, current_time);
        return;
    }

    if (p->facing == new_facing && p->is_moving) {
        p->anim_step ^= 1;
    } else {
        p->anim_step = 0;
    }

    p->target_world_x = new_world_x;
    p->target_world_y = new_world_y;
    p->moving = 1;

    p->facing = new_facing;
    p->is_moving = 1;
    p->last_move_time = current_time;
}

int update_player_motion(Player *p, double delta_time) {
    if (!p)
        return 0;

    if (!p->moving) {
        p->world_x = p->target_world_x;
        p->world_y = p->target_world_y;
        p->x = p->world_x / SUBPIXELS_PER_TILE;
        p->y = p->world_y / SUBPIXELS_PER_TILE;
        p->move_accumulator = 0.0;
        return 0;
    }

    double distance = p->move_speed * delta_time;
    if (distance < 0.0)
    {
        distance = 0.0;
    }
    p->move_accumulator += distance;
    int step = (int)floor(p->move_accumulator);
    if (step <= 0)
    {
        return 0;
    }
    p->move_accumulator -= step;

    int diff_x = abs(p->target_world_x - p->world_x);
    int diff_y = abs(p->target_world_y - p->world_y);
    int remaining = diff_x + diff_y;
    if (remaining <= 0)
    {
        p->moving = 0;
        p->move_accumulator = 0.0;
        p->x = p->world_x / SUBPIXELS_PER_TILE;
        p->y = p->world_y / SUBPIXELS_PER_TILE;
        return 1;
    }

    int actual = (step > remaining) ? remaining : step;
    if (p->target_world_x != p->world_x)
    {
        int dir = (p->target_world_x > p->world_x) ? 1 : -1;
        p->world_x += dir * actual;
    }
    else
    {
        int dir = (p->target_world_y > p->world_y) ? 1 : -1;
        p->world_y += dir * actual;
    }

    int finished = 0;
    if (actual == remaining)
    {
        p->moving = 0;
        p->move_accumulator = 0.0;
        finished = 1;
    }
    else
    {
        p->move_accumulator += (step - actual);
    }

    p->x = p->world_x / SUBPIXELS_PER_TILE;
    p->y = p->world_y / SUBPIXELS_PER_TILE;
    return finished;
}

int is_world_point_inside_player(const Player *player, int world_x, int world_y)
{
    if (!player)
        return 0;

    const int size = SUBPIXELS_PER_TILE;
    return (world_x >= player->world_x &&
            world_x < player->world_x + size &&
            world_y >= player->world_y &&
            world_y < player->world_y + size);
}

int is_tile_center_inside_player(const Player *player, int tile_x, int tile_y)
{
    if (!player)
        return 0;

    const int size = SUBPIXELS_PER_TILE;
    int center_x = tile_x * size + size / 2;
    int center_y = tile_y * size + size / 2;

    return is_world_point_inside_player(player, center_x, center_y);
}
