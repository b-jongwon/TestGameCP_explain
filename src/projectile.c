// projectile.c
#include "../include/game.h"
#include <stdio.h>

void fire_projectile(Stage *stage, const Player *player) {

    if (stage->num_projectiles >= MAX_PROJECTILES)
        return;

    Projectile *p = &stage->projectiles[stage->num_projectiles++];

    // 플레이어 위치에서 시작
    p->x = player->x;
    p->y = player->y;

    // 플레이어 바라보는 방향으로 설정
    switch (player->facing) {
        case PLAYER_FACING_UP:    p->dx = 0;  p->dy = -1; break;
        case PLAYER_FACING_DOWN:  p->dx = 0;  p->dy = 1;  break;
        case PLAYER_FACING_LEFT:  p->dx = -1; p->dy = 0;  break;
        case PLAYER_FACING_RIGHT: p->dx = 1;  p->dy = 0;  break;
        default:
    return; // 또는 break;
    }

    p->active = 1;
}

void move_projectiles(Stage *stage) {
    for (int i = 0; i < stage->num_projectiles; i++) {

        Projectile *p = &stage->projectiles[i];
        if (!p->active) continue;

        int nx = p->x + p->dx;
        int ny = p->y + p->dy;

        // 벽에 부딪히면 소멸
        char cell = stage->map[ny][nx];
        if (cell == '#' || cell == '@') {
            p->active = 0;
            continue;
        }

        // 장애물 충돌 체크
        for (int j = 0; j < stage->num_obstacles; j++) {
            Obstacle *o = &stage->obstacles[j];
            if (!o->active) continue;
            if (o->kind == OBSTACLE_KIND_PROFESSOR) continue; // 교수는 투사체 무시(다음 단계)

            if (o->x == nx && o->y == ny) {
                o->hp--;
                if (o->hp <= 0) {
                    o->active = 0;
                }
                p->active = 0;
                break;
            }
        }

        if (!p->active) continue;

        // 이동
        p->x = nx;
        p->y = ny;
    }
}