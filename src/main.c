#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "../include/game.h"
#include "stage.h"
#include "player.h"
#include "obstacle.h"
#include "render.h"
#include "timer.h"
#include "fileio.h"
#include "input.h"
#include "signal_handler.h"

extern int is_goal_reached(const Stage *stage, const Player *player);
extern int check_collision(const Stage *stage, const Player *player);

#define NUM_STAGES 5

int main(void)
{
    setup_signal_handlers();

    if (init_renderer() != 0)
    {
        fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    init_input();

    struct timeval global_start, global_end;
    gettimeofday(&global_start, NULL);

    int cleared_all = 1;

    for (int s = 1; s <= NUM_STAGES && g_running; s++)
    {
        Stage stage;
        if (load_stage(&stage, s) != 0)
        {
            fprintf(stderr, "Failed to load stage %d\n", s);
            cleared_all = 0;
            break;
        }

        Player player;
        init_player(&player, &stage);

        if (start_obstacle_thread(&stage) != 0)
        {
            fprintf(stderr, "Failed to start obstacle thread\n");
            cleared_all = 0;
            break;
        }

        struct timeval stage_start, now;
        gettimeofday(&stage_start, NULL);

        int stage_cleared = 0;
        int stage_failed = 0;

        while (g_running)
        {
            gettimeofday(&now, NULL);
            double elapsed = get_elapsed_time(stage_start, now);

            pthread_mutex_lock(&g_stage_mutex);
            render(&stage, &player, elapsed, s, NUM_STAGES);
            pthread_mutex_unlock(&g_stage_mutex);

            pthread_mutex_lock(&g_stage_mutex);
            if (check_collision(&stage, &player))
            {
                stage_failed = 1;
                pthread_mutex_unlock(&g_stage_mutex);
                break;
            }

            if (is_goal_reached(&stage, &player))
            {
                stage_cleared = 1;
                pthread_mutex_unlock(&g_stage_mutex);
                break;
            }
            pthread_mutex_unlock(&g_stage_mutex);

            int key = poll_input();

            if (key != -1)
            {
                if (key == 'q' || key == 'Q')
                {
                    g_running = 0;
                    break;
                }
                else
                {
                    pthread_mutex_lock(&g_stage_mutex);
                    move_player(&player, (char)key, &stage);
                    pthread_mutex_unlock(&g_stage_mutex);
                }
            }

            usleep(10000);
        }

        stop_obstacle_thread();

        if (!g_running)
        {
            cleared_all = 0;
            break;
        }

        if (stage_failed)
        {
            printf("You were caught at Stage %d! Game Over.\n", s);
            cleared_all = 0;
            break;
        }

        if (stage_cleared)
        {
            printf("Stage %d Cleared!\n", s);
            fflush(stdout);
            sleep(1);
        }
    }

    gettimeofday(&global_end, NULL);
    double total_time = get_elapsed_time(global_start, global_end);

    printf("\n===== GAME RESULT =====\n");
    printf("Total Playtime: %.3fs\n", total_time);

    double best_time = load_best_record();
    if (cleared_all && g_running)
    {
        printf("You cleared all stages!\n");
        if (best_time <= 0.0 || total_time < best_time)
        {
            printf("New Record!\n");
        }
        update_record_if_better(total_time);
    }
    else
    {
        printf("You failed to clear all stages. Record unchanged.\n");
    }

    best_time = load_best_record();
    printf("Best Record: %.3fs\n", best_time);
    printf("Your Time : %.3fs\n", total_time);

    restore_input();
    shutdown_renderer();

    return 0;
}
