#ifndef COLLISION_H
#define COLLISION_H

#include "game.h"

// 충돌 판정
// - 월드(서브픽셀) 좌표 기준
// - info: 겹친 방향/크기(밀어내기용)
typedef struct
{
    int overlap_top;
    int overlap_bottom;
    int overlap_left;
    int overlap_right;
} CollisionInfo;


int is_world_position_blocked(const Stage *stage, int world_x, int world_y, CollisionInfo *info);

int is_active_breakable_wall_at(const Stage *stage, int tx, int ty);

#endif // COLLISION_H
