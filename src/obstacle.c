#include <pthread.h>
#include <unistd.h>

#include "obstacle.h"
#include "game.h"
#include "signal_handler.h"

pthread_mutex_t g_stage_mutex = PTHREAD_MUTEX_INITIALIZER;

static Stage *g_stage = NULL;
static pthread_t g_thread;
static int g_thread_running = 0;

void move_obstacles(Stage *stage) {
    for (int i = 0; i < stage->num_obstacles; i++) {
        Obstacle *o = &stage->obstacles[i];
        if (o->type == 0) {
            o->x += o->dir;
            if (o->x <= 1 || o->x >= MAX_X - 2 || stage->map[o->y][o->x] == '@') {
                o->dir *= -1;
                o->x += o->dir;
            }
        } else {
            o->y += o->dir;
            if (o->y <= 1 || o->y >= MAX_Y - 2 || stage->map[o->y][o->x] == '@') {
                o->dir *= -1;
                o->y += o->dir;
            }
        }
    }
}

static void *obstacle_thread_func(void *arg) {
    (void)arg;
    while (g_running && g_thread_running) {
        pthread_mutex_lock(&g_stage_mutex);
        if (g_stage) {
            move_obstacles(g_stage);
        }
        pthread_mutex_unlock(&g_stage_mutex);
        usleep(120000);
    }
    return NULL;
}

int start_obstacle_thread(Stage *stage) {
    g_stage = stage;
    g_thread_running = 1;
    if (pthread_create(&g_thread, NULL, obstacle_thread_func, NULL) != 0) {
        g_thread_running = 0;
        return -1;
    }
    return 0;
}

void stop_obstacle_thread(void) {
    if (!g_thread_running) return;
    g_thread_running = 0;
    pthread_join(g_thread, NULL);
    g_stage = NULL;
}
