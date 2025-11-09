#include <stdio.h>
#include <string.h>

#include "render.h"
#include "obstacle.h"

void render(const Stage *stage, const Player *player, double elapsed_time, int current_stage, int total_stages) {
    char buffer[MAX_Y][MAX_X + 1];

    for (int y = 0; y < MAX_Y; y++) {
        strncpy(buffer[y], stage->map[y], MAX_X);
        buffer[y][MAX_X] = '\0';
    }

    for (int i = 0; i < stage->num_obstacles; i++) {
        Obstacle o = stage->obstacles[i];
        if (o.x >= 0 && o.x < MAX_X && o.y >= 0 && o.y < MAX_Y) {
            buffer[o.y][o.x] = 'X';
        }
    }

    if (stage->goal_x >= 0 && stage->goal_y >= 0) {
        buffer[stage->goal_y][stage->goal_x] = 'G';
    }

    if (player->x >= 0 && player->x < MAX_X && player->y >= 0 && player->y < MAX_Y) {
        buffer[player->y][player->x] = 'P';
    }

    printf("=== Stealth Adventure ===  Stage %d/%d   Time: %.2fs\n", current_stage, total_stages, elapsed_time);

    for (int y = 0; y < MAX_Y; y++) {
        for (int x = 0; x < MAX_X; x++) {
            putchar(buffer[y][x]);
        }
        putchar('\n');
    }
    printf("Controls: W/A/S/D to move, q to quit. Avoid X and reach G.\n");
}
