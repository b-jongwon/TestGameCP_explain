#ifndef PLAYER_H
#define PLAYER_H

#include "../include/game.h"   
#include "../include/stage.h"

// 플레이어 이동
// - move_player: 목표 좌표 설정
// - update_player_motion: 실제 이동 처리
void init_player(Player *p, const Stage *stage);

void move_player(Player *p, char input, const Stage *stage, double current_time);

void update_player_idle(Player *p, double current_time);

int update_player_motion(Player *p, double delta_time);

int is_world_point_inside_player(const Player *player, int world_x, int world_y);

int is_tile_center_inside_player(const Player *player, int tile_x, int tile_y);

extern int g_player_anim_stride_pixels;

#endif // PLAYER_H
