#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <pthread.h>
#include "game.h"

extern pthread_mutex_t g_stage_mutex;

int start_obstacle_thread(Stage *stage);
void stop_obstacle_thread(void);

void move_obstacles(Stage *stage); // also exposed for single-threaded test

#endif
