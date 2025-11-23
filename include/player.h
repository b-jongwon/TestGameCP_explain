// player.h
// ----------------------------------------------------
// 플레이어 캐릭터를 초기화하고, 입력에 따라 움직이게 하는 인터페이스.
// - init_player: 스테이지의 시작 위치로 플레이어를 세팅.
// - move_player: 입력 문자(w, a, s, d 등)에 따라 플레이어 위치를 갱신하고,
//                벽/장애물/골 지점과의 상호작용을 처리.

#ifndef PLAYER_H
#define PLAYER_H

#include "game.h"   // Player, Stage 구조체 정의 사용
#include "stage.h"  // stage 관련 로드 함수와 Stage 타입 사용 (중복 include 보호는 stage.h 쪽에서 처리)

// 플레이어를 특정 스테이지의 시작 위치로 초기화하는 함수.
// - 인자 p: 초기화할 Player 구조체 포인터.
// - 인자 stage: 현재 플레이할 Stage. stage->start_x, start_y를 참고하여 p->x, p->y를 설정.
// - 또한 p->alive = 1로 설정하여 플레이어를 "살아있는 상태"로 만듦.
void init_player(Player *p, const Stage *stage);

// 현재 시간을 함께 받아 플레이어를 이동.
void move_player(Player *p, char input, const Stage *stage, double current_time);

// 입력이 없을 때 호출해 일정 시간 이후 스탠드 자세로 전환.
void update_player_idle(Player *p, double current_time);

#endif // PLAYER_H
