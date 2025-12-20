#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../include/obstacle.h"
#include "../include/game.h"
#include "../include/signal_handler.h"
#include "../include/collision.h"
#include "../include/professor_pattern.h"

typedef struct
{
    int x, y;      // 타일 좌표
    int first_dir; // 출발지에서 처음 움직였던 방향 (1:우, 2:좌, 3:하, 4:상)
} Node;
#define QUEUE_SIZE (MAX_X * MAX_Y)

pthread_mutex_t g_stage_mutex = PTHREAD_MUTEX_INITIALIZER;

// 장애물 스레드에서 접근할 현재 스테이지 포인터.
static Stage *g_stage = NULL;

// 장애물 스레드 핸들
static pthread_t g_thread;

// 스레드가 실행 중인지 여부 플래그
static int g_thread_running = 0;

// 장애물 스레드가 플레이어 위치를 참고하기 위한 포인터
static Player *g_player_ref = NULL;

// 메인에서 플레이어 주소를 넘겨받는 함수 (obstacle.h에도 선언 필요)
void set_obstacle_player_ref(Player *p)
{
    g_player_ref = p;
}

static int try_move_obstacle(Obstacle *o, Stage *stage, int delta_world_x, int delta_world_y);

// 스피너 이동 로직
static void update_spinner(Obstacle *o, Stage *stage)
{
    // 1. 각도 증가
    double speed = o->move_speed;
    double current_angle = o->angle_index * speed;

    // 2. 새로운 위치 계산 (중심점 + 반지름 * 삼각비)
    int nx_world = o->center_world_x + (int)round(o->orbit_radius_world * cos(current_angle));
    int ny_world = o->center_world_y + (int)round(o->orbit_radius_world * sin(current_angle));

    if (!is_world_position_blocked(stage, nx_world, ny_world, NULL))
    {
        o->world_x = nx_world;
        o->world_y = ny_world;
    }

    o->angle_index++;
}

// Bfs 로 교수님 로직 변경
static int get_next_step_bfs(Stage *stage, int start_tx, int start_ty, int target_tx, int target_ty)
{
    // 이미 목표에 도착했으면 정지
    if (start_tx == target_tx && start_ty == target_ty)
        return 0;

    static char visited[MAX_Y][MAX_X];
    memset(visited, 0, sizeof(visited));

    static Node queue[QUEUE_SIZE];
    int front = 0;
    int rear = 0;

    visited[start_ty][start_tx] = 1;
    queue[rear++] = (Node){start_tx, start_ty, 0};

    int dx[4] = {1, -1, 0, 0};
    int dy[4] = {0, 0, 1, -1};
    int dir_code[4] = {1, 2, 3, 4};

    while (front < rear)
    {
        Node cur = queue[front++];

        if (cur.x == target_tx && cur.y == target_ty)
        {
            return cur.first_dir;
        }

        for (int i = 0; i < 4; i++)
        {
            int nx = cur.x + dx[i];
            int ny = cur.y + dy[i];

            if (nx < 0 || nx >= MAX_X || ny < 0 || ny >= MAX_Y)
                continue;

            if (visited[ny][nx])
                continue;

            if (is_tile_impassable_char(stage->map[ny][nx]))
                continue;

            visited[ny][nx] = 1;

            int next_first_dir = (cur.first_dir == 0) ? dir_code[i] : cur.first_dir;
            queue[rear++] = (Node){nx, ny, next_first_dir};

            if (rear >= QUEUE_SIZE)
                return 0;
        }
    }
    return 0;
}

//  교수님 AI (추격) 로직
static void update_professor(Obstacle *o, Stage *stage, double delta_time)
{
    if (!g_player_ref)
        return;

    if (o->alert == 0)
    {
        int dx = g_player_ref->world_x - o->world_x;
        int dy = g_player_ref->world_y - o->world_y;
        double dist = (fabs((double)dx) + fabs((double)dy)) / (double)SUBPIXELS_PER_TILE;
        if (dist <= o->sight_range)
            o->alert = 1;
    }
    // 패턴 함수 호출
    // 패턴 함수가 0을 반환하면(스킬 시전 등) should_move가 0이 되어 이동을 건너뛰게 됨.
    int should_move = update_professor_pattern(stage, o, g_player_ref, delta_time);

    if (should_move) // 패턴 함수가 1을 반환해야 이동 가능
    {
        if (delta_time < 0.0)
            delta_time = 0.0;
        o->move_accumulator += o->move_speed * delta_time;
        int pixels_to_move = (int)floor(o->move_accumulator);

        if (pixels_to_move <= 0)
            return;
        if (pixels_to_move > SUBPIXELS_PER_TILE)
            pixels_to_move = SUBPIXELS_PER_TILE;
        o->move_accumulator -= pixels_to_move;

        const int TILE = SUBPIXELS_PER_TILE;

        for (int step = 0; step < pixels_to_move; ++step)
        {
            if (o->alert)
            {

                int center_x = o->world_x + TILE / 2;
                int center_y = o->world_y + TILE / 2;

                int cur_tx = center_x / TILE;
                int cur_ty = center_y / TILE;

                int target_tx = (g_player_ref->world_x + TILE / 2) / TILE;
                int target_ty = (g_player_ref->world_y + TILE / 2) / TILE;

                int next_dir = get_next_step_bfs(stage, cur_tx, cur_ty, target_tx, target_ty);

                int dx = 0;
                int dy = 0;

                int tile_center_world_x = cur_tx * TILE;
                int tile_center_world_y = cur_ty * TILE;

                int align_power = 1;

                if (next_dir == 1)
                {
                    dx = 1;

                    if (o->world_y > tile_center_world_y)
                        dy = -align_power;
                    else if (o->world_y < tile_center_world_y)
                        dy = align_power;
                }
                else if (next_dir == 2)
                {
                    dx = -1;

                    if (o->world_y > tile_center_world_y)
                        dy = -align_power;
                    else if (o->world_y < tile_center_world_y)
                        dy = align_power;
                }
                else if (next_dir == 3)
                {
                    dy = 1;

                    if (o->world_x > tile_center_world_x)
                        dx = -align_power;
                    else if (o->world_x < tile_center_world_x)
                        dx = align_power;
                }
                else if (next_dir == 4)
                {
                    dy = -1;

                    if (o->world_x > tile_center_world_x)
                        dx = -align_power;
                    else if (o->world_x < tile_center_world_x)
                        dx = align_power;
                }

                if (dx != 0 || dy != 0)
                {

                    if (try_move_obstacle(o, stage, dx, dy))
                    {
                    }

                    else
                    {
                        int main_dx = (next_dir == 1) ? 1 : ((next_dir == 2) ? -1 : 0);
                        int main_dy = (next_dir == 3) ? 1 : ((next_dir == 4) ? -1 : 0);

                        if (!try_move_obstacle(o, stage, main_dx, main_dy))
                        {

                            int align_dx = dx - main_dx;
                            int align_dy = dy - main_dy;
                            try_move_obstacle(o, stage, align_dx, align_dy);
                        }
                    }
                }
            }
            else
            {

                int dir = (o->dir == 0) ? 1 : o->dir;
                if (o->type == 0)
                {
                    if (!try_move_obstacle(o, stage, dir, 0))
                    {
                        o->dir = -dir;
                        try_move_obstacle(o, stage, o->dir, 0);
                    }
                }
                else
                {
                    if (!try_move_obstacle(o, stage, 0, dir))
                    {
                        o->dir = -dir;
                        try_move_obstacle(o, stage, 0, o->dir);
                    }
                }
            }
        }
    }
}

// 스테이지 내의 모든 장애물을 한 번씩 이동시키는 함수.
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

        case OBSTACLE_KIND_BREAKABLE_WALL:
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

// 실제로 장애물을 주기적으로 움직이는 스레드 함수.
static void *obstacle_thread_func(void *arg)
{
    (void)arg;

    struct timespec prev_ts;
    clock_gettime(CLOCK_MONOTONIC, &prev_ts);

    while (g_running && g_thread_running)
    {
        struct timespec now_ts;
        clock_gettime(CLOCK_MONOTONIC, &now_ts);
        double delta_time = (now_ts.tv_sec - prev_ts.tv_sec) +
                            (now_ts.tv_nsec - prev_ts.tv_nsec) / 1e9;
        prev_ts = now_ts;

        pthread_mutex_lock(&g_stage_mutex);

        if (g_stage)
        {

            move_obstacles(g_stage, delta_time);
        }

        pthread_mutex_unlock(&g_stage_mutex);

        usleep(20000);
    }

    return NULL;
}

// 함정 충돌 체크 (메인 스레드에서 호출)
int check_trap_collision(const Stage *stage, const Player *player)
{
    if (!stage || !player)
        return 0;

    // 플레이어의 정중앙 좌표 계산
    int center_x = player->world_x + SUBPIXELS_PER_TILE / 2;
    int center_y = player->world_y + SUBPIXELS_PER_TILE / 2;

    // 타일 좌표로 변환
    int tile_x = center_x / SUBPIXELS_PER_TILE;
    int tile_y = center_y / SUBPIXELS_PER_TILE;

    if (tile_x < 0 || tile_x >= stage->width || tile_y < 0 || tile_y >= stage->height)
    {
        return 0;
    }

    if (stage->map[tile_y][tile_x] == 'T')
    {
        return 1;
    }

    return 0;
}

// start_obstacle_thread()
int start_obstacle_thread(Stage *stage)
{

    // 전역 포인터에 현재 스테이지 등록
    g_stage = stage;

    g_thread_running = 1;

    if (pthread_create(&g_thread, NULL, obstacle_thread_func, NULL) != 0)
    {

        g_thread_running = 0;
        return -1;
    }

    return 0;
}

// 현재 실행 중인 장애물 스레드를 안전하게 종료시키는 함수.
void stop_obstacle_thread(void)
{

    if (!g_thread_running)
        return;

    g_thread_running = 0;

    pthread_join(g_thread, NULL);

    g_stage = NULL;
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
    return 1;
}
