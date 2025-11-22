#include <pthread.h>    // pthread_t, pthread_create, pthread_join, pthread_mutex_t 등
#include <unistd.h>     // usleep

#include "obstacle.h"       // move_obstacles, start/stop 함수 선언 및 g_stage_mutex extern
#include "game.h"           // Stage, Obstacle, MAX_X, MAX_Y 등
#include "signal_handler.h" // g_running 전역 플래그


// 전역 뮤텍스 정의.
// - obstacle.h에 extern으로 선언되어 있고,
//   여기서 실제 메모리를 하나 만들어 초기화.
pthread_mutex_t g_stage_mutex = PTHREAD_MUTEX_INITIALIZER;

// 장애물 스레드에서 접근할 현재 스테이지 포인터.
// - start_obstacle_thread에서 설정.
// - obstacle_thread_func에서 사용.
static Stage *g_stage = NULL;

// 장애물 스레드 핸들
static pthread_t g_thread;

// 스레드가 실행 중인지 여부 플래그
static int g_thread_running = 0;


// ----------------------------------------------------------
// move_obstacles()
// ----------------------------------------------------------
// 스테이지 내의 모든 장애물을 한 번씩 이동시키는 함수.
// - 각 장애물의 type(0: 수평, 1: 수직)과 dir(+1 / -1)을 사용해 한 칸 이동.
// - 맵 경계나 벽('@')에 닿으면 방향을 반대로 뒤집는다.
void move_obstacles(Stage *stage) {

    // 등록된 모든 장애물 순회
    for (int i = 0; i < stage->num_obstacles; i++) {

        Obstacle *o = &stage->obstacles[i];

        // type == 0 → 가로(수평) 이동
        if (o->type == 0) {
            // 현재 방향(dir)에 따라 x 좌표 이동
            o->x += o->dir;

            // x가 왼쪽/오른쪽 경계를 벗어나거나, 해당 위치가 벽('@')이면
            if (o->x <= 1 || 
                o->x >= MAX_X - 2 || 
                stage->map[o->y][o->x] == '#') {

                // 방향을 반대로 뒤집고
                o->dir *= -1;

                // 반대 방향으로 한 칸 더 이동해 경계/벽 안쪽으로 되돌린다.
                o->x += o->dir;
            }
        }
        // type == 1 → 세로(수직) 이동
        else {
            // 현재 방향(dir)에 따라 y 좌표 이동
            o->y += o->dir;

            // y가 위/아래 경계를 벗어나거나, 해당 위치가 벽('@')이면
            if (o->y <= 1 || 
                o->y >= MAX_Y - 2 || 
                stage->map[o->y][o->x] == '#') {

                // 방향 뒤집기
                o->dir *= -1;

                // 반대 방향으로 한 칸 되돌리기
                o->y += o->dir;
            }
        }
    }
}


// ----------------------------------------------------------
// obstacle_thread_func()
// ----------------------------------------------------------
// 실제로 장애물을 주기적으로 움직이는 스레드 함수.
// - g_running(게임 전체 실행 플래그)와 g_thread_running이 둘 다 1인 동안 반복.
// - mutex로 g_stage를 보호하면서 move_obstacles 호출.
// - usleep(120ms)로 이동 간격(애니메이션 속도) 조절.
static void *obstacle_thread_func(void *arg) {
    (void)arg;   // 인자를 사용하지 않으므로 경고 방지용 캐스팅

    while (g_running && g_thread_running) {

        // 스테이지 데이터 보호를 위해 mutex lock
        pthread_mutex_lock(&g_stage_mutex);

        if (g_stage) {
            // 현재 스테이지에 대해 장애물 한 번씩 이동
            move_obstacles(g_stage);
        }

        // 임계구역 끝
        pthread_mutex_unlock(&g_stage_mutex);

        // 120ms 정도 대기 → 장애물 이동 속도 결정
        usleep(120000);
    }

    return NULL;  // 스레드 종료
}


// ----------------------------------------------------------
// start_obstacle_thread()
// ----------------------------------------------------------
// 장애물을 자동으로 움직이는 스레드 시작.
// - 인자 stage: 이 스레드가 다룰 현재 스테이지 포인터.
// - 성공 시 0, 실패 시 -1 반환.
int start_obstacle_thread(Stage *stage) {

    // 전역 포인터에 현재 스테이지 등록
    g_stage = stage;

    // 스레드 실행 플래그 ON
    g_thread_running = 1;

    // 스레드 생성: obstacle_thread_func를 실행
    if (pthread_create(&g_thread, NULL, obstacle_thread_func, NULL) != 0) {
        // 실패 시 플래그 되돌리고 에러 리턴
        g_thread_running = 0;
        return -1;
    }

    return 0; // 성공
}


// ----------------------------------------------------------
// stop_obstacle_thread()
// ----------------------------------------------------------
// 현재 실행 중인 장애물 스레드를 안전하게 종료시키는 함수.
// - g_thread_running 플래그를 0으로 바꿔 루프를 끝내고,
//   pthread_join으로 스레드가 완전히 종료될 때까지 기다린 뒤,
//   g_stage 포인터를 NULL로 리셋.
void stop_obstacle_thread(void) {

    // 스레드가 이미 안 돌고 있으면 할 일 없음
    if (!g_thread_running) return;

    // 루프를 빠져나오도록 플래그 OFF
    g_thread_running = 0;

    // 스레드가 종료될 때까지 대기 (리소스 정리)
    pthread_join(g_thread, NULL);

    // 현재 스테이지 참조 제거
    g_stage = NULL;
}
