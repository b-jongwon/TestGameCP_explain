#include <pthread.h> // pthread_t, pthread_create, pthread_join, pthread_mutex_t 등
#include <unistd.h>  // usleep
#include <math.h>
#include <stdlib.h>

#include "../include/obstacle.h"       // move_obstacles, start/stop 함수 선언 및 g_stage_mutex extern
#include "../include/game.h"           // Stage, Obstacle, MAX_X, MAX_Y 등
#include "../include/signal_handler.h" // g_running 전역 플래그
#include "../include/collision.h"

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

// [추가] 장애물 스레드가 플레이어 위치를 참고하기 위한 포인터
static const Player *g_player_ref = NULL;

// [추가] 메인에서 플레이어 주소를 넘겨받는 함수 (obstacle.h에도 선언 필요)
void set_obstacle_player_ref(const Player *p)
{
    g_player_ref = p;
}

static void sync_obstacle_tile_position(Obstacle *o);
static int try_move_obstacle(Obstacle *o, Stage *stage, int delta_world_x, int delta_world_y);

// ----------------------------------------------------------
// 내부 헬퍼 함수 1: 스피너 이동 로직
// ----------------------------------------------------------
static void update_spinner(Obstacle *o, Stage *stage)
{
    // 1. 각도 증가 (속도 조절: 0.2 라디안씩 회전)
    // angle_index를 실수형 각도 누적값처럼 활용하거나, 별도 float 필드를 쓸 수도 있지만
    // 여기서는 간단히 angle_index를 각도로 환산한다고 가정.
    double speed = 0.2;
    double current_angle = o->angle_index * speed;

    // 2. 새로운 위치 계산 (중심점 + 반지름 * 삼각비)
    int nx = o->center_x + (int)round(o->radius * cos(current_angle));
    int ny = o->center_y + (int)round(o->radius * sin(current_angle));

    // 3. 맵 경계 체크 (혹시 모를 에러 방지)
    if (nx > 0 && nx < MAX_X && ny > 0 && ny < MAX_Y)
    {
        // 벽이 아닐 때만 이동 (스피너는 벽을 뚫을지 말지 선택 가능, 여기선 안 뚫음)
        if (stage->map[ny][nx] != '#')
        {
            o->x = nx;
            o->y = ny;
            o->world_x = nx * SUBPIXELS_PER_TILE;
            o->world_y = ny * SUBPIXELS_PER_TILE;
        }
    }

    // 4. 다음 프레임을 위해 인덱스 증가
    o->angle_index++;
}

// ----------------------------------------------------------
// 내부 헬퍼 함수 2: 교수님 AI (추격) 로직
// ----------------------------------------------------------
static void update_professor(Obstacle *o, Stage *stage, double delta_time)
{
    if (!g_player_ref)
        return; // 플레이어 정보 없으면 아무것도 안 함

    // 1. 플레이어와의 거리 계산 (맨해튼 거리)
    int dx = g_player_ref->x - o->x;
    int dy = g_player_ref->y - o->y;
    int dist = abs(dx) + abs(dy);

    // 2. 시야 체크 및 상태 변경
    if (dist <= o->sight_range)
    {
        o->alert = 1; // 발견!
    }
    else
    {
        o->alert = 0; // 놓침 (또는 원래대로)
    }

    if (delta_time < 0.0)
        delta_time = 0.0;

    o->move_accumulator += o->move_speed * delta_time;
    int pixels_to_move = (int)floor(o->move_accumulator);
    if (pixels_to_move <= 0)
        return;
    if (pixels_to_move > SUBPIXELS_PER_TILE)
        pixels_to_move = SUBPIXELS_PER_TILE;
    o->move_accumulator -= pixels_to_move;

    const int tile_size = SUBPIXELS_PER_TILE;
    for (int step = 0; step < pixels_to_move; ++step)
    {
        if (o->alert)
        {
            int player_center_x = g_player_ref->world_x + tile_size / 2;
            int player_center_y = g_player_ref->world_y + tile_size / 2;
            int obstacle_center_x = o->world_x + tile_size / 2;
            int obstacle_center_y = o->world_y + tile_size / 2;

            int diff_x = player_center_x - obstacle_center_x;
            int diff_y = player_center_y - obstacle_center_y;
            int step_x = 0;
            int step_y = 0;

            if (abs(diff_x) >= abs(diff_y) && diff_x != 0)
            {
                step_x = (diff_x > 0) ? 1 : -1;
            }
            else if (diff_y != 0)
            {
                step_y = (diff_y > 0) ? 1 : -1;
            }

            if (step_x != 0)
            {
                if (!try_move_obstacle(o, stage, step_x, 0))
                {
                    if (diff_y != 0)
                    {
                        step_y = (diff_y > 0) ? 1 : -1;
                        if (!try_move_obstacle(o, stage, 0, step_y))
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else if (step_y != 0)
            {
                if (!try_move_obstacle(o, stage, 0, step_y))
                {
                    if (diff_x != 0)
                    {
                        step_x = (diff_x > 0) ? 1 : -1;
                        if (!try_move_obstacle(o, stage, step_x, 0))
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                break; // 이미 같은 위치
            }
        }
        else
        {
            int dir = (o->dir == 0) ? 1 : o->dir;
            int moved = 0;
            if (o->type == 0)
            {
                moved = try_move_obstacle(o, stage, dir, 0);
            }
            else
            {
                moved = try_move_obstacle(o, stage, 0, dir);
            }

            if (!moved)
            {
                o->dir *= -1;
                dir = (o->dir == 0) ? 1 : o->dir;
                if (o->type == 0)
                {
                    if (!try_move_obstacle(o, stage, dir, 0))
                        break;
                }
                else
                {
                    if (!try_move_obstacle(o, stage, 0, dir))
                        break;
                }
            }
        }
    }
}
// ----------------------------------------------------------
// move_obstacles()
// ----------------------------------------------------------
// 스테이지 내의 모든 장애물을 한 번씩 이동시키는 함수.
// - 각 장애물의 type(0: 수평, 1: 수직)과 dir(+1 / -1)을 사용해 한 칸 이동.
// - 맵 경계나 벽('@')에 닿으면 방향을 반대로 뒤집는다.
void move_obstacles(Stage *stage, double delta_time)
{
    if (delta_time < 0.0)
        delta_time = 0.0;

    for (int i = 0; i < stage->num_obstacles; i++)
    {
        Obstacle *o = &stage->obstacles[i];
        if (!o->active)
            continue;

        switch (o->kind)
        {
        case OBSTACLE_KIND_SPINNER:
            update_spinner(o, stage);
            break;

        case OBSTACLE_KIND_PROFESSOR:
            update_professor(o, stage, delta_time);
            break;

        case OBSTACLE_KIND_LINEAR:
        default:
            o->move_accumulator += o->move_speed * delta_time;
            int step_pixels = (int)floor(o->move_accumulator);
            if (step_pixels <= 0)
                break;

            o->move_accumulator -= step_pixels;
            int total_moved = 0;
            const int max_step = SUBPIXELS_PER_TILE;
            while (total_moved < step_pixels)
            {
                int move = o->dir;
                if (move == 0)
                    move = 1;
                move *= 1;

                int applied = 0;
                if (o->type == 0)
                {
                    applied = try_move_obstacle(o, stage, move, 0);
                }
                else
                {
                    applied = try_move_obstacle(o, stage, 0, move);
                }

                if (!applied)
                {
                    o->dir *= -1;
                    if (o->type == 0)
                    {
                        if (!try_move_obstacle(o, stage, o->dir, 0))
                        {
                            break;
                        }
                    }
                    else
                    {
                        if (!try_move_obstacle(o, stage, 0, o->dir))
                        {
                            break;
                        }
                    }
                }

                total_moved += (applied ? 1 : 0);
                if (total_moved >= max_step)
                    break;
            }
            break;
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
static void *obstacle_thread_func(void *arg)
{
    (void)arg; // 인자를 사용하지 않으므로 경고 방지용 캐스팅

    struct timespec prev_ts;
    clock_gettime(CLOCK_MONOTONIC, &prev_ts);

    while (g_running && g_thread_running)
    {
        struct timespec now_ts;
        clock_gettime(CLOCK_MONOTONIC, &now_ts);
        double delta_time = (now_ts.tv_sec - prev_ts.tv_sec) +
                            (now_ts.tv_nsec - prev_ts.tv_nsec) / 1e9;
        prev_ts = now_ts;

        // 스테이지 데이터 보호를 위해 mutex lock
        pthread_mutex_lock(&g_stage_mutex);

        if (g_stage)
        {
            // 현재 스테이지에 대해 장애물 한 번씩 이동
            move_obstacles(g_stage, delta_time);
        }

        // 임계구역 끝
        pthread_mutex_unlock(&g_stage_mutex);

        // 120ms 정도 대기 → 장애물 이동 속도 결정
        usleep(20000);
    }

    return NULL; // 스레드 종료
}

// ----------------------------------------------------------
// start_obstacle_thread()
// ----------------------------------------------------------
// 장애물을 자동으로 움직이는 스레드 시작.
// - 인자 stage: 이 스레드가 다룰 현재 스테이지 포인터.
// - 성공 시 0, 실패 시 -1 반환.
int start_obstacle_thread(Stage *stage)
{

    // 전역 포인터에 현재 스테이지 등록
    g_stage = stage;

    // 스레드 실행 플래그 ON
    g_thread_running = 1;

    // 스레드 생성: obstacle_thread_func를 실행
    if (pthread_create(&g_thread, NULL, obstacle_thread_func, NULL) != 0)
    {
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
void stop_obstacle_thread(void)
{

    // 스레드가 이미 안 돌고 있으면 할 일 없음
    if (!g_thread_running)
        return;

    // 루프를 빠져나오도록 플래그 OFF
    g_thread_running = 0;

    // 스레드가 종료될 때까지 대기 (리소스 정리)
    pthread_join(g_thread, NULL);

    // 현재 스테이지 참조 제거
    g_stage = NULL;
}
static void sync_obstacle_tile_position(Obstacle *o)
{
    if (!o)
        return;
    o->x = o->world_x / SUBPIXELS_PER_TILE;
    o->y = o->world_y / SUBPIXELS_PER_TILE;
}

static int try_move_obstacle(Obstacle *o, Stage *stage, int delta_world_x, int delta_world_y)
{
    if (!o || !stage)
        return 0;

    int new_world_x = o->world_x + delta_world_x;
    int new_world_y = o->world_y + delta_world_y;
    if (is_world_position_blocked(stage, new_world_x, new_world_y, NULL))
    {
        return 0;
    }

    o->world_x = new_world_x;
    o->world_y = new_world_y;
    sync_obstacle_tile_position(o);
    return 1;
}
