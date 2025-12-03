#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "../include/game.h"
#include "../include/stage.h"
#include "../include/player.h"
#include "../include/obstacle.h"
#include "../include/render.h"
#include "../include/timer.h"
#include "../include/fileio.h"
#include "../include/input.h"
#include "../include/signal_handler.h"
#include "../include/projectile.h"
#include "../include/sound.h" //bgm 추가

extern int is_goal_reached(const Stage *stage, const Player *player);
extern int check_collision(Stage *stage, Player *player);

static const double kScooterDurationSec = 20.0;

int main(void)
{
    setup_signal_handlers();

    if (init_renderer() != 0)
    {
        fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    init_input();
    const char *bgm_file_path = "bgm/BGM.wav";                // bgm 파일 경로 설정
    const char *gameover_bgm_path = "bgm/bgm_GameOut.wav";    // 장애물 게임오버 bgm 파일 경로 설정
    const char *item_sound_path = "bgm/Get_Item.wav";         // 아이템 획득 사운드 파일 경로 설정
    const char *item_use_sound_path = "bgm/Use_Item.wav";     // 아이템 사용 사운드 파일 경로 설정
    const char *next_level_sound_path = "bgm/Next_Level.wav"; // 스테이지 클리어, 다음 레벨 전환 사운드 파일 경로 설정
    const char *bag_acquire_sound_path = "bgm/Get_Bag.wav";   // 가방 획득 사운드 파일 경로 설정
    const char *walking_sound_path = "bgm/Walking.wav";       // 걷기 사운드 파일 경로 설정

    struct timeval global_start, global_end;
    gettimeofday(&global_start, NULL);

    int cleared_all = 1;

    play_bgm(bgm_file_path, 1); // BGM 재생 시작 (Non-blocking)

    const int total_stages = get_stage_count();

    for (int s = 1; s <= total_stages && g_running; s++)
    {
        Stage stage;
        if (load_stage(&stage, s) != 0)
        {
            fprintf(stderr, "Failed to load stage %d\n", s);
            stop_bgm(); // 오류 발생시 bgm 중지
            cleared_all = 0;
            break;
        }

        Player player;
        init_player(&player, &stage);

        set_obstacle_player_ref(&player);

        if (start_obstacle_thread(&stage) != 0)
        {
            fprintf(stderr, "Failed to start obstacle thread\n");
            stop_bgm(); // 오류 발생시 bgm 중지
            cleared_all = 0;
            break;
        }

        struct timeval stage_start, now;
        gettimeofday(&stage_start, NULL);
        double previous_elapsed = 0.0;

        int stage_cleared = 0;
        int stage_failed = 0;

        while (g_running)
        {
            struct timespec frame_start_ts;
            clock_gettime(CLOCK_MONOTONIC, &frame_start_ts);

            gettimeofday(&now, NULL);
            double elapsed = get_elapsed_time(stage_start, now);
            double frame_delta = elapsed - previous_elapsed;
            if (frame_delta < 0.0)
            {
                frame_delta = 0.0;
            }
            previous_elapsed = elapsed;

            int move_finished = 0;

            pthread_mutex_lock(&g_stage_mutex);
            move_finished = update_player_motion(&player, frame_delta);
            if (!player.has_backpack &&
                is_tile_center_inside_player(&player, stage.goal_x, stage.goal_y))
            {
                player.has_backpack = 1;
                stage.map[stage.goal_y][stage.goal_x] = ' ';

                play_sfx_nonblocking(bag_acquire_sound_path); // 가방 획득 사운드 재생 (Non-blocking)
            }
            render(&stage, &player, elapsed, s, total_stages);
            pthread_mutex_unlock(&g_stage_mutex);

            if (move_finished)
            {
                int held = current_direction_key();
                if (held != -1)
                {
                    pthread_mutex_lock(&g_stage_mutex);
                    move_player(&player, (char)held, &stage, elapsed);
                    pthread_mutex_unlock(&g_stage_mutex);
                }
            }

            pthread_mutex_lock(&g_stage_mutex);

            if (check_collision(&stage, &player)) // 충돌 체크
            {
                stop_bgm();                                    // 충돌 시 기존 BGM 중지
                play_obstacle_caught_sound(gameover_bgm_path); // 장애물 게임오버 사운드 재생 (Blocking)
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
                // --- 🔥 투사체 발사 ---
                if (key == 'k' || key == 'K')
                {
                    pthread_mutex_lock(&g_stage_mutex);
                    fire_projectile(&stage, &player);
                    play_sfx_nonblocking(item_use_sound_path); // 투사체 발사 사운드 재생 (논블로킹)
                    pthread_mutex_unlock(&g_stage_mutex);
                    continue; // 이동 처리와 겹치지 않게 skip
                }

                pthread_mutex_lock(&g_stage_mutex);
                move_player(&player, (char)key, &stage, elapsed);

                play_sfx_nonblocking(walking_sound_path); // 걷기 사운드 재생 (논블로킹)

                pthread_mutex_unlock(&g_stage_mutex);
            }
            else
            {
                pthread_mutex_lock(&g_stage_mutex);
                update_player_idle(&player, elapsed);
                pthread_mutex_unlock(&g_stage_mutex);
            }

            // ===== 아이템 획득 체크 =====
            pthread_mutex_lock(&g_stage_mutex);
            for (int i = 0; i < stage.num_items; i++)
            {
                Item *it = &stage.items[i];
                if (!it->active)
                {
                    continue;
                }

                int item_tile_x = it->world_x / SUBPIXELS_PER_TILE;
                int item_tile_y = it->world_y / SUBPIXELS_PER_TILE;
                if (!is_tile_center_inside_player(&player, item_tile_x, item_tile_y))
                {
                    continue;
                }

                it->active = 0; // 아이템 비활성화 (맵에서 사라짐)

                switch (it->type)
                {
                case ITEM_TYPE_SHIELD:
                    player.shield_count++; // 보호막 1개 획득
                    printf("Shield acquired! (x%d)\n", player.shield_count);
                    break;
                case ITEM_TYPE_SCOOTER:
                {
                    const double scooter_multiplier = 2.0;
                    player.has_scooter = 1;
                    player.speed_multiplier = scooter_multiplier;
                    player.move_speed = player.base_move_speed * player.speed_multiplier;
                    player.scooter_expire_time = elapsed + kScooterDurationSec;
                    printf("Scooter equipped! Speed multiplier: %.1fx\n", player.speed_multiplier);
                    break;
                }
                default:
                    break;
                }

                play_sfx_nonblocking(item_sound_path); // 아이템 획득 사운드 재생 (Non-blocking)
            }
            if (player.has_scooter && player.scooter_expire_time > 0.0 && elapsed >= player.scooter_expire_time)
            {
                player.has_scooter = 0;
                player.speed_multiplier = 1.0;
                player.move_speed = player.base_move_speed * player.speed_multiplier;
                player.scooter_expire_time = 0.0;
                printf("Scooter effect expired.\n");
            }

            pthread_mutex_unlock(&g_stage_mutex);

            pthread_mutex_lock(&g_stage_mutex);
            move_projectiles(&stage);
            pthread_mutex_unlock(&g_stage_mutex);

            struct timespec frame_end_ts;
            clock_gettime(CLOCK_MONOTONIC, &frame_end_ts);
            double frame_time = (frame_end_ts.tv_sec - frame_start_ts.tv_sec) +
                                (frame_end_ts.tv_nsec - frame_start_ts.tv_nsec) / 1e9;
            const double target_frame = 1.0 / 60.0;
            if (frame_time < target_frame)
            {
                double sleep_sec = target_frame - frame_time;
                if (sleep_sec > 0.0)
                {
                    struct timespec sleep_ts;
                    sleep_ts.tv_sec = (time_t)sleep_sec;
                    sleep_ts.tv_nsec = (long)((sleep_sec - sleep_ts.tv_sec) * 1e9);
                    if (sleep_ts.tv_nsec < 0)
                        sleep_ts.tv_nsec = 0;
                    nanosleep(&sleep_ts, NULL);
                }
            }
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
            play_sfx_nonblocking(next_level_sound_path); // 다음 레벨 전환 사운드 재생 (논블로킹)

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

    stop_bgm(); // 게임 종료 시 BGM 중지

    restore_input();
    shutdown_renderer();

    return 0;
}
