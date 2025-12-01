// obstacle.h
// -------------------------------------------------
// 스테이지 안의 장애물들을 움직이는 로직과, 그 로직을 별도의 스레드로 돌릴지에 대한 관리 기능 제공.
// - 멀티스레드로 장애물만 계속 움직이게 하거나,
//   단일 스레드 테스트 환경에서 move_obstacles만 직접 호출할 수도 있도록 설계.

#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <pthread.h>    // pthread_t, pthread_mutex_t 등 POSIX 스레드 관련 타입 사용
#include "../include/game.h"       // Stage, Obstacle 구조체 정의를 사용하기 위해 include

// 전역으로 선언된 뮤텍스(mutex).
// - g_stage_mutex는 Stage 구조체에 대한 동시 접근을 보호하기 위해 사용.
// - 장애물 스레드와 메인 스레드가 동시에 stage->obstacles, stage->map 등에 접근할 수 있기 때문에,
//   해당 코드 구간을 pthread_mutex_lock / unlock으로 감싸서 데이터 레이스를 방지.
extern pthread_mutex_t g_stage_mutex;

// [추가] 장애물 시스템에 플레이어 참조를 설정하는 함수
void set_obstacle_player_ref(const Player *p);

// 장애물 이동을 담당하는 스레드를 시작하는 함수.
// - 인자 stage: 현재 플레이 중인 Stage의 포인터.
// - 내부에서 pthread_create를 통해 장애물 전용 스레드를 생성하고,
//   그 스레드가 while(g_running) 같은 루프 안에서 move_obstacles를 주기적으로 호출하도록 구현할 수 있음.
// - 성공 시 0, 실패 시 음수 리턴 등의 형태로 구현 가능.
int start_obstacle_thread(Stage *stage);

// 장애물 스레드를 정지 시키는 함수.
// - 내부에서 전역 플래그를 끄거나, pthread_cancel / pthread_join 등을 호출하여
//   장애물 스레드가 안전하게 종료되도록 처리.
// - 게임 종료, 스테이지 전환 시 호출.
void stop_obstacle_thread(void);

// 실제 장애물들을 한 번씩 움직이는 함수.
// - stage->obstacles 배열을 순회하며 각 장애물의 위치를 dir, type에 따라 한 칸씩 이동.
// - 맵 경계에 부딪치면 dir을 반대로 바꾸거나, 특정 규칙에 따라 튕기도록 구현.
// - 원래는 스레드 안에서 주기적으로 호출되지만,
//   단일 스레드 환경에서 테스트할 때 직접 호출하도록 "공개"해둔 함수.
void move_obstacles(Stage *stage, double delta_time); // also exposed for single-threaded test

#endif // OBSTACLE_H
