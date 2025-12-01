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

#include "../include/game.h"
#include "../include/render.h"

#define TILE_SIZE 32

typedef enum
{
    PLAYER_VARIANT_NORMAL = 0,
    PLAYER_VARIANT_BACKPACK,
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
    [PLAYER_VARIANT_BACKPACK] = {[PLAYER_FACING_DOWN] = {"assets/image/player_backpack/foward_stand.png", "assets/image/player_backpack/foward_stand.png", "assets/image/player_backpack/foward_left.png", "assets/image/player_backpack/foward_right.png"}, [PLAYER_FACING_UP] = {"assets/image/player_backpack/back_stand_1.png", "assets/image/player_backpack/back_stand_2.png", "assets/image/player_backpack/back_left.png", "assets/image/player_backpack/back_right.png"}, [PLAYER_FACING_LEFT] = {"assets/image/player_backpack/left_stand.PNG", "assets/image/player_backpack/left_stand.PNG", "assets/image/player_backpack/left_left.png", "assets/image/player_backpack/left_right.png"}, [PLAYER_FACING_RIGHT] = {"assets/image/player_backpack/right_stand.png", "assets/image/player_backpack/right_stand.png", "assets/image/player_backpack/right_left.png", "assets/image/player_backpack/right_right.png"}}};

static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture *g_tex_floor = NULL;
static SDL_Texture *g_tex_wall = NULL;
static SDL_Texture *g_tex_goal = NULL;

static SDL_Texture *g_tex_professor = NULL;   // P 교수님
static SDL_Texture *g_tex_spinner = NULL;     // R (스피너)
static SDL_Texture *g_tex_obstacle = NULL;    // X (일반 장애물)


static SDL_Texture *g_tex_item_shield = NULL; // 필드에 놓인 쉴드 아이템(I)
static SDL_Texture *g_tex_projectile = NULL;  // 투사체
static SDL_Texture *g_tex_shield_on = NULL;   // 플레이어 보호막 활성화

static SDL_Texture *g_tex_exit = NULL;

static SDL_Texture *g_player_textures[PLAYER_VARIANT_COUNT][PLAYER_FACING_COUNT][PLAYER_FRAME_COUNT] = {{{NULL}}};
static int g_window_w = 0;
static int g_window_h = 0;

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

    g_tex_professor = load_texture("assets/image/한명균교수님.png");
    g_tex_obstacle = load_texture("assets/image/professor64.png");    // X (일반)
    g_tex_spinner = load_texture("assets/image/professor64.png");     // R (스피너)


    g_tex_item_shield = load_texture("assets/image/shield64.png");   // I 아이템 전용 텍스처
    g_tex_projectile = load_texture("assets/image/professor64.png");  // 투사체 임시 렌더링
    g_tex_shield_on = load_texture("assets/image/shieldon64.png");    // 보호막 활성화 표현

    if (!g_tex_projectile || !g_tex_item_shield || !g_tex_shield_on)
        return -1;

    if (!g_tex_floor || !g_tex_wall || !g_tex_goal || !g_tex_professor || !g_tex_exit)
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

    destroy_texture(&g_tex_professor);   // 교수
    destroy_texture(&g_tex_obstacle);    // 일반 장애물
    destroy_texture(&g_tex_spinner);     // 도는 장애물
    destroy_texture(&g_tex_item_shield); // 아이템 렌더링 셧다운
    destroy_texture(&g_tex_projectile);  // 투사체 셧다운
    destroy_texture(&g_tex_shield_on);   // 플레이어 보호막 텍스처
    

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

    SDL_SetRenderDrawColor(g_renderer, 15, 15, 15, 255);
    SDL_RenderClear(g_renderer);

    for (int y = 0; y < stage->height; y++)
    {
        for (int x = 0; x < stage->width; x++)
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

    for (int i = 0; i < stage->num_obstacles; i++)
    {
        const Obstacle *o = &stage->obstacles[i];
        if (!o->active)
            continue;

        SDL_Texture *tex_to_draw = NULL;

        switch (o->kind)
        {
        case OBSTACLE_KIND_PROFESSOR:
            tex_to_draw = g_tex_professor;
            break;
        case OBSTACLE_KIND_SPINNER:
            tex_to_draw = g_tex_spinner;
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
            draw_texture_at_world(tex_to_draw, obstacle_world_x, obstacle_world_y);
        }
    }

    for (int i = 0; i < stage->num_items; i++)
    {
        const Item *it = &stage->items[i];
        if (it->active)
        {
            int offset_y = compute_vertical_bounce_offset(elapsed_time);
            int tile_x = it->world_x / SUBPIXELS_PER_TILE;
            int tile_y = it->world_y / SUBPIXELS_PER_TILE;
            draw_texture_with_pixel_offset(g_tex_item_shield, tile_x, tile_y, 0, offset_y);
        }
    } // 아이템 렌더링

    for (int i = 0; i < stage->num_projectiles; i++)
    {
        const Projectile *p = &stage->projectiles[i];
        if (p->active)
        {
            double projectile_world_x = (double)p->world_x / SUBPIXELS_PER_TILE;
            double projectile_world_y = (double)p->world_y / SUBPIXELS_PER_TILE;
            draw_texture_at_world(g_tex_projectile, projectile_world_x, projectile_world_y);
        }
    } // 투사체 렌더링

    PlayerFacing facing = player->facing;
    if (facing < 0 || facing >= PLAYER_FACING_COUNT)
    {
        facing = PLAYER_FACING_DOWN;
    }

    int variant = player->has_backpack ? PLAYER_VARIANT_BACKPACK : PLAYER_VARIANT_NORMAL;
    int frame = PLAYER_FRAME_STAND_A;
    if (player->is_moving)
    {
        frame = player->anim_step ? PLAYER_FRAME_STEP_B : PLAYER_FRAME_STEP_A;
    }
    else if (facing == PLAYER_FACING_UP)
    {
        int toggle = ((int)floor(elapsed_time)) % 2;
        frame = toggle ? PLAYER_FRAME_STAND_B : PLAYER_FRAME_STAND_A;
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

    SDL_RenderPresent(g_renderer);

    char title[128];
    if (player->has_backpack)
    {
        snprintf(title, sizeof(title), "Stage %d/%d | Time %.2fs | Return to Exit",
                 current_stage, total_stages, elapsed_time);
    }
    else
    {
        snprintf(title, sizeof(title), "Stage %d/%d | Time %.2fs", current_stage, total_stages, elapsed_time);
    }
    SDL_SetWindowTitle(g_window, title);
}
