// ============================================================================
// render.c
// ----------------------------------------------------------------------------
// SDL2 + SDL2_image를 이용해 타일 기반 맵을 이미지로 렌더링한다.
// Stage.map에 있는 문자 정보를 실제 PNG 텍스처와 매핑하여 화면에 표시한다.
// ============================================================================

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "../include/game.h"
#include "../include/render.h"

#define TILE_SIZE 32

typedef enum
{
    PLAYER_VARIANT_NORMAL = 0,
    PLAYER_VARIANT_BACKPACK,
    PLAYER_VARIANT_SCOOTER,
    PLAYER_VARIANT_SCOOTER_BACKPACK,
    PLAYER_VARIANT_COUNT
} PlayerVariant;

typedef enum
{
    PLAYER_FRAME_STAND_A = 0,
    PLAYER_FRAME_STAND_B,
    PLAYER_FRAME_STEP_A,
    PLAYER_FRAME_STEP_B,
    PLAYER_FRAME_COUNT
} PlayerFrame;

typedef struct
{
    const char *stand_a;
    const char *stand_b;
    const char *step_a;
    const char *step_b;
} PlayerTextureSet;

static const PlayerTextureSet PLAYER_TEXTURE_PATHS[PLAYER_VARIANT_COUNT][PLAYER_FACING_COUNT] = {
    [PLAYER_VARIANT_NORMAL] = {
        [PLAYER_FACING_DOWN] = {
            "assets/image/player/foward_stand.png",
            "assets/image/player/foward_stand.png",
            "assets/image/player/foward_left.PNG",
            "assets/image/player/foward_right.png"},
        [PLAYER_FACING_UP] = {"assets/image/player/back_stand_1.PNG", "assets/image/player/back_stand_2.png", "assets/image/player/back_left.PNG", "assets/image/player/back_right.PNG"},
        [PLAYER_FACING_LEFT] = {"assets/image/player/left_stand.PNG", "assets/image/player/left_stand.PNG", "assets/image/player/left_left.png", "assets/image/player/left_right.png"},
        [PLAYER_FACING_RIGHT] = {"assets/image/player/right_stand.png", "assets/image/player/right_stand.png", "assets/image/player/right_left.PNG", "assets/image/player/right_right.PNG"}},
    [PLAYER_VARIANT_BACKPACK] = {[PLAYER_FACING_DOWN] = {"assets/image/player_backpack/foward_stand.png", "assets/image/player_backpack/foward_stand.png", "assets/image/player_backpack/foward_left.png", "assets/image/player_backpack/foward_right.png"}, [PLAYER_FACING_UP] = {"assets/image/player_backpack/back_stand_1.png", "assets/image/player_backpack/back_stand_2.png", "assets/image/player_backpack/back_left.png", "assets/image/player_backpack/back_right.png"}, [PLAYER_FACING_LEFT] = {"assets/image/player_backpack/left_stand.PNG", "assets/image/player_backpack/left_stand.PNG", "assets/image/player_backpack/left_left.png", "assets/image/player_backpack/left_right.png"}, [PLAYER_FACING_RIGHT] = {"assets/image/player_backpack/right_stand.png", "assets/image/player_backpack/right_stand.png", "assets/image/player_backpack/right_left.png", "assets/image/player_backpack/right_right.png"}},
    [PLAYER_VARIANT_SCOOTER] = {[PLAYER_FACING_DOWN] = {"assets/image/player_scooter/forward_stand.PNG", "assets/image/player_scooter/forward_stand.PNG", "assets/image/player_scooter/forward_stand.PNG", "assets/image/player_scooter/forward_stand.PNG"}, [PLAYER_FACING_UP] = {"assets/image/player_scooter/back_stand_1.PNG", "assets/image/player_scooter/back_stand_2.png", "assets/image/player_scooter/back_stand_1.PNG", "assets/image/player_scooter/back_stand_2.png"}, [PLAYER_FACING_LEFT] = {"assets/image/player_scooter/left_stand.PNG", "assets/image/player_scooter/left_stand.PNG", "assets/image/player_scooter/left_stand.PNG", "assets/image/player_scooter/left_stand.PNG"}, [PLAYER_FACING_RIGHT] = {"assets/image/player_scooter/right_stand.png", "assets/image/player_scooter/right_stand.png", "assets/image/player_scooter/right_stand.png", "assets/image/player_scooter/right_stand.png"}},
    [PLAYER_VARIANT_SCOOTER_BACKPACK] = {[PLAYER_FACING_DOWN] = {"assets/image/player_scooter_backpack/front_stand.PNG", "assets/image/player_scooter_backpack/front_stand.PNG", "assets/image/player_scooter_backpack/front_stand.PNG", "assets/image/player_scooter_backpack/front_stand.PNG"}, [PLAYER_FACING_UP] = {"assets/image/player_scooter_backpack/back_stand_1.png", "assets/image/player_scooter_backpack/back_stand_2.PNG", "assets/image/player_scooter_backpack/back_stand_1.png", "assets/image/player_scooter_backpack/back_stand_2.PNG"}, [PLAYER_FACING_LEFT] = {"assets/image/player_scooter_backpack/left_stand.PNG", "assets/image/player_scooter_backpack/left_stand.PNG", "assets/image/player_scooter_backpack/left_stand.PNG", "assets/image/player_scooter_backpack/left_stand.PNG"}, [PLAYER_FACING_RIGHT] = {"assets/image/player_scooter_backpack/right_stand.png", "assets/image/player_scooter_backpack/right_stand.png", "assets/image/player_scooter_backpack/right_stand.png", "assets/image/player_scooter_backpack/right_stand.png"}}};

static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture *g_tex_floor = NULL;
static SDL_Texture *g_tex_wall = NULL;
static SDL_Texture *g_tex_goal = NULL;

static SDL_Texture *g_tex_professor_1 = NULL; // 스테이지별 교수님
static SDL_Texture *g_tex_professor_2 = NULL;
static SDL_Texture *g_tex_professor_3 = NULL;
static SDL_Texture *g_tex_professor_4 = NULL;
static SDL_Texture *g_tex_professor_5 = NULL;
static SDL_Texture *g_tex_professor_6 = NULL;

static SDL_Texture *g_tex_spinner = NULL;  // R (스피너)
static SDL_Texture *g_tex_obstacle = NULL; // X (일반 장애물)

static SDL_Texture *g_tex_item_shield = NULL;  // 필드에 놓인 쉴드 아이템(I)
static SDL_Texture *g_tex_item_scooter = NULL; // E-scooter 아이템(E)
static SDL_Texture *g_tex_item_supply = NULL;  // 투사체 보충 아이템

static SDL_Texture *g_tex_projectile = NULL; // 투사체
static SDL_Texture *g_tex_shield_on = NULL;  // 플레이어 보호막 활성화

static SDL_Texture *g_tex_trap = NULL;       // 트랩
static SDL_Texture *g_tex_wall_break = NULL; // 깨지는 벽

static SDL_Texture *g_tex_exit = NULL;

static SDL_Texture *g_player_textures[PLAYER_VARIANT_COUNT][PLAYER_FACING_COUNT][PLAYER_FRAME_COUNT] = {{{NULL}}};
static int g_window_w = 0;
static int g_window_h = 0;

static int is_tile_blocking_vision(const Stage *stage, int x, int y)
{
    if (!stage)
        return 1;

    int width = (stage->width > 0) ? stage->width : MAX_X;
    int height = (stage->height > 0) ? stage->height : MAX_Y;
    if (x < 0 || y < 0 || x >= width || y >= height)
        return 1;

    char cell = stage->map[y][x];
    return (cell == '#' || cell == '@');
}

static int has_line_of_sight(const Stage *stage, int start_x, int start_y, int target_x, int target_y)
{
    if (!stage)
        return 0;

    int width = (stage->width > 0) ? stage->width : MAX_X;
    int height = (stage->height > 0) ? stage->height : MAX_Y;
    if (target_x < 0 || target_y < 0 || target_x >= width || target_y >= height)
        return 0;

    int x0 = start_x;
    int y0 = start_y;
    int x1 = target_x;
    int y1 = target_y;
    int dx = abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (1)
    {
        if (!(x0 == start_x && y0 == start_y) && (x0 != x1 || y0 != y1))
        {
            if (is_tile_blocking_vision(stage, x0, y0))
            {
                return 0;
            }
        }

        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }

    return 1;
}

static void compute_visibility(const Stage *stage, const Player *player, unsigned char visibility[MAX_Y][MAX_X])
{
    int width = (stage && stage->width > 0) ? stage->width : MAX_X;
    int height = (stage && stage->height > 0) ? stage->height : MAX_Y;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            visibility[y][x] = 0;
        }
    }

    if (!stage || !player)
    {
        return;
    }

    int start_x = player->world_x / SUBPIXELS_PER_TILE;
    int start_y = player->world_y / SUBPIXELS_PER_TILE;
    if (start_x < 0)
        start_x = 0;
    if (start_y < 0)
        start_y = 0;
    if (start_x >= width)
        start_x = width - 1;
    if (start_y >= height)
        start_y = height - 1;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            visibility[y][x] = has_line_of_sight(stage, start_x, start_y, x, y);
        }
    }
}

static SDL_Texture *load_texture(const char *path)
{
    SDL_Surface *surface = IMG_Load(path);
    if (!surface)
    {
        fprintf(stderr, "IMG_Load failed for %s: %s\n", path, IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(g_renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture)
    {
        fprintf(stderr, "SDL_CreateTextureFromSurface failed for %s: %s\n", path, SDL_GetError());
    }

    return texture;
}

static void destroy_texture(SDL_Texture **texture)
{
    if (texture && *texture)
    {
        SDL_DestroyTexture(*texture);
        *texture = NULL;
    }
}

static void draw_texture_with_pixel_offset(SDL_Texture *texture, int x, int y, int offset_x, int offset_y)
{
    if (!texture)
    {
        return;
    }

    SDL_Rect dst = {x * TILE_SIZE + offset_x, y * TILE_SIZE + offset_y, TILE_SIZE, TILE_SIZE};
    SDL_RenderCopy(g_renderer, texture, NULL, &dst);
}

static void draw_texture_at_world(SDL_Texture *texture, double world_x, double world_y)
{
    if (!texture)
        return;

    SDL_Rect dst = {(int)lround(world_x * TILE_SIZE), (int)lround(world_y * TILE_SIZE), TILE_SIZE, TILE_SIZE};
    SDL_RenderCopy(g_renderer, texture, NULL, &dst);
}

static void draw_texture_scaled(SDL_Texture *texture, double world_x, double world_y, double scale)
{
    if (!texture)
    {
        return;
    }

    const int width = (int)lround(TILE_SIZE * scale);
    const int height = (int)lround(TILE_SIZE * scale);

    const int offset_x = (TILE_SIZE - width) / 2;
    const int offset_y = (TILE_SIZE - height) / 2;

    SDL_Rect dst = {(int)lround(world_x * TILE_SIZE) + offset_x,
                    (int)lround(world_y * TILE_SIZE) + offset_y,
                    width,
                    height};
    SDL_RenderCopy(g_renderer, texture, NULL, &dst);
}

static int compute_vertical_bounce_offset(double elapsed_time)
{
    const double speed = 6.0;
    const double amplitude = 2.0;
    double wave = sin(elapsed_time * speed);
    return (int)lround(wave * amplitude);
}

int init_renderer(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    const int initial_w = 800;
    const int initial_h = 600;
    g_window = SDL_CreateWindow("Professor Dodge",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                initial_w,
                                initial_h,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!g_window)
    {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer)
    {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        g_window = NULL;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    g_window_w = initial_w;
    g_window_h = initial_h;

    g_tex_floor = load_texture("assets/image/floor64.png");
    g_tex_wall = load_texture("assets/image/wall64.png");
    g_tex_goal = load_texture("assets/image/backpack64.png");
    g_tex_exit = load_texture("assets/image/exit.PNG");

    g_tex_professor_1 = load_texture("assets/image/한명균교수님.png");
    g_tex_professor_2 = load_texture("assets/image/김진욱교수님테스트용.png");
    g_tex_professor_3 = load_texture("assets/image/한명균교수님.png");
    g_tex_professor_4 = load_texture("assets/image/한명균교수님.png");
    g_tex_professor_5 = load_texture("assets/image/한명균교수님.png");
    g_tex_professor_6 = load_texture("assets/image/한명균교수님.png");

    g_tex_obstacle = load_texture("assets/image/professor64.png"); // X (일반)
    g_tex_spinner = load_texture("assets/image/professor64.png");  // R (스피너)

    g_tex_item_shield = load_texture("assets/image/shield64.png");   // I 아이템 전용 텍스처
    g_tex_item_scooter = load_texture("assets/image/scooter64.png"); // E 아이템 전용 텍스처
    g_tex_item_supply = load_texture("assets/image/supply.png");     // A 아이템 투사체 보급

    g_tex_trap = load_texture("assets/image/trap.png");         // 트랩 (일반타일로 의문사 또는 실제 보이게 해서 못 지나가도록)
    g_tex_wall_break = load_texture("assets/image/wall64.png"); // 깨지는 벽

    g_tex_projectile = load_texture("assets/image/ball.png");      // 투사체 임시 렌더링
    g_tex_shield_on = load_texture("assets/image/shieldon64.png"); // 보호막 활성화 표현

    if (!g_tex_projectile || !g_tex_item_shield || !g_tex_item_scooter || !g_tex_shield_on)
        return -1;

    if (!g_tex_floor || !g_tex_wall || !g_tex_goal || !g_tex_professor_1 || !g_tex_exit)
    {
        return -1;
    }

    for (int variant = 0; variant < PLAYER_VARIANT_COUNT; variant++)
    {
        for (int facing = 0; facing < PLAYER_FACING_COUNT; facing++)
        {
            const PlayerTextureSet *set = &PLAYER_TEXTURE_PATHS[variant][facing];
            const char *paths[PLAYER_FRAME_COUNT] = {
                set->stand_a,
                set->stand_b,
                set->step_a,
                set->step_b};
            for (int frame = 0; frame < PLAYER_FRAME_COUNT; frame++)
            {
                g_player_textures[variant][facing][frame] = load_texture(paths[frame]);
                if (!g_player_textures[variant][facing][frame])
                {
                    return -1;
                }
            }
        }
    }

    return 0;
}

void shutdown_renderer(void)
{
    destroy_texture(&g_tex_floor);
    destroy_texture(&g_tex_wall);
    destroy_texture(&g_tex_goal);
    destroy_texture(&g_tex_exit);

    destroy_texture(&g_tex_professor_1); // 교수
    destroy_texture(&g_tex_professor_2);
    destroy_texture(&g_tex_professor_3);
    destroy_texture(&g_tex_professor_4);
    destroy_texture(&g_tex_professor_5);
    destroy_texture(&g_tex_professor_6);

    destroy_texture(&g_tex_obstacle);    // 일반 장애물
    destroy_texture(&g_tex_spinner);     // 도는 장애물
    destroy_texture(&g_tex_item_shield); // 아이템 렌더링 셧다운
    destroy_texture(&g_tex_item_scooter);
    destroy_texture(&g_tex_item_supply); // 보급

    destroy_texture(&g_tex_wall_break); // 깨지는 벽

    destroy_texture(&g_tex_trap); // 트랩

    destroy_texture(&g_tex_projectile); // 투사체 셧다운
    destroy_texture(&g_tex_shield_on);  // 플레이어 보호막 텍스처

    for (int variant = 0; variant < PLAYER_VARIANT_COUNT; variant++)
    {
        for (int facing = 0; facing < PLAYER_FACING_COUNT; facing++)
        {
            for (int frame = 0; frame < PLAYER_FRAME_COUNT; frame++)
            {
                destroy_texture(&g_player_textures[variant][facing][frame]);
            }
        }
    }

    if (g_renderer)
    {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
    if (g_window)
    {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }

    IMG_Quit();
    SDL_Quit();
}

static void ensure_window_matches_stage(const Stage *stage)
{
    if (!g_window || !g_renderer || !stage)
        return;

    int target_w = stage->width * TILE_SIZE;
    int target_h = stage->height * TILE_SIZE;

    if (target_w <= 0)
        target_w = TILE_SIZE * 10;
    if (target_h <= 0)
        target_h = TILE_SIZE * 10;

    if (target_w != g_window_w || target_h != g_window_h)
    {
        SDL_SetWindowSize(g_window, target_w, target_h);
        SDL_RenderSetLogicalSize(g_renderer, target_w, target_h);
        g_window_w = target_w;
        g_window_h = target_h;
    }
}

static SDL_Texture *texture_for_cell(char cell)
{
    if (cell == '#' || cell == '@')
        return g_tex_wall;

    if (cell == 'T')
        return g_tex_trap;
    return g_tex_floor;
}

static void draw_texture(SDL_Texture *texture, int x, int y)
{
    if (!texture)
        return;
    SDL_Rect dst = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
    SDL_RenderCopy(g_renderer, texture, NULL, &dst);
}

void render(const Stage *stage, const Player *player, double elapsed_time,
            int current_stage, int total_stages)
{
    if (!g_renderer || !stage || !player)
        return;

    ensure_window_matches_stage(stage);

    int stage_width = (stage->width > 0) ? stage->width : MAX_X;
    int stage_height = (stage->height > 0) ? stage->height : MAX_Y;
    unsigned char visibility[MAX_Y][MAX_X];
    compute_visibility(stage, player, visibility);

    SDL_SetRenderDrawColor(g_renderer, 15, 15, 15, 255);
    SDL_RenderClear(g_renderer);

    for (int y = 0; y < stage_height; y++)
    {
        for (int x = 0; x < stage_width; x++)
        {
            char cell = stage->map[y][x];
            SDL_Texture *base = texture_for_cell(cell);
            draw_texture(base, x, y);
        }
    }

    if (player->has_backpack)
    {
        draw_texture(g_tex_exit, stage->start_x, stage->start_y);
    }
    else
    {
        int offset_y = compute_vertical_bounce_offset(elapsed_time);
        draw_texture_with_pixel_offset(g_tex_goal, stage->goal_x, stage->goal_y, 0, offset_y);
    }
    SDL_Texture *current_prof_tex = g_tex_professor_1; // 기본값

    switch (current_stage)
    {
    case 1:
        current_prof_tex = g_tex_professor_1;
        break;
    case 2:
        current_prof_tex = g_tex_professor_2;
        break;
    case 3:
        current_prof_tex = g_tex_professor_3;
        break;
    case 4:
        current_prof_tex = g_tex_professor_4;
        break;
    case 5:
        current_prof_tex = g_tex_professor_5;
        break;
    case 6:
        current_prof_tex = g_tex_professor_6;
        break;
    default:
        current_prof_tex = g_tex_professor_1;
        break;
    }

    for (int i = 0; i < stage->num_obstacles; i++)
    {
        const Obstacle *o = &stage->obstacles[i];
        if (!o->active)
            continue;

        SDL_Texture *tex_to_draw = NULL;

        switch (o->kind)
        {
        case OBSTACLE_KIND_PROFESSOR:
            tex_to_draw = current_prof_tex;
            break;
        case OBSTACLE_KIND_SPINNER:
            tex_to_draw = g_tex_spinner;
            break;
        case OBSTACLE_KIND_BREAKABLE_WALL:
            tex_to_draw = g_tex_wall_break;
            break;

        case OBSTACLE_KIND_LINEAR:
        default:
            tex_to_draw = g_tex_obstacle;
            break;
        }

        if (tex_to_draw)
        {
            double obstacle_world_x = (double)o->world_x / SUBPIXELS_PER_TILE;
            double obstacle_world_y = (double)o->world_y / SUBPIXELS_PER_TILE;
            int tile_x = (int)floor(obstacle_world_x);
            int tile_y = (int)floor(obstacle_world_y);
            if (tile_x < 0 || tile_y < 0 || tile_x >= stage_width || tile_y >= stage_height)
                continue;
            if (!visibility[tile_y][tile_x])
                continue;
            draw_texture_at_world(tex_to_draw, obstacle_world_x, obstacle_world_y);
        }
    }

    for (int i = 0; i < stage->num_items; i++)
    {
        const Item *it = &stage->items[i];
        if (!it->active)
        {
            continue;
        }

        SDL_Texture *item_tex = NULL;
        switch (it->type)
        {
        case ITEM_TYPE_SHIELD:
            item_tex = g_tex_item_shield;
            break;
        case ITEM_TYPE_SCOOTER:
            item_tex = g_tex_item_scooter;
            break;
        case ITEM_TYPE_SUPPLY:
            item_tex = g_tex_item_supply;
            break;
        default:
            break;
        }

        if (!item_tex)
        {
            continue;
        }

        int offset_y = compute_vertical_bounce_offset(elapsed_time);
        int tile_x = it->world_x / SUBPIXELS_PER_TILE;
        int tile_y = it->world_y / SUBPIXELS_PER_TILE;
        if (tile_x < 0 || tile_y < 0 || tile_x >= stage_width || tile_y >= stage_height)
            continue;
        if (!visibility[tile_y][tile_x])
            continue;
        draw_texture_with_pixel_offset(item_tex, tile_x, tile_y, 0, offset_y);
    } // 아이템 렌더링

    for (int i = 0; i < stage->num_projectiles; i++)
    {
        const Projectile *p = &stage->projectiles[i];
        if (p->active)
        {
            double projectile_world_x = (double)p->world_x / SUBPIXELS_PER_TILE;
            double projectile_world_y = (double)p->world_y / SUBPIXELS_PER_TILE;
            int tile_x = (int)floor(projectile_world_x);
            int tile_y = (int)floor(projectile_world_y);
            if (tile_x < 0 || tile_y < 0 || tile_x >= stage_width || tile_y >= stage_height)
                continue;
            if (!visibility[tile_y][tile_x])
                continue;
            draw_texture_at_world(g_tex_projectile, projectile_world_x, projectile_world_y);
        }
    } // 투사체 렌더링

    PlayerFacing facing = player->facing;
    if (facing < 0 || facing >= PLAYER_FACING_COUNT)
    {
        facing = PLAYER_FACING_DOWN;
    }

    int variant = PLAYER_VARIANT_NORMAL;
    if (player->has_scooter)
    {
        variant = player->has_backpack ? PLAYER_VARIANT_SCOOTER_BACKPACK : PLAYER_VARIANT_SCOOTER;
    }
    else if (player->has_backpack)
    {
        variant = PLAYER_VARIANT_BACKPACK;
    }
    int frame = PLAYER_FRAME_STAND_A;
    switch (player->anim_phase)
    {
    case PLAYER_ANIM_PHASE_STEP_A:
        frame = PLAYER_FRAME_STEP_A;
        break;
    case PLAYER_ANIM_PHASE_STEP_B:
        frame = PLAYER_FRAME_STEP_B;
        break;
    case PLAYER_ANIM_PHASE_IDLE_B:
        frame = PLAYER_FRAME_STAND_B;
        break;
    case PLAYER_ANIM_PHASE_IDLE_A:
    default:
        frame = PLAYER_FRAME_STAND_A;
        break;
    }

    SDL_Texture *player_tex = g_player_textures[variant][facing][frame];
    if (!player_tex)
    {
        player_tex = g_player_textures[PLAYER_VARIANT_NORMAL][PLAYER_FACING_DOWN][PLAYER_FRAME_STAND_A];
    }
    double player_world_x = (double)player->world_x / SUBPIXELS_PER_TILE;
    double player_world_y = (double)player->world_y / SUBPIXELS_PER_TILE;
    draw_texture_at_world(player_tex, player_world_x, player_world_y);

    if (player->shield_count > 0 && g_tex_shield_on)
    {
        draw_texture_scaled(g_tex_shield_on, player_world_x, player_world_y, 1.2);
    }

    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_renderer, 40, 40, 40, 200);
    for (int y = 0; y < stage_height; ++y)
    {
        for (int x = 0; x < stage_width; ++x)
        {
            if (!visibility[y][x])
            {
                SDL_Rect fog = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                SDL_RenderFillRect(g_renderer, &fog);
            }
        }
    }
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);

    SDL_RenderPresent(g_renderer);

    char title[128];
    if (player->has_backpack)
    {

        snprintf(title, sizeof(title), "Stage %d/%d | Time %.2fs | Return to Exit | reamin ball: %d",
                 current_stage, total_stages, elapsed_time, stage->remaining_ammo);
    }
    else
    {

        snprintf(title, sizeof(title), "Stage %d/%d | Time %.2fs | reamin ball: %d",
                 current_stage, total_stages, elapsed_time, stage->remaining_ammo);
    }
    SDL_SetWindowTitle(g_window, title);
}
