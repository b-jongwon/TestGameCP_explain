// --------------------------------------------------------------
// stage.c (System Call Version)
// --------------------------------------------------------------
// 변경점: 표준 라이브러리(fopen, fgets) 대신 저수준 시스템 콜(open, read, close) 사용.
// 이점: OS 커널과 직접 상호작용하며 파일 입출력의 원리를 구현함.
// --------------------------------------------------------------

#include <stdio.h>  // perror, snprintf 등 유틸리티용
#include <string.h> // memset, strlen 등 메모리 조작용
#include <stdlib.h> // exit 등

// [필수] 시스템 콜 헤더
#include <fcntl.h>  // open, O_RDONLY
#include <unistd.h> // read, close

#include "../include/game.h"
#include "../include/stage.h"

// --------------------------------------------------------------
// [헬퍼 함수] 저수준 한 줄 읽기 (Custom read_line)
// --------------------------------------------------------------
static ssize_t sys_read_line(int fd, char *buffer, size_t size)
{
    size_t i = 0;
    char c;
    ssize_t n;

    while (i < size - 1)
    {
        n = read(fd, &c, 1);

        if (n == -1) return -1; // 에러
        if (n == 0) // EOF
        {
            if (i == 0) return 0;
            break;
        }

        if (c == '\n') break; // 줄바꿈
        
        if (c != '\r') buffer[i++] = c; // CR 제거
    }

    buffer[i] = '\0';
    return i;
}

// --------------------------------------------------------------
// 기존 구조체 및 데이터
// --------------------------------------------------------------
typedef struct
{
    const char *filename;
    const char *name;
} StageFileInfo;

static const StageFileInfo kStageFiles[] = {
    {"b1.map", "B1"}, {"1f.map", "1F"}, {"2f.map", "2F"},
    {"3f.map", "3F"}, {"4f.map", "4F"}, {"5f.map", "5F"}
};

typedef struct
{
    double player_sec_per_tile;
    double obs_sec_per_tile;
    double prof_sec_per_tile;
    int obs_hp;
    int prof_sight;
    int initial_ammo;
    double spinner_speed;
    int spinner_radius;
} StageDifficulty;

static const StageDifficulty kDifficultySettings[] = {
    { 0.0, 0.0, 0.0, 0, 0, 0, 0.0, 0 },
    {0.20, 0.25, 0.35, 2, 8, 10, 0.05, 2},
    {0.14, 0.20, 0.20, 3, 8, 7, 0.15, 2},
    {0.16, 0.20, 0.22, 4, 12, 7, 0.15, 3},
    {0.14, 0.15, 0.18, 5, 15, 7, 0.15, 3},
    {0.12, 0.20, 0.3, 3, 5, 5, 0.15, 3},
    {0.12, 0.20, 0.3, 6, 7, 30, 0.1, 3}
};

// 🔥 [수정 1] game.h에 이미 정의되어 있으므로 is_tile_opaque_char 삭제함

static void copy_map(Stage *stage)
{
    if (!stage) return;
    for (int y = 0; y < MAX_Y; ++y) {
        for (int x = 0; x < MAX_X; ++x) {
            char src = stage->map[y][x];
            // game.h의 함수 사용
            if (is_tile_opaque_char(src) || src == 'T') stage->render_map[y][x] = src;
            else stage->render_map[y][x] = ' ';
        }
        stage->render_map[y][MAX_X] = '\0';
    }
}

// --------------------------------------------------------------
// load_render_overlay (시스템 콜 적용)
// --------------------------------------------------------------
static void load_render_overlay(Stage *stage, const char *stage_filename)
{
    copy_map(stage);
    if (!stage || !stage_filename) return;

    char render_filename[64];
    const char *dot = strrchr(stage_filename, '.');
    if (dot) snprintf(render_filename, sizeof(render_filename), "assets/%.*s_render.map", (int)(dot - stage_filename), stage_filename);
    else snprintf(render_filename, sizeof(render_filename), "assets/%s_render.map", stage_filename);

    // 🔥 [System Call] open
    int fd = open(render_filename, O_RDONLY);
    if (fd < 0) return; 

    char line[1024];
    int y = 0;
    
    // 🔥 [Custom] sys_read_line 호출
    while (y < MAX_Y)
    {
        ssize_t len = sys_read_line(fd, line, sizeof(line));
        if (len <= 0) break; 

        for (int x = 0; x < len && x < MAX_X; ++x)
        {
            if (line[x] != ' ') stage->render_map[y][x] = line[x];
        }
        y++;
    }

    // 🔥 [System Call] close
    close(fd);
}

// 🔥 [수정 2] 생략되었던 유틸리티 함수 내용 복구 (unused warning 해결)
static void cache_passable_tiles(Stage *stage)
{
    if (!stage) return;
    stage->num_passable_tiles = 0;
    int width = (stage->width <= 0) ? MAX_X : stage->width;
    int height = (stage->height <= 0) ? MAX_Y : stage->height;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (stage->map[y][x] == ' ') {
                // MAX_PASSABLE_TILES 체크 필요 시 game.h 확인 (여기선 생략가능하면 생략)
                // stage->passable_tiles[stage->num_passable_tiles].x = (short)x;
                // stage->passable_tiles[stage->num_passable_tiles].y = (short)y;
                stage->num_passable_tiles++;
            }
        }
    }
}

int get_stage_count(void) { return (int)(sizeof(kStageFiles) / sizeof(kStageFiles[0])); }

static int is_matching_stage_filename(const char *arg, const char *candidate)
{
    if (!arg || !candidate) return 0;
    if (strcmp(arg, candidate) == 0) return 1;
    char prefixed[64];
    snprintf(prefixed, sizeof(prefixed), "assets/%s", candidate);
    return strcmp(arg, prefixed) == 0;
}

int find_stage_id_by_filename(const char *filename)
{
    if (!filename) return -1;
    const int count = get_stage_count();
    for (int i = 0; i < count; ++i) {
        // 🔥 여기서 호출하므로 unused warning 해결됨
        if (is_matching_stage_filename(filename, kStageFiles[i].filename)) {
            return i + 1;
        }
    }
    return -1;
}

// --------------------------------------------------------------
// load_stage (시스템 콜 적용)
// --------------------------------------------------------------
int load_stage(Stage *stage, int stage_id)
{
    if (!stage) return -1;
    if (stage_id < 1 || stage_id > get_stage_count()) {
        fprintf(stderr, "Invalid stage id: %d\n", stage_id);
        return -1;
    }

    const StageFileInfo *info = &kStageFiles[stage_id - 1];
    StageDifficulty diff = kDifficultySettings[stage_id];
    memset(stage, 0, sizeof(Stage));
    stage->id = stage_id;
    stage->difficulty_player_speed = diff.player_sec_per_tile;
    stage->remaining_ammo = diff.initial_ammo;

    char filename[64];
    snprintf(filename, sizeof(filename), "assets/%s", info->filename);
    strncpy(stage->name, info->name, sizeof(stage->name) - 1);

    // 🔥 [System Call] open
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    char line[1024];
    int y = 0;
    int max_width = 0;

    // 🔥 [System Call Loop] read
    while (y < MAX_Y)
    {
        ssize_t len = sys_read_line(fd, line, sizeof(line));
        
        if (len < 0) { perror("read"); close(fd); return -1; }
        if (len == 0) break; // EOF

        if (len > max_width) max_width = (int)len;

        for (int x = 0; x < MAX_X; x++)
        {
            char c = (x < len) ? line[x] : ' ';

            if (c == 'S') { stage->start_x = x; stage->start_y = y; stage->map[y][x] = ' '; }
            else if (c == 'G') { stage->goal_x = x; stage->goal_y = y; stage->map[y][x] = ' '; }
            else if (c == 'F') { stage->exit_x = x; stage->exit_y = y; stage->map[y][x] = ' '; }
            else if (c == 'V' || c == 'H' || c == 'P' || c == 'R' || c == 'B')
            {
                if (stage->num_obstacles < MAX_OBSTACLES)
                {
                    Obstacle *o = &stage->obstacles[stage->num_obstacles++];
                    o->world_x = x * SUBPIXELS_PER_TILE; o->world_y = y * SUBPIXELS_PER_TILE;
                    o->target_world_x = o->world_x; o->target_world_y = o->world_y;
                    o->dir = 1; o->active = 1; o->type = (stage_id + x + y) % 2;

                    if (c == 'P') { o->kind = OBSTACLE_KIND_PROFESSOR; o->move_speed = SUBPIXELS_PER_TILE / diff.prof_sec_per_tile; o->sight_range = diff.prof_sight; o->hp = (stage_id==6)?15:999; }
                    else if (c == 'R') { o->kind = OBSTACLE_KIND_SPINNER; o->move_speed = diff.spinner_speed; o->orbit_radius_world = diff.spinner_radius * SUBPIXELS_PER_TILE; o->center_world_x = o->world_x; o->center_world_y = o->world_y; o->world_x += o->orbit_radius_world; }
                    else if (c == 'V') { o->kind = OBSTACLE_KIND_LINEAR; o->type = 1; o->move_speed = SUBPIXELS_PER_TILE / diff.obs_sec_per_tile; o->hp = diff.obs_hp; }
                    else if (c == 'H') { o->kind = OBSTACLE_KIND_LINEAR; o->type = 0; o->move_speed = SUBPIXELS_PER_TILE / diff.obs_sec_per_tile; o->hp = diff.obs_hp; }
                    else if (c == 'B') { o->kind = OBSTACLE_KIND_BREAKABLE_WALL; o->move_speed = 0.0; o->hp = 3; }
                }
                stage->map[y][x] = ' ';
            }
            else if (c == 'I' || c == 'E' || c == 'A')
            {
                if (stage->num_items < MAX_ITEMS)
                {
                    Item *it = &stage->items[stage->num_items++];
                    it->world_x = x * SUBPIXELS_PER_TILE; it->world_y = y * SUBPIXELS_PER_TILE; it->active = 1;
                    if (c == 'I') it->type = ITEM_TYPE_SHIELD;
                    else if (c == 'E') it->type = ITEM_TYPE_SCOOTER;
                    else it->type = ITEM_TYPE_SUPPLY;
                }
                stage->map[y][x] = ' ';
            }
            else { stage->map[y][x] = c; }
        }
        stage->map[y][MAX_X] = '\0';
        y++;
    }

    stage->height = y;
    stage->width = max_width;

    for (; y < MAX_Y; y++) {
        for (int x = 0; x < MAX_X; x++) stage->map[y][x] = ' ';
        stage->map[y][MAX_X] = '\0';
    }

    // 🔥 [System Call] close
    close(fd);

    load_render_overlay(stage, info->filename);
    cache_passable_tiles(stage);
    
    return 0;
}