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
#include "render.h"

#define TILE_SIZE 32

typedef enum {
    PLAYER_VARIANT_NORMAL = 0,
    PLAYER_VARIANT_BACKPACK,
    PLAYER_VARIANT_COUNT
} PlayerVariant;

typedef enum {
    PLAYER_FRAME_STAND_A = 0,
    PLAYER_FRAME_STAND_B,
    PLAYER_FRAME_STEP_A,
    PLAYER_FRAME_STEP_B,
    PLAYER_FRAME_COUNT
} PlayerFrame;

typedef struct {
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
            "assets/image/player/foward_right.png"
        },
        [PLAYER_FACING_UP] = {
            "assets/image/player/back_stand_1.PNG",
            "assets/image/player/back_stand_2.png",
            "assets/image/player/back_left.PNG",
            "assets/image/player/back_right.PNG"
        },
        [PLAYER_FACING_LEFT] = {
            "assets/image/player/left_stand.PNG",
            "assets/image/player/left_stand.PNG",
            "assets/image/player/left_left.png",
            "assets/image/player/left_right.png"
        },
        [PLAYER_FACING_RIGHT] = {
            "assets/image/player/right_stand.png",
            "assets/image/player/right_stand.png",
            "assets/image/player/right_left.PNG",
            "assets/image/player/right_right.PNG"
        }
    },
    [PLAYER_VARIANT_BACKPACK] = {
        [PLAYER_FACING_DOWN] = {
            "assets/image/player_backpack/foward_stand.png",
            "assets/image/player_backpack/foward_stand.png",
            "assets/image/player_backpack/foward_left.png",
            "assets/image/player_backpack/foward_right.png"
        },
        [PLAYER_FACING_UP] = {
            "assets/image/player_backpack/back_stand_1.png",
            "assets/image/player_backpack/back_stand_2.png",
            "assets/image/player_backpack/back_left.png",
            "assets/image/player_backpack/back_right.png"
        },
        [PLAYER_FACING_LEFT] = {
            "assets/image/player_backpack/left_stand.PNG",
            "assets/image/player_backpack/left_stand.PNG",
            "assets/image/player_backpack/left_left.png",
            "assets/image/player_backpack/left_right.png"
        },
        [PLAYER_FACING_RIGHT] = {
            "assets/image/player_backpack/right_stand.png",
            "assets/image/player_backpack/right_stand.png",
            "assets/image/player_backpack/right_left.png",
            "assets/image/player_backpack/right_right.png"
        }
    }
};

static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture *g_tex_floor = NULL;
static SDL_Texture *g_tex_wall = NULL;
static SDL_Texture *g_tex_goal = NULL;
static SDL_Texture *g_tex_professor = NULL;
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
    g_tex_professor = load_texture("assets/image/professor64.png");
    g_tex_exit = load_texture("assets/image/exit.PNG");

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
                set->step_b
            };
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
    destroy_texture(&g_tex_professor);
    destroy_texture(&g_tex_exit);

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
            if (cell == 'G' && !player->has_backpack)
            {
                draw_texture(g_tex_goal, x, y);
            }
        }
    }

    if (player->has_backpack)
    {
        draw_texture(g_tex_exit, stage->start_x, stage->start_y);
    }

    for (int i = 0; i < stage->num_obstacles; i++)
    {
        Obstacle o = stage->obstacles[i];
        draw_texture(g_tex_professor, o.x, o.y);
    }

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
    draw_texture(player_tex, player->x, player->y);

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
