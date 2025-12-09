#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "../include/fileio.h"
#include "../include/game.h"
#include "../include/input.h"
#include "../include/obstacle.h"
#include "../include/player.h"
#include "../include/professor_pattern.h"
#include "../include/projectile.h"
#include "../include/render.h"
#include "../include/signal_handler.h"
#include "../include/sound.h"
#include "../include/stage.h"
#include "../include/timer.h"

extern int is_goal_reached(const Stage *stage, const Player *player);
extern int check_collision(Stage *stage, Player *player);

typedef enum
{
    APP_STATE_TITLE = 0,
    APP_STATE_RECORDS,
    APP_STATE_GAMEPLAY,
    APP_STATE_GAME_OVER,
    APP_STATE_EXIT
} AppState;

typedef enum
{
    TITLE_MENU_START = 0,
    TITLE_MENU_RECORDS,
    TITLE_MENU_EXIT,
    TITLE_MENU_COUNT
} TitleMenuSelection;

typedef enum
{
    GAMEPLAY_OUTCOME_ABORTED = 0,
    GAMEPLAY_OUTCOME_CLEARED,
    GAMEPLAY_OUTCOME_FAILED
} GameplayOutcome;

typedef struct
{
    const char *bgm_file_path;
    const char *gameover_bgm_path;
    const char *item_sound_path;
    const char *item_use_sound_path;
    const char *next_level_sound_path;
    const char *bag_acquire_sound_path;
    const char *walking_sound_path;
    const char *no_item_sound_path;
} SoundAssets;

static const double kScooterDurationSec = 20.0;
static const double kWalkSfxIntervalBaseSec = 0.45;
static const double kWalkSfxIntervalScooterSec = 0.25;
static double g_last_walk_sfx_time = 0.0;

static int run_title_menu(void);
static void run_records_view(void);
static void run_game_over_view(void);
static GameplayOutcome run_campaign(int start_stage_id,
                                    int end_stage_id,
                                    int stages_to_play,
                                    int playing_full_campaign,
                                    const SoundAssets *sounds);
static void drain_pending_input(void);

int main(int argc, char *argv[])
{
    setup_signal_handlers();
    init_sound_system();

    if (init_renderer() != 0)
    {
        fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    init_input();

    SoundAssets sounds = {
        .bgm_file_path = "bgm/BGM.wav",
        .gameover_bgm_path = "bgm/bgm_GameOut.wav",
        .item_sound_path = "bgm/Get_Item.wav",
        .item_use_sound_path = "bgm/Use_Item.wav",
        .next_level_sound_path = "bgm/Next_Level.wav",
        .bag_acquire_sound_path = "bgm/Get_Bag.wav",
        .walking_sound_path = "bgm/Walking.wav",
        .no_item_sound_path = "bgm/No_Item.wav"};

    const int available_stage_count = get_stage_count();
    int start_stage_id = 1;
    int end_stage_id = available_stage_count;
    int stages_to_play = available_stage_count;
    int playing_full_campaign = 1;

    if (argc >= 2)
    {
        int requested_stage_id = find_stage_id_by_filename(argv[1]);
        if (requested_stage_id < 0)
        {
            fprintf(stderr, "알 수 없는 맵 파일: %s\n", argv[1]);
            fprintf(stderr, "assets/ 디렉토리에 존재하는 .map 파일명을 인자로 넘겨주세요.\n");
            restore_input();
            shutdown_renderer();
            return 1;
        }

        start_stage_id = requested_stage_id;
        end_stage_id = requested_stage_id;
        stages_to_play = 1;
        playing_full_campaign = 0;
        printf("지정된 맵(%s)만 플레이합니다.\n", argv[1]);
    }

    AppState state = APP_STATE_TITLE;
    while (state != APP_STATE_EXIT && g_running)
    {
        switch (state)
        {
        case APP_STATE_TITLE:
        {
            int selection = run_title_menu();
            if (!g_running)
            {
                state = APP_STATE_EXIT;
                break;
            }
            if (selection == TITLE_MENU_START)
            {
                state = APP_STATE_GAMEPLAY;
            }
            else if (selection == TITLE_MENU_RECORDS)
            {
                state = APP_STATE_RECORDS;
            }
            else
            {
                state = APP_STATE_EXIT;
            }
            break;
        }
        case APP_STATE_RECORDS:
            run_records_view();
            state = APP_STATE_TITLE;
            break;
        case APP_STATE_GAMEPLAY:
        {
            GameplayOutcome outcome = run_campaign(start_stage_id,
                                                   end_stage_id,
                                                   stages_to_play,
                                                   playing_full_campaign,
                                                   &sounds);
            if (!g_running)
            {
                state = APP_STATE_EXIT;
                break;
            }
            if (outcome == GAMEPLAY_OUTCOME_FAILED)
            {
                state = APP_STATE_GAME_OVER;
            }
            else if (outcome == GAMEPLAY_OUTCOME_ABORTED)
            {
                state = APP_STATE_EXIT;
            }
            else
            {
                state = APP_STATE_TITLE;
            }
            break;
        }
        case APP_STATE_GAME_OVER:
            run_game_over_view();
            state = APP_STATE_TITLE;
            break;
        case APP_STATE_EXIT:
        default:
            state = APP_STATE_EXIT;
            break;
        }
    }

    stop_bgm();
    restore_input();
    shutdown_renderer();
    return 0;
}

static int run_title_menu(void)
{
    int selection = TITLE_MENU_START;
    drain_pending_input();
    while (g_running)
    {
        render_title_screen(selection);
        SDL_Delay(16);

        int key = read_input();
        if (key == -1)
        {
            continue;
        }

        if (key == 'w' || key == 'W')
        {
            selection = (selection + TITLE_MENU_COUNT - 1) % TITLE_MENU_COUNT;
            continue;
        }
        if (key == 's' || key == 'S')
        {
            selection = (selection + 1) % TITLE_MENU_COUNT;
            continue;
        }
        if (key == 'q' || key == 'Q')
        {
            return TITLE_MENU_EXIT;
        }
        if (key == '\r' || key == '\n' || key == ' ' || key == 'k' || key == 'K')
        {
            return selection;
        }
    }
    return TITLE_MENU_EXIT;
}

static void run_records_view(void)
{
    double best_time = load_best_record();
    drain_pending_input();
    while (g_running)
    {
        render_records_screen(best_time);
        SDL_Delay(16);
        if (read_input() != -1)
        {
            break;
        }
    }
}

static void run_game_over_view(void)
{
    drain_pending_input();
    while (g_running)
    {
        render_game_over_screen();
        SDL_Delay(16);
        if (read_input() != -1)
        {
            break;
        }
    }
}

static GameplayOutcome run_campaign(int start_stage_id,
                                    int end_stage_id,
                                    int stages_to_play,
                                    int playing_full_campaign,
                                    const SoundAssets *sounds)
{
    if (!sounds)
    {
        return GAMEPLAY_OUTCOME_ABORTED;
    }

    struct timeval global_start, global_end;
    gettimeofday(&global_start, NULL);

    int cleared_all = 1;
    int failure_detected = 0;

    const char *tts_start_command = "espeak -a 200 -v en-us+m5 -s 160 'Game Start!'";
    fflush(stdout);
    system(tts_start_command);
    play_bgm(sounds->bgm_file_path, 1);

    for (int stage_id = start_stage_id, stage_counter = 0;
         stage_id <= end_stage_id && g_running;
         stage_id++, stage_counter++)
    {
        const int current_stage_display = stage_counter + 1;
        Stage stage;
        if (load_stage(&stage, stage_id) != 0)
        {
            fprintf(stderr, "Failed to load stage %d\n", stage_id);
            stop_bgm();
            cleared_all = 0;
            failure_detected = 1;
            break;
        }

        Player player;
        init_player(&player, &stage);
        g_last_walk_sfx_time = 0.0;

        set_obstacle_player_ref(&player);
        if (start_obstacle_thread(&stage) != 0)
        {
            fprintf(stderr, "Failed to start obstacle thread\n");
            stop_bgm();
            cleared_all = 0;
            failure_detected = 1;
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

            pthread_mutex_lock(&g_stage_mutex);
            int move_finished = update_player_motion(&player, frame_delta);
            if (!player.has_backpack &&
                is_tile_center_inside_player(&player, stage.goal_x, stage.goal_y))
            {
                player.has_backpack = 1;
                stage.map[stage.goal_y][stage.goal_x] = ' ';
                play_sfx_nonblocking(sounds->bag_acquire_sound_path);
            }
            render(&stage, &player, elapsed, current_stage_display, stages_to_play);
            pthread_mutex_unlock(&g_stage_mutex);

            if (move_finished)
            {
                int held = current_direction_key();
                if (held != -1)
                {
                    pthread_mutex_lock(&g_stage_mutex);
                    double walk_interval = player.has_scooter ? kWalkSfxIntervalScooterSec : kWalkSfxIntervalBaseSec;
                    move_player(&player, (char)held, &stage, elapsed);
                    if (elapsed - g_last_walk_sfx_time >= walk_interval)
                    {
                        play_sfx_nonblocking(sounds->walking_sound_path);
                        g_last_walk_sfx_time = elapsed;
                    }
                    pthread_mutex_unlock(&g_stage_mutex);
                }
            }

            pthread_mutex_lock(&g_stage_mutex);
            if (player.shield_count > 0)
            {
                if (check_trap_collision(&stage, &player) || check_collision(&stage, &player))
                {
                    player.shield_count--;
                    printf("쉴드로 방어 했습니다! 남은 쉴드: %d개\n", player.shield_count);
                    play_sfx_nonblocking(sounds->item_use_sound_path);
                    pthread_mutex_unlock(&g_stage_mutex);
                    continue;
                }
            }

            if (check_trap_collision(&stage, &player))
            {
                printf("트랩을 밟았습니다!\n");
                stop_bgm();
                const char *tts_game_out_command = "espeak -a 200 -v en-us+m5 -s 140 'Game Out!'";
                fflush(stdout);
                system(tts_game_out_command);
                play_obstacle_caught_sound(sounds->gameover_bgm_path);
                stage_failed = 1;
                pthread_mutex_unlock(&g_stage_mutex);
                break;
            }

            if (check_collision(&stage, &player))
            {
                stop_bgm();
                const char *tts_game_out_command = "espeak -a 200 -v en-us+m5 -s 140 'Game Out!'";
                fflush(stdout);
                system(tts_game_out_command);
                play_obstacle_caught_sound(sounds->gameover_bgm_path);
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
                if (key == 'k' || key == 'K' || key == ' ')
                {
                    pthread_mutex_lock(&g_stage_mutex);
                    if (stage.remaining_ammo > 0)
                    {
                        fire_projectile(&stage, &player);
                        play_sfx_nonblocking(sounds->item_use_sound_path);
                    }
                    else
                    {
                        play_sfx_nonblocking(sounds->no_item_sound_path);
                    }
                    pthread_mutex_unlock(&g_stage_mutex);
                    continue;
                }

                pthread_mutex_lock(&g_stage_mutex);
                move_player(&player, (char)key, &stage, elapsed);
                double walk_interval = player.has_scooter ? kWalkSfxIntervalScooterSec : kWalkSfxIntervalBaseSec;
                if (elapsed - g_last_walk_sfx_time >= walk_interval)
                {
                    play_sfx_nonblocking(sounds->walking_sound_path);
                    g_last_walk_sfx_time = elapsed;
                }
                pthread_mutex_unlock(&g_stage_mutex);
            }
            else
            {
                pthread_mutex_lock(&g_stage_mutex);
                update_player_idle(&player, elapsed);
                pthread_mutex_unlock(&g_stage_mutex);
            }

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

                it->active = 0;
                play_sfx_nonblocking(sounds->item_sound_path);

                switch (it->type)
                {
                case ITEM_TYPE_SHIELD:
                    player.shield_count++;
                    printf("보호막을 획득했습니다! 현재 보호막: %d개\n", player.shield_count);
                    break;
                case ITEM_TYPE_SCOOTER:
                {
                    const double scooter_multiplier = 2.0;
                    player.has_scooter = 1;
                    player.speed_multiplier = scooter_multiplier;
                    player.move_speed = player.base_move_speed * player.speed_multiplier;
                    player.scooter_expire_time = elapsed + kScooterDurationSec;
                    printf("E-scooter 효과 활성화! %.1f초 동안 이동 속도 증가\n", player.speed_multiplier);
                    break;
                }
                case ITEM_TYPE_SUPPLY:
                    stage.remaining_ammo += SUPPLY_REFILL_AMOUNT;
                    printf("탄약 보충! 남은 투사체: %d\n", stage.remaining_ammo);
                    break;
                default:
                    break;
                }
            }
            pthread_mutex_unlock(&g_stage_mutex);

            pthread_mutex_lock(&g_stage_mutex);
            if (player.has_scooter && player.scooter_expire_time > 0.0 && elapsed >= player.scooter_expire_time)
            {
                player.has_scooter = 0;
                player.speed_multiplier = 1.0;
                player.move_speed = player.base_move_speed * player.speed_multiplier;
                player.scooter_expire_time = 0.0;
                printf("E-scooter 효과가 종료되었습니다.\n");
            }
            pthread_mutex_unlock(&g_stage_mutex);

            ProfessorBulletResult bullet_result = PROFESSOR_BULLET_RESULT_NONE;
            pthread_mutex_lock(&g_stage_mutex);
            move_projectiles(&stage);
            bullet_result = update_professor_bullets(&stage, &player, frame_delta);
            pthread_mutex_unlock(&g_stage_mutex);

            if (bullet_result == PROFESSOR_BULLET_RESULT_SHIELD_BLOCKED)
            {
                printf("교수의 탄환을 쉴드로 막았습니다! 남은 쉴드: %d개\n", player.shield_count);
                play_sfx_nonblocking(sounds->item_use_sound_path);
            }
            else if (bullet_result == PROFESSOR_BULLET_RESULT_FATAL)
            {
                stop_bgm();
                const char *tts_game_out_command = "espeak -a 200 -v en-us+m5 -s 140 'Game Out!'";
                fflush(stdout);
                system(tts_game_out_command);
                play_obstacle_caught_sound(sounds->gameover_bgm_path);
                stage_failed = 1;
                break;
            }

            struct timespec frame_end_ts;
            clock_gettime(CLOCK_MONOTONIC, &frame_end_ts);
            double frame_time_ms = (frame_end_ts.tv_sec - frame_start_ts.tv_sec) * 1000.0 +
                                   (frame_end_ts.tv_nsec - frame_start_ts.tv_nsec) / 1000000.0;
            const double target_frame_time_ms = 16.67;
            if (frame_time_ms < target_frame_time_ms)
            {
                double sleep_time_ms = target_frame_time_ms - frame_time_ms;
                struct timespec sleep_ts = {
                    .tv_sec = 0,
                    .tv_nsec = (long)(sleep_time_ms * 1000000.0)};
                nanosleep(&sleep_ts, NULL);
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
            printf("지금까지 출튀 한 횟수는 %d 번!  게임종료.\n", current_stage_display);
            cleared_all = 0;
            failure_detected = 1;
            break;
        }

        if (stage_cleared)
        {
            const char *tts_clear_command = "espeak -a 180 -v en-us+m3 'Clear!'";
            fflush(stdout);
            system(tts_clear_command);
            play_sfx_nonblocking(sounds->next_level_sound_path);
            printf("스테이지 %s 출튀 성공!\n", stage.name);
            fflush(stdout);
            sleep(1);
        }
    }

    gettimeofday(&global_end, NULL);
    double total_time = get_elapsed_time(global_start, global_end);

    printf("\n===== 게임 결과 =====\n");
    printf("전체 플레이 시간: %.3fs\n", total_time);

    double best_time = load_best_record();
    if (cleared_all && g_running && !failure_detected)
    {
        const char *tts_game_clear_command = "espeak -a 180 -v en-us+m5 -s 140 'Game Clear!'";
        fflush(stdout);
        system(tts_game_clear_command);

        if (playing_full_campaign)
        {
            printf("모든 스테이지 클리어!\n");
            if (best_time <= 0.0 || total_time < best_time)
            {
                printf("최고 기록!\n");
            }
            update_record_if_better(total_time);
        }
        else
        {
            printf("선택한 스테이지를 모두 클리어했습니다. 전체 기록은 갱신되지 않습니다.\n");
        }
    }
    else
    {
        printf("스테이지 클리어 실패로 기록이 기록되지 않습니다..\n");
    }

    best_time = load_best_record();
    printf("최고기록: %.3fs\n", best_time);
    printf("이번 기록 : %.3fs\n", total_time);

    stop_bgm();

    if (!g_running)
    {
        return GAMEPLAY_OUTCOME_ABORTED;
    }
    if (!cleared_all || failure_detected)
    {
        return GAMEPLAY_OUTCOME_FAILED;
    }
    return GAMEPLAY_OUTCOME_CLEARED;
}

static void drain_pending_input(void)
{
    int flushed = 0;
    while (read_input() != -1)
    {
        flushed = 1;
    }
    if (flushed)
    {
        SDL_Delay(10);
    }
}
