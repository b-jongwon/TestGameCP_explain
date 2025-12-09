// src/professor_patterns.c

#include "../include/professor_pattern.h"
#include "../include/sound.h"
#include "../include/player.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef int (*PatternFunc)(Stage *, Obstacle *, Player *, double);

enum
{
    B1_STATE_IDLE = 0,
    B1_STATE_PREPARE,
    B1_STATE_SKILL_A,
    B1_STATE_SKILL_B
};

static const double kB1PrepareDuration = 2.0;
static const double kB1SkillAToBDelay = 4.0;
static const double kB1SkillBDuration = 2.0;
static const double kB1SkillACloneLifetime = 6.0;
static const double kB1SkillBCloneLifetime = 2.0;
static const int kB1ClonesPerCast = 16;
static const char *kB1ContactSfx = "bgm/Professor_b1_contact.wav";
static const char *kB1SkillASfx = "bgm/Professor_b1_illusion.wav";
static const char *kB1SkillBSfx = "bgm/Professor_b1_2power2.wav";

enum
{
    STAGE3_STATE_WAIT = 0,
    STAGE3_STATE_FIRING = 1
};

static const double kStage3BurstInterval = 2.5;
static const double kStage3ShotSpacing = 0.3;
static const int kStage3ShotsPerBurst = 3;
static const double kStage3BulletSpeed = 6.0;
static const double kStage3BulletLifetime = 4.0;

static void clear_professor_clones(Stage *stage)
{
    if (!stage)
    {
        return;
    }
    memset(stage->professor_clones, 0, sizeof(stage->professor_clones));
    stage->num_professor_clones = 0;
}

static void decay_professor_clones(Stage *stage, double delta_time)
{
    if (!stage)
    {
        return;
    }

    if (delta_time < 0.0)
    {
        delta_time = 0.0;
    }

    int alive = 0;
    for (int i = 0; i < MAX_PROFESSOR_CLONES; ++i)
    {
        ProfessorClone *clone = &stage->professor_clones[i];
        if (!clone->active)
        {
            continue;
        }

        if (delta_time > 0.0)
        {
            clone->remaining_time -= delta_time;
            if (clone->remaining_time <= 0.0)
            {
                clone->active = 0;
                continue;
            }
        }

        alive++;
    }

    stage->num_professor_clones = alive;
}

static int tile_overlaps_player(const Player *player, int tx, int ty)
{
    if (!player)
    {
        return 0;
    }

    const int tile_size = SUBPIXELS_PER_TILE;
    int tile_left = tx * tile_size;
    int tile_right = tile_left + tile_size;
    int tile_top = ty * tile_size;
    int tile_bottom = tile_top + tile_size;

    if (player->world_x + tile_size <= tile_left ||
        player->world_x >= tile_right ||
        player->world_y + tile_size <= tile_top ||
        player->world_y >= tile_bottom)
    {
        return 0;
    }
    return 1;
}

static int tile_has_professor_clone(const Stage *stage, int tx, int ty)
{
    if (!stage)
    {
        return 0;
    }
    for (int i = 0; i < MAX_PROFESSOR_CLONES; ++i)
    {
        const ProfessorClone *clone = &stage->professor_clones[i];
        if (clone->active && clone->tile_x == tx && clone->tile_y == ty)
        {
            return 1;
        }
    }
    return 0;
}

static int tile_has_obstacle(const Stage *stage, int tx, int ty)
{
    if (!stage)
    {
        return 0;
    }
    for (int i = 0; i < stage->num_obstacles; ++i)
    {
        const Obstacle *o = &stage->obstacles[i];
        if (!o->active)
        {
            continue;
        }
        int ox = o->world_x / SUBPIXELS_PER_TILE;
        int oy = o->world_y / SUBPIXELS_PER_TILE;
        if (ox == tx && oy == ty)
        {
            return 1;
        }
    }
    return 0;
}

static int tile_has_item(const Stage *stage, int tx, int ty)
{
    if (!stage)
    {
        return 0;
    }
    for (int i = 0; i < stage->num_items; ++i)
    {
        const Item *it = &stage->items[i];
        if (!it->active)
        {
            continue;
        }
        int ix = it->world_x / SUBPIXELS_PER_TILE;
        int iy = it->world_y / SUBPIXELS_PER_TILE;
        if (ix == tx && iy == ty)
        {
            return 1;
        }
    }
    return 0;
}

static int add_professor_clone(Stage *stage, int tx, int ty, double ttl)
{
    if (!stage)
    {
        return 0;
    }
    for (int i = 0; i < MAX_PROFESSOR_CLONES; ++i)
    {
        ProfessorClone *clone = &stage->professor_clones[i];
        if (clone->active)
        {
            continue;
        }

        clone->tile_x = tx;
        clone->tile_y = ty;
        clone->remaining_time = ttl;
        clone->active = 1;
        stage->num_professor_clones++;
        return 1;
    }
    return 0;
}

static int spawn_professor_clones(Stage *stage, const Player *player, const Obstacle *prof,
                                  int desired_count, double ttl)
{
    if (!stage || desired_count <= 0)
    {
        return 0;
    }

    if (stage->num_passable_tiles <= 0)
    {
        return 0;
    }

    short candidate_indices[MAX_PASSABLE_TILES];
    int available = 0;

    for (int i = 0; i < stage->num_passable_tiles; ++i)
    {
        int x = stage->passable_tiles[i].x;
        int y = stage->passable_tiles[i].y;

        if (tile_overlaps_player(player, x, y))
        {
            continue;
        }
        if (tile_has_obstacle(stage, x, y))
        {
            continue;
        }
        if (tile_has_item(stage, x, y))
        {
            continue;
        }
        if (tile_has_professor_clone(stage, x, y))
        {
            continue;
        }

        candidate_indices[available++] = (short)i;
        if (available >= MAX_PASSABLE_TILES)
        {
            break;
        }
    }

    int created = 0;
    while (created < desired_count && available > 0)
    {
        int pick = rand() % available;
        int index = candidate_indices[pick];
        TileCoord chosen = stage->passable_tiles[index];
        if (add_professor_clone(stage, chosen.x, chosen.y, ttl))
        {
            created++;
        }

        candidate_indices[pick] = candidate_indices[available - 1];
        available--;
    }

    (void)prof;
    return created;
}

static void cast_b1_skill_a(Stage *stage, const Player *player, const Obstacle *prof)
{
    spawn_professor_clones(stage, player, prof, kB1ClonesPerCast, kB1SkillACloneLifetime);
    play_sfx_nonblocking(kB1SkillASfx);
}

static void cast_b1_skill_b(Stage *stage, const Player *player, const Obstacle *prof)
{
    spawn_professor_clones(stage, player, prof, kB1ClonesPerCast, kB1SkillBCloneLifetime);
    play_sfx_nonblocking(kB1SkillBSfx);
}

static ProfessorBullet *acquire_professor_bullet_slot(Stage *stage)
{
    if (!stage)
    {
        return NULL;
    }

    for (int i = 0; i < MAX_PROFESSOR_BULLETS; ++i)
    {
        ProfessorBullet *slot = &stage->professor_bullets[i];
        if (!slot->active)
        {
            return slot;
        }
    }
    return NULL;
}

static void spawn_stage3_bullet(Stage *stage, const Obstacle *prof, const Player *player)
{
    if (!stage || !prof || !player)
    {
        return;
    }

    ProfessorBullet *slot = acquire_professor_bullet_slot(stage);
    if (!slot)
    {
        return;
    }

    const double tile_scale = (double)SUBPIXELS_PER_TILE;
    double origin_x = ((double)prof->world_x + tile_scale / 2.0) / tile_scale;
    double origin_y = ((double)prof->world_y + tile_scale / 2.0) / tile_scale;
    double player_center_x = ((double)player->world_x + tile_scale / 2.0) / tile_scale;
    double player_center_y = ((double)player->world_y + tile_scale / 2.0) / tile_scale;

    double dir_x = player_center_x - origin_x;
    double dir_y = player_center_y - origin_y;
    double len = hypot(dir_x, dir_y);
    if (len < 1e-5)
    {
        dir_x = 0.0;
        dir_y = 1.0;
        len = 1.0;
    }

    dir_x /= len;
    dir_y /= len;

    slot->world_x = origin_x - 0.5;
    slot->world_y = origin_y - 0.5;
    slot->vel_x = dir_x * kStage3BulletSpeed;
    slot->vel_y = dir_y * kStage3BulletSpeed;
    slot->remaining_time = kStage3BulletLifetime;
    slot->active = 1;
    if (stage->num_professor_bullets < MAX_PROFESSOR_BULLETS)
    {
        stage->num_professor_bullets++;
    }
}

ProfessorBulletResult update_professor_bullets(Stage *stage, Player *player, double delta_time)
{
    if (!stage)
    {
        return PROFESSOR_BULLET_RESULT_NONE;
    }

    if (delta_time < 0.0)
    {
        delta_time = 0.0;
    }

    ProfessorBulletResult result = PROFESSOR_BULLET_RESULT_NONE;
    int active_count = 0;
    int width = (stage->width > 0) ? stage->width : MAX_X;
    int height = (stage->height > 0) ? stage->height : MAX_Y;

    for (int i = 0; i < MAX_PROFESSOR_BULLETS; ++i)
    {
        ProfessorBullet *bullet = &stage->professor_bullets[i];
        if (!bullet->active)
        {
            continue;
        }

        if (delta_time > 0.0)
        {
            bullet->world_x += bullet->vel_x * delta_time;
            bullet->world_y += bullet->vel_y * delta_time;
        }

        bullet->remaining_time -= delta_time;
        if (bullet->remaining_time <= 0.0)
        {
            bullet->active = 0;
            continue;
        }

        double center_x = bullet->world_x + 0.5;
        double center_y = bullet->world_y + 0.5;
        int tile_x = (int)floor(center_x);
        int tile_y = (int)floor(center_y);
        if (tile_x < 0 || tile_y < 0 || tile_x >= width || tile_y >= height)
        {
            bullet->active = 0;
            continue;
        }

        if (is_tile_impassable_char(stage->map[tile_y][tile_x]))
        {
            bullet->active = 0;
            continue;
        }

        if (player)
        {
            int center_world_x = (int)lround(center_x * SUBPIXELS_PER_TILE);
            int center_world_y = (int)lround(center_y * SUBPIXELS_PER_TILE);
            if (is_world_point_inside_player(player, center_world_x, center_world_y))
            {
                bullet->active = 0;
                if (player->shield_count > 0)
                {
                    player->shield_count--;
                    if (result != PROFESSOR_BULLET_RESULT_FATAL)
                    {
                        result = PROFESSOR_BULLET_RESULT_SHIELD_BLOCKED;
                    }
                }
                else
                {
                    result = PROFESSOR_BULLET_RESULT_FATAL;
                }
                continue;
            }
        }

        active_count++;
    }

    stage->num_professor_bullets = active_count;
    return result;
}

int pattern_stage_b1(Stage *stage, Obstacle *prof, Player *player, double delta_time)
{
    if (!stage || !prof || !player)
    {
        return 1;
    }

    if (delta_time < 0.0)
    {
        delta_time = 0.0;
    }

    decay_professor_clones(stage, delta_time);

    if (!player->has_backpack)
    {
        prof->alert = 0;
        if (prof->p_state != B1_STATE_IDLE)
        {
            clear_professor_clones(stage);
        }
        prof->p_state = B1_STATE_IDLE;
        prof->p_timer = 0.0;
        prof->p_misc = 0;
        return 0; // 가방 획득 전에는 움직임 금지
    }

    prof->alert = 1;

    if (!prof->p_misc)
    {
        play_sfx_nonblocking(kB1ContactSfx);
        prof->p_misc = 1;
    }

    if (prof->p_state == B1_STATE_IDLE)
    {
        prof->p_state = B1_STATE_PREPARE;
        prof->p_timer = 0.0;
    }

    switch (prof->p_state)
    {
    case B1_STATE_PREPARE:
        prof->p_timer += delta_time;
        if (prof->p_timer >= kB1PrepareDuration)
        {
            prof->p_timer -= kB1PrepareDuration;
            cast_b1_skill_a(stage, player, prof);
            prof->p_state = B1_STATE_SKILL_A;
        }
        break;
    case B1_STATE_SKILL_A:
        prof->p_timer += delta_time;
        if (prof->p_timer >= kB1SkillAToBDelay)
        {
            prof->p_timer -= kB1SkillAToBDelay;
            cast_b1_skill_b(stage, player, prof);
            prof->p_state = B1_STATE_SKILL_B;
        }
        break;
    case B1_STATE_SKILL_B:
        prof->p_timer += delta_time;
        if (prof->p_timer >= kB1SkillBDuration)
        {
            prof->p_timer -= kB1SkillBDuration;
            clear_professor_clones(stage);
            prof->p_state = B1_STATE_PREPARE;
        }
        break;
    default:
        prof->p_state = B1_STATE_PREPARE;
        prof->p_timer = 0.0;
        break;
    }

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

    // 🚨 2단계 발각 사운드 파일 경로
    const char *PROF_LV6_SFX_PATH = "bgm/Professor_lv2.wav";

    // -------------------------------------------------------------
    // 2. 발각 사운드 재생 로직 (첫 발견 시 1회 실행)
    // -------------------------------------------------------------
    if (prof->alert && prof->p_timer == 0.0)
    {
        play_sfx_nonblocking(PROF_LV6_SFX_PATH);

        // p_timer를 0.1로 설정하여 다음 프레임에 중복 실행을 방지합니다.
        prof->p_timer = 0.1;
    }
    else if (!prof->alert)
    {
        // 미발견 상태로 돌아가면 타이머를 0으로 리셋합니다.
        prof->p_timer = 0.0;
    }

    // -------------------------------------------------------------
    // 3. 시야 차단/혼란 로직 (발견 즉시)
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
    if (!stage || !prof || !player)
    {
        return 1;
    }

    if (!prof->alert)
    {
        prof->p_state = STAGE3_STATE_WAIT;
        prof->p_timer = 0.0;
        prof->p_counter = 0;
        return 1;
    }

    if (delta_time < 0.0)
    {
        delta_time = 0.0;
    }

    if (prof->p_state != STAGE3_STATE_WAIT && prof->p_state != STAGE3_STATE_FIRING)
    {
        prof->p_state = STAGE3_STATE_WAIT;
        prof->p_timer = 0.0;
        prof->p_counter = 0;
    }

    if (prof->p_state == STAGE3_STATE_WAIT)
    {
        prof->p_timer += delta_time;
        if (prof->p_timer >= kStage3BurstInterval)
        {
            prof->p_timer = 0.0;
            prof->p_counter = 0;
            prof->p_state = STAGE3_STATE_FIRING;
        }
        return 1;
    }

    prof->p_timer -= delta_time;
    while (prof->p_timer <= 0.0 && prof->p_counter < kStage3ShotsPerBurst)
    {
        spawn_stage3_bullet(stage, prof, player);
        prof->p_counter++;
        prof->p_timer += kStage3ShotSpacing;
    }

    if (prof->p_counter >= kStage3ShotsPerBurst)
    {
        prof->p_state = STAGE3_STATE_WAIT;
        prof->p_timer = 0.0;
    }

    return 0;
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

   
    const char *PROF_LV5_SFX_PATH = "bgm/Professor_lv5.wav";

    if (prof->alert && prof->p_timer == 0.0)
    {
        play_sfx_nonblocking(PROF_LV5_SFX_PATH);

        prof->p_timer = 0.1;
    }
    else if (!prof->alert)
    {
        prof->p_timer = 0.0;
    }

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
    if (!prof)
        return 1;

    // 🚨 6단계 발각 사운드 파일 경로
    const char *PROF_LV6_SFX_PATH = "bgm/Professor_lv6.wav";

    // -------------------------------------------------------------
    // 1. 발각 사운드 재생 로직 (첫 발견 시 1회 실행)
    // -------------------------------------------------------------
    if (prof->alert && prof->p_timer == 0.0)
    {
        play_sfx_nonblocking(PROF_LV6_SFX_PATH);

        // p_timer를 0.1로 설정하여 다음 프레임에 중복 실행을 방지합니다.
        prof->p_timer = 0.1;
    }
    else if (!prof->alert)
    {
        // 미발견 상태로 돌아가면 타이머를 0으로 리셋합니다.
        prof->p_timer = 0.0;
    }

    // 1~5의 패턴을 모두 적용
    int p1 = pattern_stage_b1(stage, prof, player, dt);
    int p2 = pattern_stage_1f(stage, prof, player, dt);
    int p3 = pattern_stage_2f(stage, prof, player, dt);
    int p4 = pattern_stage_3f(stage, prof, player, dt);
    int p5 = pattern_stage_4f(stage, prof, player, dt);

    return (p1 && p2 && p3 && p4 && p5);
}

static const PatternFunc kPatterns[] = {
    NULL,             // 0 (Not used)
    pattern_stage_b1, // Stage 1
    pattern_stage_1f, // Stage 2
    pattern_stage_2f, // Stage 3
    pattern_stage_3f, // Stage 4
    pattern_stage_4f, // Stage 5
    pattern_stage_5f  // Stage 6
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
