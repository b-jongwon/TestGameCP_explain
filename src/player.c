#include <math.h>
#include <stdlib.h>

#include "../include/player.h"
#include "../include/collision.h"

int g_player_anim_stride_pixels = 4;

static PlayerAnimPhase advance_anim_phase(PlayerAnimPhase phase)
{
    switch (phase)
    {
    case PLAYER_ANIM_PHASE_IDLE_A:
        return PLAYER_ANIM_PHASE_STEP_A;
    case PLAYER_ANIM_PHASE_STEP_A:
        return PLAYER_ANIM_PHASE_IDLE_B;
    case PLAYER_ANIM_PHASE_IDLE_B:
        return PLAYER_ANIM_PHASE_STEP_B;
    case PLAYER_ANIM_PHASE_STEP_B:
    default:
        return PLAYER_ANIM_PHASE_IDLE_A;
    }
}

static void set_idle_animation(Player *p)
{
    if (!p)
        return;
    p->anim_phase = PLAYER_ANIM_PHASE_IDLE_A;
    p->anim_pixel_progress = 0;
}

void init_player(Player *p, const Stage *stage) {
    p->world_x = stage->start_x * SUBPIXELS_PER_TILE;
    p->world_y = stage->start_y * SUBPIXELS_PER_TILE;
    p->target_world_x = p->world_x;
    p->target_world_y = p->world_y;

    // [수정] 하드코딩 값을 제거하고 스테이지 설정값을 사용
    // 만약 설정값이 0.0이면 안전장치로 0.18 사용
    double speed_setting = (stage->difficulty_player_speed > 0.0) ? stage->difficulty_player_speed : 0.18;
    p->base_move_speed = SUBPIXELS_PER_TILE / speed_setting;
    p->speed_multiplier = 1.0;
    p->move_speed = p->base_move_speed * p->speed_multiplier;
    
    p->move_accumulator = 0.0;
    p->moving = 0;
    p->alive = 1;
    p->has_backpack = 0;
    p->has_scooter = 0;
    p->scooter_expire_time = 0.0;
    p->facing = PLAYER_FACING_DOWN;
    p->anim_phase = PLAYER_ANIM_PHASE_IDLE_A;
    p->anim_pixel_progress = 0;
    p->is_moving = 0;
    p->last_move_time = 0.0;
    p->shield_count = 0; 
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
        set_idle_animation(p);
    }
}


static int tile_is_passable(const Stage *stage, int tile_x, int tile_y)
{
    if (!stage)
        return 0;

    int stage_width = (stage->width > 0) ? stage->width : MAX_X;
    int stage_height = (stage->height > 0) ? stage->height : MAX_Y;
    if (tile_x < 0 || tile_y < 0 || tile_x >= stage_width || tile_y >= stage_height)
        return 0;

    char cell = stage->map[tile_y][tile_x];
    return (cell != '#' && cell != '@');
}

static int count_front_free_pixels(const Player *p, const Stage *stage, int dir_x, int dir_y)
{
    if (!p || !stage)
        return 0;

    const int tile_size = SUBPIXELS_PER_TILE;
    const int stage_width = (stage->width > 0) ? stage->width : MAX_X;
    const int stage_height = (stage->height > 0) ? stage->height : MAX_Y;
    const int world_limit_x = stage_width * tile_size;
    const int world_limit_y = stage_height * tile_size;

    if (dir_x != 0)
    {
        int front_x = (dir_x > 0) ? p->world_x + tile_size : p->world_x - 1;
        if (front_x < 0 || front_x >= world_limit_x)
            return 0;

        int free_pixels = 0;
        for (int offset = 0; offset < tile_size; ++offset)
        {
            int sample_y = p->world_y + offset;
            if (sample_y < 0 || sample_y >= world_limit_y)
                continue;

            int tile_x = front_x / tile_size;
            int tile_y = sample_y / tile_size;
            if (tile_is_passable(stage, tile_x, tile_y))
            {
                free_pixels++;
            }
        }
        return free_pixels;
    }

    if (dir_y != 0)
    {
        int front_y = (dir_y > 0) ? p->world_y + tile_size : p->world_y - 1;
        if (front_y < 0 || front_y >= world_limit_y)
            return 0;

        int free_pixels = 0;
        for (int offset = 0; offset < tile_size; ++offset)
        {
            int sample_x = p->world_x + offset;
            if (sample_x < 0 || sample_x >= world_limit_x)
                continue;

            int tile_x = sample_x / tile_size;
            int tile_y = front_y / tile_size;
            if (tile_is_passable(stage, tile_x, tile_y))
            {
                free_pixels++;
            }
        }
        return free_pixels;
    }

    return 0;
}

static int direct_check(const Player *p, const Stage *stage, int dir_x, int dir_y)
{
    int space = count_front_free_pixels(p, stage, dir_x, dir_y);
    if (space == SUBPIXELS_PER_TILE)
        return 1;
    if (space == 0)
        return -1;
    return 0;
}

static int direction_from_input(char input, int *dir_x, int *dir_y, PlayerFacing *facing)
{
    if (!dir_x || !dir_y || !facing)
        return 0;

    switch (input)
    {
    case 'w':
    case 'W':
        *dir_x = 0;
        *dir_y = -1;
        *facing = PLAYER_FACING_UP;
        return 1;
    case 's':
    case 'S':
        *dir_x = 0;
        *dir_y = 1;
        *facing = PLAYER_FACING_DOWN;
        return 1;
    case 'a':
    case 'A':
        *dir_x = -1;
        *dir_y = 0;
        *facing = PLAYER_FACING_LEFT;
        return 1;
    case 'd':
    case 'D':
        *dir_x = 1;
        *dir_y = 0;
        *facing = PLAYER_FACING_RIGHT;
        return 1;
    default:
        return 0;
    }
}

static void start_movement(Player *p, int dir_x, int dir_y, PlayerFacing facing, double current_time)
{
    if (!p)
        return;

    const int step = PLAYER_MOVE_STEP_SUBPIXELS;
    if (!p->is_moving)
    {
        if (p->anim_phase != PLAYER_ANIM_PHASE_STEP_A &&
            p->anim_phase != PLAYER_ANIM_PHASE_STEP_B)
        {
            p->anim_phase = advance_anim_phase(p->anim_phase);
        }
        p->anim_pixel_progress = 0;
    }

    p->target_world_x = p->world_x + dir_x * step;
    p->target_world_y = p->world_y + dir_y * step;
    p->moving = 1;
    p->facing = facing;
    p->is_moving = 1;
    p->last_move_time = current_time;
}

static int edge_is_clear(const Player *p, const Stage *stage,
                         int dir_x, int dir_y, int edge_sign)
{
    if (!p || !stage)
        return 0;

    const int tile_size = SUBPIXELS_PER_TILE;
    const int stage_width = (stage->width > 0) ? stage->width : MAX_X;
    const int stage_height = (stage->height > 0) ? stage->height : MAX_Y;
    const int world_limit_x = stage_width * tile_size;
    const int world_limit_y = stage_height * tile_size;
    const int edge_span = 2;

    if (dir_x != 0)
    {
        int front_x = (dir_x > 0) ? p->world_x + tile_size : p->world_x - 1;
        if (front_x < 0 || front_x >= world_limit_x)
            return 0;

        int start_offset = (edge_sign < 0) ? 0 : (tile_size - edge_span);
        int clear_pixels = 0;
        for (int i = 0; i < edge_span; ++i)
        {
            int sample_y = p->world_y + start_offset + i;
            if (sample_y < 0 || sample_y >= world_limit_y)
                continue;

            int tile_x = front_x / tile_size;
            int tile_y = sample_y / tile_size;
            if (tile_is_passable(stage, tile_x, tile_y))
            {
                clear_pixels++;
            }
        }
        return (clear_pixels == edge_span);
    }
    else if (dir_y != 0)
    {
        int front_y = (dir_y > 0) ? p->world_y + tile_size : p->world_y - 1;
        if (front_y < 0 || front_y >= world_limit_y)
            return 0;

        int start_offset = (edge_sign < 0) ? 0 : (tile_size - edge_span);
        int clear_pixels = 0;
        for (int i = 0; i < edge_span; ++i)
        {
            int sample_x = p->world_x + start_offset + i;
            if (sample_x < 0 || sample_x >= world_limit_x)
                continue;

            int tile_x = sample_x / tile_size;
            int tile_y = front_y / tile_size;
            if (tile_is_passable(stage, tile_x, tile_y))
            {
                clear_pixels++;
            }
        }
        return (clear_pixels == edge_span);
    }

    return 0;
}

static int attempt_slide(Player *p, const Stage *stage, int dir_x, int dir_y, double current_time)
{
    if (!p || !stage)
        return 0;

    int slide_dir_x = 0;
    int slide_dir_y = 0;
    PlayerFacing slide_facing = p->facing;

    if (dir_x != 0)
    {
        int top_clear = edge_is_clear(p, stage, dir_x, 0, -1);
        int bottom_clear = edge_is_clear(p, stage, dir_x, 0, 1);

        if (top_clear == bottom_clear)
            return 0;

        if (top_clear)
        {
            slide_dir_y = -1;
            slide_facing = PLAYER_FACING_UP;
        }
        else
        {
            slide_dir_y = 1;
            slide_facing = PLAYER_FACING_DOWN;
        }
    }
    else if (dir_y != 0)
    {
        int left_clear = edge_is_clear(p, stage, 0, dir_y, -1);
        int right_clear = edge_is_clear(p, stage, 0, dir_y, 1);

        if (left_clear == right_clear)
            return 0;

        if (left_clear)
        {
            slide_dir_x = -1;
            slide_facing = PLAYER_FACING_LEFT;
        }
        else
        {
            slide_dir_x = 1;
            slide_facing = PLAYER_FACING_RIGHT;
        }
    }
    else
    {
        return 0;
    }

    if (direct_check(p, stage, slide_dir_x, slide_dir_y) != 1)
        return 0;

    start_movement(p, slide_dir_x, slide_dir_y, slide_facing, current_time);
    return 1;
}

void move_player(Player *p, char input, const Stage *stage, double current_time) {
    if (!p || !stage) return;

    if (p->moving) {
        return; // 현재 이동 중이면 새로운 명령 무시
    }

    int dir_x = 0;
    int dir_y = 0;
    PlayerFacing facing = p->facing;
    if (!direction_from_input(input, &dir_x, &dir_y, &facing))
    {
        update_player_idle(p, current_time);
        return;
    }

    int check = direct_check(p, stage, dir_x, dir_y);
    if (check == 1)
    {
        start_movement(p, dir_x, dir_y, facing, current_time);
        return;
    }

    if (check == 0)
    {
        if (attempt_slide(p, stage, dir_x, dir_y, current_time))
        {
            return;
        }
    }

    update_player_idle(p, current_time);
}

int update_player_motion(Player *p, double delta_time) {
    if (!p)
        return 0;

    if (!p->moving) {
        p->world_x = p->target_world_x;
        p->world_y = p->target_world_y;
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

    if (actual > 0 && p->is_moving)
    {
        int stride = g_player_anim_stride_pixels;
        if (stride <= 0)
        {
            stride = SUBPIXELS_PER_TILE;
        }
        p->anim_pixel_progress += actual;
        while (p->anim_pixel_progress >= stride)
        {
            p->anim_pixel_progress -= stride;
            p->anim_phase = advance_anim_phase(p->anim_phase);
        }
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
