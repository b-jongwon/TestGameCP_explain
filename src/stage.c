#include <stdio.h>
#include <string.h>

#include "stage.h"

int load_stage(Stage *stage, int stage_id) {
    memset(stage, 0, sizeof(Stage));
    stage->id = stage_id;

    if (stage_id == 1) {
        snprintf(stage->name, sizeof(stage->name), "Training Hall");
    } else if (stage_id == 2) {
        snprintf(stage->name, sizeof(stage->name), "Guarded Corridor");
    } else {
        snprintf(stage->name, sizeof(stage->name), "Final Escape");
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "assets/stage%d.map", stage_id);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    char line[256];
    int y = 0;
    while (y < MAX_Y && fgets(line, sizeof(line), fp)) {
        int len = (int)strlen(line);
        if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }
        for (int x = 0; x < MAX_X; x++) {
            char c = (x < len) ? line[x] : ' ';
            if (c == 'S') {
                stage->start_x = x;
                stage->start_y = y;
                stage->map[y][x] = ' ';
            } else if (c == 'G') {
                stage->goal_x = x;
                stage->goal_y = y;
                stage->map[y][x] = 'G';
            } else if (c == 'X') {
                if (stage->num_obstacles < MAX_OBSTACLES) {
                    Obstacle *o = &stage->obstacles[stage->num_obstacles++];
                    o->x = x;
                    o->y = y;
                    o->dir = 1;
                    o->type = (stage_id + x + y) % 2;
                }
                stage->map[y][x] = ' ';
            } else {
                stage->map[y][x] = c;
            }
        }
        stage->map[y][MAX_X] = '\0';
        y++;
    }

    for (; y < MAX_Y; y++) {
        for (int x = 0; x < MAX_X; x++) {
            stage->map[y][x] = ' ';
        }
        stage->map[y][MAX_X] = '\0';
    }

    fclose(fp);
    return 0;
}
