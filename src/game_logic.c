#include "game.h"

int is_goal_reached(const Stage *stage, const Player *player) {
    return (player->x == stage->goal_x && player->y == stage->goal_y);
}

int check_collision(const Stage *stage, const Player *player) {
    for (int i = 0; i < stage->num_obstacles; i++) {
        const Obstacle *o = &stage->obstacles[i];
        if (o->x == player->x && o->y == player->y) {
            return 1;
        }
    }
    return 0;
}
