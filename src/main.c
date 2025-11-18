#include <stdio.h>      // printf, fprintf 등 표준 입출력
#include <stdlib.h>     // exit, atexit 등
#include <unistd.h>     // usleep, sleep 등
#include <sys/time.h>   // struct timeval, gettimeofday

// 게임 내부 도메인/모듈 헤더들
#include "game.h"           // Player, Stage, Obstacle 구조체 및 상수들
#include "stage.h"          // load_stage
#include "player.h"         // init_player, move_player
#include "obstacle.h"       // start_obstacle_thread, stop_obstacle_thread, move_obstacles, g_stage_mutex
#include "render.h"         // render 함수
#include "timer.h"          // get_elapsed_time
#include "fileio.h"         // load_best_record, update_record_if_better
#include "input.h"          // init_input, restore_input, poll_input
#include "signal_handler.h" // g_running, setup_signal_handlers

// goal/충돌 체크 함수는 다른 C파일에 구현되어 있고 여기서만 extern으로 선언해 사용
extern int is_goal_reached(const Stage *stage, const Player *player);
extern int check_collision(const Stage *stage, const Player *player);

// 전체 스테이지 개수 (현재 3개)
#define NUM_STAGES 3


// ----------------------------------------------------------
// main()
// ----------------------------------------------------------
// 게임의 진입점. 전체 흐름:
//
// 1. 시그널 핸들러 / 입력 모드 초기화
// 2. 전체 플레이 시간 측정 시작(global_start)
// 3. 각 스테이지(1 ~ NUM_STAGES)에 대해:
//      - 스테이지 로드
//      - 플레이어 초기화
//      - 장애물 스레드 시작
//      - 스테이지 루프:
//          * 시간 계산
//          * 화면 렌더링
//          * 충돌/골 도달 체크
//          * 입력 받아 플레이어 이동
//      - 스레드 종료
//      - 클리어/실패/종료 상황 처리
// 4. 전체 플레이 시간 출력
// 5. 모든 스테이지 클리어 시 기록 갱신
int main(void) {

    // Ctrl+C, SIGTERM 등 시그널이 왔을 때 g_running = 0으로 설정하도록 등록
    setup_signal_handlers();

    // 터미널 입력 모드 초기화 (non-blocking, no-echo 등)
    init_input();

    // 프로그램 정상 종료 시 restore_input() 자동 호출되도록 등록
    atexit(restore_input);

    // 전체 게임 시작 시간 기록
    struct timeval global_start, global_end;
    gettimeofday(&global_start, NULL);

    // 모든 스테이지를 다 깼는지 여부 (초기값: 깰 예정)
    int cleared_all = 1;

    // 스테이지 루프: 1번부터 NUM_STAGES까지
    // g_running이 0이 되면(시그널/종료 요청) 즉시 루프 탈출
    for (int s = 1; s <= NUM_STAGES && g_running; s++) {

        Stage stage;   // 현재 스테이지 정보 저장할 구조체

        // 스테이지 로드 (파일에서 맵, 장애물, 시작/골 위치 등 읽기)
        if (load_stage(&stage, s) != 0) {
            fprintf(stderr, "Failed to load stage %d\n", s);
            exit(1);   // 치명적인 에러로 간주하고 프로그램 종료
        }

        // 플레이어 상태 구조체
        Player player;

        // 플레이어를 현재 스테이지의 시작 위치로 초기화
        init_player(&player, &stage);

        // 장애물 움직이는 스레드 시작
        if (start_obstacle_thread(&stage) != 0) {
            fprintf(stderr, "Failed to start obstacle thread\n");
            exit(1);
        }

        // 이 스테이지에서의 시간 측정을 위한 시작 시각
        struct timeval stage_start, now;
        gettimeofday(&stage_start, NULL);

        // 현재 스테이지가 클리어되었는지, 실패했는지 플래그
        int stage_cleared = 0;
        int stage_failed = 0;

        // 한 스테이지 내의 메인 루프
        while (g_running) {

            // 현재 시각을 얻고 스테이지 시작 시각과의 차이로 경과 시간 계산
            gettimeofday(&now, NULL);
            double elapsed = get_elapsed_time(stage_start, now);

            // ----- 렌더링 -----
            // 스테이지/플레이어/장애물 정보를 그리는 동안
            // 장애물 스레드와 데이터 충돌이 나지 않도록 mutex로 보호
            pthread_mutex_lock(&g_stage_mutex);

            // 터미널 화면 지우기 (리눅스 쉘 기준)
            system("clear");

            // 현재 상태를 화면에 출력
            render(&stage, &player, elapsed, s, NUM_STAGES);

            pthread_mutex_unlock(&g_stage_mutex);
            // ----- 렌더링 끝 -----

            // ----- 충돌 및 골 체크 -----
            pthread_mutex_lock(&g_stage_mutex);

            // 플레이어와 장애물 충돌 여부 확인
            if (check_collision(&stage, &player)) {
                stage_failed = 1;                 // 이 스테이지 실패
                pthread_mutex_unlock(&g_stage_mutex);
                break;                            // 스테이지 루프 탈출
            }

            // 플레이어가 골 위치에 도달했는지 확인
            if (is_goal_reached(&stage, &player)) {
                stage_cleared = 1;                // 이 스테이지 클리어
                pthread_mutex_unlock(&g_stage_mutex);
                break;                            // 스테이지 루프 탈출
            }

            pthread_mutex_unlock(&g_stage_mutex);
            // ----- 체크 끝 -----

            // ----- 입력 처리 -----
            int key = poll_input();   // 현재 눌린 키가 있으면 읽어온다. 없으면 -1.

            if (key != -1) {
                // q 또는 Q를 누르면 게임 전체 종료 요청
                if (key == 'q' || key == 'Q') {
                    g_running = 0;   // 전역 플래그를 꺼서 바깥 루프들까지 멈추게 함
                    break;
                } else {
                    // W/A/S/D 등 이동 키 처리
                    // 이때도 stage/플레이어 상태를 건드리므로 mutex로 보호
                    pthread_mutex_lock(&g_stage_mutex);
                    move_player(&player, (char)key, &stage);
                    pthread_mutex_unlock(&g_stage_mutex);
                }
            }
            // ----- 입력 처리 끝 -----

            // 너무 빠르게 루프를 돌지 않도록 10ms 쉰다.
            usleep(10000);
        }

        // 이 스테이지에서 사용하던 장애물 스레드 중지(합류)
        stop_obstacle_thread();

        // g_running이 0이면(시그널/종료 키) 게임 전체 중단
        if (!g_running) {
            cleared_all = 0;    // 모든 스테이지 클리어는 아님
            break;
        }
        // 스테이지 실패(장애물에 잡힘)
        else if (stage_failed) {
            printf("\nYou were caught at Stage %d! Game Over.\n", s);
            cleared_all = 0;
            break;
        }
        // 스테이지 클리어 성공
        else if (stage_cleared) {
            printf("\nStage %d Cleared!\n", s);
            sleep(1);  // 잠깐 쉬었다 다음 스테이지로
        }
    }

    // 전체 게임 종료 시간 기록 후, 총 플레이 시간 계산
    gettimeofday(&global_end, NULL);
    double total_time = get_elapsed_time(global_start, global_end);

    printf("\nTotal Playtime: %.3fs\n", total_time);

    // 모든 스테이지를 다 깼고, 도중에 강제 종료도 안 된 경우만 기록 갱신
    if (cleared_all && g_running) {
        printf("You cleared all stages!\n");
        update_record_if_better(total_time);
    } else {
        printf("Record is updated only when all stages are cleared.\n");
    }

    return 0;
}
