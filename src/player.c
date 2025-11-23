#include "player.h"

void init_player(Player *p, const Stage *stage) {
    p->x = stage->start_x;
    p->y = stage->start_y;
    p->alive = 1;
    p->has_backpack = 0;
    p->facing = PLAYER_FACING_DOWN;
    p->anim_step = 0;
    p->is_moving = 0;
    p->last_move_time = 0.0;
}

static int is_wall(char cell) {
    return (cell == '#' || cell == '@');
}

void update_player_idle(Player *p, double current_time) {
    if (!p) return;

    if (p->is_moving && current_time - p->last_move_time >= 0.5) {
        p->is_moving = 0;
        p->anim_step = 0;
    }
}

void move_player(Player *p, char input, const Stage *stage, double current_time) {
    if (!p || !stage) return;

    int nx = p->x;
    int ny = p->y;
    PlayerFacing new_facing = p->facing;

    switch (input) {
        case 'w':
        case 'W':
            ny--;
            new_facing = PLAYER_FACING_UP;
            break;
        case 's':
        case 'S':
            ny++;
            new_facing = PLAYER_FACING_DOWN;
            break;
        case 'a':
        case 'A':
            nx--;
            new_facing = PLAYER_FACING_LEFT;
            break;
        case 'd':
        case 'D':
            nx++;
            new_facing = PLAYER_FACING_RIGHT;
            break;
        default:
            update_player_idle(p, current_time);
            return;
    }

    if (nx < 0 || nx >= MAX_X || ny < 0 || ny >= MAX_Y) {
        update_player_idle(p, current_time);
        return;
    }

    char cell = stage->map[ny][nx];
    if (is_wall(cell)) {
        update_player_idle(p, current_time);
        return;
    }

    if (nx == p->x && ny == p->y) {
        update_player_idle(p, current_time);
        return;
    }

    if (p->facing == new_facing && p->is_moving) {
        p->anim_step ^= 1;
    } else {
        p->anim_step = 0;
    }

    p->x = nx;
    p->y = ny;
    p->facing = new_facing;
    p->is_moving = 1;
    p->last_move_time = current_time;
}
