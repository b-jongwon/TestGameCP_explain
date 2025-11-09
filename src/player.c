#include "player.h"

void init_player(Player *p, const Stage *stage) {
    p->x = stage->start_x;
    p->y = stage->start_y;
    p->alive = 1;
}

void move_player(Player *p, char input, const Stage *stage) {
    int nx = p->x;
    int ny = p->y;

    if (input == 'w' || input == 'W') ny--;
    else if (input == 's' || input == 'S') ny++;
    else if (input == 'a' || input == 'A') nx--;
    else if (input == 'd' || input == 'D') nx++;

    if (nx < 0 || nx >= MAX_X || ny < 0 || ny >= MAX_Y)
        return;

    char cell = stage->map[ny][nx];
    if (cell == '@')
        return;

    p->x = nx;
    p->y = ny;
}
