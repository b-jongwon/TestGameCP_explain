// src/professor_patterns.c

#include "../include/professor_pattern.h"
#include <stdio.h>
#include <math.h>

typedef int (*PatternFunc)(Stage *, Obstacle *, Player *, double);

int pattern_stage_b1(Stage *stage, Obstacle *prof, Player *player, double delta_time)
{

    return 1;
}

/**
 * Stage 2 교수 패턴
 * - 교수 캐릭터에 발각시
 *   1) 플레이어 시야 혼란 상태 (전반적인 화면 밝기 어두워짐)
 */
int pattern_stage_1f(Stage *stage, Obstacle *prof, Player *player, double delta_time)
{
    (void)stage;

    if (!prof || !player)
        return 1;

    // -------------------------------------------------------------
    // 1. 상수 정의
    // -------------------------------------------------------------
    const double BASE_SPEED_PER_TILE_SEC = 0.20;
    const double BASE_MOVE_SPEED = SUBPIXELS_PER_TILE / BASE_SPEED_PER_TILE_SEC;

    const double PROF_BOOST_FACTOR = 1.0;

    // -------------------------------------------------------------
    // 2. 시야 차단/혼란 로직 (발견 즉시)
    // -------------------------------------------------------------

    if (prof->alert)
    {
        // A. 발견 즉시 플레이어에게 시야 혼란 상태 부여
        player->is_confused = 1;

        // B. 교수님은 가속 추격
        prof->move_speed = BASE_MOVE_SPEED * PROF_BOOST_FACTOR;

        // printf("DEBUG P2: 교수님에게 발각! 즉시 시야 차단 발동.\n");
    }
    else
    {
        // A. 미발견 상태일 때 시야 혼란 상태 해제
        player->is_confused = 0;

        // B. 교수님 속도는 기본 속도 유지
        prof->move_speed = BASE_MOVE_SPEED;
    }

    // 플레이어를 놓쳤다면 타이머(p_timer)를 초기화합니다.
    if (!prof->alert)
    {
        prof->p_timer = 0.0;
    }

    return 1;
}

/**
 * Stage 3 교수 패턴
 * - 교수 캐릭터 발각시
 *   1) 일정 주기(2.5초)마다 플레이어의 근처 벽 타일로 순간 이동
 *   2) 교수 캐릭터 1.4배 가속 추격
 */
int pattern_stage_2f(Stage *stage, Obstacle *prof, Player *player, double delta_time)
{
    player->is_confused = 0; // 2단계(1층) 상태 초기화

    if (!prof || !player || !stage)
        return 1;

    if (delta_time < 0.0)
        delta_time = 0.0;
    prof->p_timer += delta_time; // 타이머 업데이트

    // -------------------------------------------------------------
    // 1. 상수 정의
    // -------------------------------------------------------------
    // 🚨 0.20은 Stage 2의 prof_sec_per_tile 값입니다.
    const double BASE_SPEED_PER_TILE_SEC = 0.20;
    const double BASE_MOVE_SPEED = SUBPIXELS_PER_TILE / BASE_SPEED_PER_TILE_SEC;

    const double CYCLE = 2.5;          // 총 2.5초 주기
    const double SWEEP_DURATION = 1.5; // 1.5초 동안 가속 추격
    const double BOOST_FACTOR = 1.4;   // 🚨 1.5배 가속
    const int TILE_SIZE = SUBPIXELS_PER_TILE;

    double t = fmod(prof->p_timer, CYCLE); // 0.0 ~ 2.5 사이의 주기 시간 계산

    // -------------------------------------------------------------
    // 2. 가속/차단 로직 적용
    // -------------------------------------------------------------

    if (t < SWEEP_DURATION) // 0.0초부터 1.5초까지 (추격 모드)
    {
        // 1.5초 동안 가속 추격
        prof->move_speed = BASE_MOVE_SPEED * BOOST_FACTOR;
    }
    else // 1.5초부터 2.5초까지 (차단/딜레이 모드)
    {
        // 🚨 차단 발동 시점 (2.5초 주기 시작 직전)
        // 이 로직은 차단 구간이 시작될 때 단 한 번만 실행되어야 합니다.
        if (t - delta_time < SWEEP_DURATION)
        {
            // A. 무작위 목표 타일 위치 찾기 (플레이어 근처의 유효한 벽 타일)
            // 🚨 이 find_random_block_tile 함수는 외부에서 정의되어야 합니다.
            // 여기서는 임시로 플레이어 타일 위치를 사용합니다.
            int player_tile_x = prof->world_x / TILE_SIZE;
            int player_tile_y = prof->world_y / TILE_SIZE;

            // 임시 목표 (현재 맵 타일이 벽인지 검사하는 로직이 필요하지만, 여기서는 단순화)
            int block_x = player_tile_x + (rand() % 3) - 1; // 플레이어 근처 1칸 이내
            int block_y = player_tile_y + (rand() % 3) - 1;

            // B. 순간 이동
            prof->world_x = block_x * TILE_SIZE;
            prof->world_y = block_y * TILE_SIZE;

            // C. 추격 목표도 리셋
            prof->target_world_x = prof->world_x;
            prof->target_world_y = prof->world_y;

            printf("플레이어 발각! 교수님 (%d, %d)로 순간이동.\n", block_x, block_y);
            fflush(stdout);
        }

        // 1.0초 동안 정지 (경로 차단 효과)
        prof->move_speed = 0.0;
    }

    // 플레이어를 놓쳤다면 타이머를 리셋합니다.
    if (!prof->alert)
    {
        prof->p_timer = 0.0;
    }

    return 1;
}

int pattern_stage_3f(Stage *stage, Obstacle *prof, Player *player, double delta_time)
{

    return 1;
}

/**
 * Stage 5 교수 패턴
 * - 교수(alert=1)인 동안 4초 주기로
 *   1) 순간적으로 강한 감속
 *   2) 천천히 원래 속도로 회복
 */
int pattern_stage_4f(Stage *stage, Obstacle *prof, Player *player, double delta_time)
{
    (void)stage;
    if (!prof || !player)
        return 1;

    // 교수에게 아직 안 걸렸으면: 패턴 효과 없음 + 속도 원상복귀
    if (!prof->alert)
    {
        prof->p_timer = 0.0;

        double base_speed = player->base_move_speed * player->speed_multiplier; // 스쿠터 포함 기본 속도
        player->move_speed = base_speed;
        return 1; // 교수는 계속 움직여도 됨
    }

    if (delta_time < 0.0)
        delta_time = 0.0;

    // ====== 타이머 누적 (교수 1마리별로 따로 돌아가는 타이머) ======
    prof->p_timer += delta_time;

    // 한 사이클 길이 (초) – 네가 말한 4초
    const double CYCLE = 4.0;
    // "한번 확 느려지는 구간" 길이
    const double HIT_DURATION = 0.2; // 0.2초 동안 최저 속도 유지
    const double MIN_FACTOR = 0.25;  // 최저 속도: 원래의 25%

    double t = fmod(prof->p_timer, CYCLE);

    double base_speed = player->base_move_speed * player->speed_multiplier;

    double factor;
    if (t < HIT_DURATION)
    {
        // 1) 처음 HIT_DURATION 동안은 **확 느려진 상태** 유지
        factor = MIN_FACTOR;
    }
    else
    {
        // 2) 그 이후 ~ 4초까지는 선형으로 서서히 회복
        double recover_time = CYCLE - HIT_DURATION;   // 4.0 - 0.2 = 3.8
        double u = (t - HIT_DURATION) / recover_time; // 0 ~ 1
        if (u < 0.0)
            u = 0.0;
        if (u > 1.0)
            u = 1.0;

        factor = MIN_FACTOR + (1.0 - MIN_FACTOR) * u; // MIN_FACTOR → 1.0 으로 서서히 증가
    }

    player->move_speed = base_speed * factor; // 최종 적용 속도: (난이도/스쿠터 등 기본 속도) * (교수 디버프 계수)

    return 1; // 이동은 그대로 진행
}

int pattern_stage_5f(Stage *stage, Obstacle *prof, Player *player, double dt)
{
    // 1~5의 패턴을 모두 적용
    int p1 = pattern_stage_b1(stage, prof, player, dt);
    int p2 = pattern_stage_1f(stage, prof, player, dt);
    int p3 = pattern_stage_2f(stage, prof, player, dt);
    int p4 = pattern_stage_3f(stage, prof, player, dt);
    int p5 = pattern_stage_4f(stage, prof, player, dt);

    return (p1 && p2 && p3 && p4 && p5);
}

static const PatternFunc kPatterns[] = {
    NULL,            // 0 (Not used)
    pattern_stage_1, // Stage 1
    pattern_stage_2, // Stage 2
    pattern_stage_3, // Stage 3
    pattern_stage_4, // Stage 4
    pattern_stage_5, // Stage 5
    pattern_stage_6  // Stage 6
};

int update_professor_pattern(Stage *stage, Obstacle *prof, Player *player, double delta_time)
{
    int id = stage->id;

    // 안전장치
    if (id < 1 || id > 6)
        return 0;

    // 해당 스테이지 함수 호출
    if (kPatterns[id])
    {
        return kPatterns[id](stage, prof, player, delta_time);
    }
    return 1;
}
