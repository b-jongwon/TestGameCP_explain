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

// 플레이어를 입력에 따라 이동시키는 함수.
// - 인자 p: 현재 플레이어 상태.
// - 인자 input: 사용자가 누른 키. 예: 'w', 'a', 's', 'd', 'q' 등.
// - 인자 stage: 맵 정보와 장애물, 벽 등을 담고 있는 현재 스테이지.
// - 내부에서 할 일 예시:
//   1) 입력에 따라 임시로 새 좌표(nx, ny)를 계산.
//   2) 그 위치가 맵 범위 내인지 검사.
//   3) 벽(예: '#')이면 이동 취소.
//   4) 장애물 위치와 겹치면 플레이어 사망 처리(p->alive = 0).
//   5) 골 지점과 겹치면 클리어 플래그 설정 등.
void move_player(Player *p, char input, const Stage *stage);

#endif // PLAYER_H
