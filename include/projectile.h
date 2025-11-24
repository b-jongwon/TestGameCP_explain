// projectile.h
// --------------------------------------------------------------
// 플레이어가 발사하는 투사체(Projectile)의 생성/이동을 담당하는 모듈.
//
// fire_projectile()  : 플레이어 기준 방향으로 투사체 생성
// move_projectiles() : 매 프레임마다 모든 투사체 이동 + 충돌 처리
// --------------------------------------------------------------

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "game.h"   // Stage, Player, Projectile 구조체 필요

// 플레이어가 바라보는 방향으로 투사체 하나 발사
void fire_projectile(Stage *stage, const Player *player);

// 모든 투사체 이동 처리 + 장애물 충돌 처리
void move_projectiles(Stage *stage);

#endif // PROJECTILE_H