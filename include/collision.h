#ifndef COLLISION_H
#define COLLISION_H

#include "game.h"

typedef struct
{
    int overlap_top;
    int overlap_bottom;
    int overlap_left;
    int overlap_right;
} CollisionInfo;

int is_world_position_blocked(const Stage *stage, int world_x, int world_y, CollisionInfo *info);

#endif // COLLISION_H
