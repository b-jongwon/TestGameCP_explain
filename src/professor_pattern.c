// src/professor_patterns.c



#include "../include/professor_pattern.h"
#include <stdio.h> 
#include <math.h>

typedef int (*PatternFunc)(Stage*, Obstacle*,  Player*, double);



int pattern_stage_1(Stage *stage, Obstacle *prof,  Player *player, double delta_time)
{
    
    return 1; 
}


int pattern_stage_2(Stage *stage, Obstacle *prof,  Player *player, double delta_time)
{
    
    return 1;
}


int pattern_stage_3(Stage *stage, Obstacle *prof,  Player *player, double delta_time)
{
    
    return 1;
}


int pattern_stage_4(Stage *stage, Obstacle *prof,  Player *player, double delta_time)
{
   
    return 1;
}


/**
 * Stage 5 교수 패턴
 * - 교수(alert=1)인 동안 4초 주기로
 *   1) 순간적으로 강한 감속
 *   2) 천천히 원래 속도로 회복
 */
int pattern_stage_5(Stage *stage, Obstacle *prof, Player *player, double delta_time)
{
    (void)stage;
    if (!prof || !player) return 1;

    // 교수에게 아직 안 걸렸으면: 패턴 효과 없음 + 속도 원상복귀
    if (!prof->alert)
    {
        prof->p_timer = 0.0;

        double base_speed = player->base_move_speed * player->speed_multiplier; // 스쿠터 포함 기본 속도
        player->move_speed = base_speed;
        return 1; // 교수는 계속 움직여도 됨
    }

    if (delta_time < 0.0) delta_time = 0.0;

    // ====== 타이머 누적 (교수 1마리별로 따로 돌아가는 타이머) ======
    prof->p_timer += delta_time;

    // 한 사이클 길이 (초) – 네가 말한 4초
    const double CYCLE = 4.0;
    // "한번 확 느려지는 구간" 길이
    const double HIT_DURATION = 0.2;   // 0.2초 동안 최저 속도 유지
    const double MIN_FACTOR   = 0.25;  // 최저 속도: 원래의 25%

   
    double t = fmod(prof->p_timer, CYCLE);

    // 스쿠터/난이도 포함 “기본 러닝 속도”
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
        double recover_time = CYCLE - HIT_DURATION;       // 4.0 - 0.2 = 3.8
        double u = (t - HIT_DURATION) / recover_time;     // 0 ~ 1
        if (u < 0.0) u = 0.0;
        if (u > 1.0) u = 1.0;

        // MIN_FACTOR → 1.0 으로 서서히 증가
        factor = MIN_FACTOR + (1.0 - MIN_FACTOR) * u;
    }

    // 최종 적용 속도: (난이도/스쿠터 등 기본 속도) * (교수 디버프 계수) 
    player->move_speed = base_speed * factor;

    return 1; // 이동은 그대로 진행
}


int pattern_stage_6(Stage *stage, Obstacle *prof,  Player *player, double delta_time)
{
    return 1;
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

int update_professor_pattern(Stage *stage, Obstacle *prof,  Player *player, double delta_time) {
    int id = stage->id;
    
    // 안전장치
    if (id < 1 || id > 6) return 0;

    // 해당 스테이지 함수 호출
    if (kPatterns[id]) {
        return kPatterns[id](stage, prof, player, delta_time);
    }
    return 1;
}



