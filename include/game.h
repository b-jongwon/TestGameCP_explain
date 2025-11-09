#ifndef GAME_H
#define GAME_H

#define MAX_X 60
#define MAX_Y 20
#define MAX_OBSTACLES 64

typedef struct {
    int x, y;
    int alive;
} Player;

typedef struct {
    int x, y;
    int dir;   // +1 or -1
    int type;  // 0 = horizontal, 1 = vertical
} Obstacle;

typedef struct {
    int id;
    char name[32];
    char map[MAX_Y][MAX_X + 1];
    int start_x, start_y;
    int goal_x, goal_y;
    int num_obstacles;
    Obstacle obstacles[MAX_OBSTACLES];
} Stage;

#endif
