// ============================================================================
// render.c
// ----------------------------------------------------------------------------
// SDL2 + SDL2_image를 이용해 타일 기반 맵을 이미지로 렌더링한다.
// Stage.map에 있는 문자 정보를 실제 PNG 텍스처와 매핑하여 화면에 표시한다.
// ============================================================================

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

#include "../include/game.h"
#include "render.h"

#define TILE_SIZE 32

static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture *g_tex_floor = NULL;
static SDL_Texture *g_tex_wall = NULL;
static SDL_Texture *g_tex_goal = NULL;
static SDL_Texture *g_tex_player = NULL;
static SDL_Texture *g_tex_professor = NULL;
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
    g_tex_player = load_texture("assets/image/player64.png");
    g_tex_professor = load_texture("assets/image/professor64.png");

    if (!g_tex_floor || !g_tex_wall || !g_tex_goal || !g_tex_player || !g_tex_professor)
    {
        return -1;
    }

    return 0;
}

void shutdown_renderer(void)
{
    destroy_texture(&g_tex_floor);
    destroy_texture(&g_tex_wall);
    destroy_texture(&g_tex_goal);
    destroy_texture(&g_tex_player);
    destroy_texture(&g_tex_professor);

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
            if (cell == 'G')
            {
                draw_texture(g_tex_goal, x, y);
            }
        }
    }

    for (int i = 0; i < stage->num_obstacles; i++)
    {
        Obstacle o = stage->obstacles[i];
        draw_texture(g_tex_professor, o.x, o.y);
    }

    draw_texture(g_tex_goal, stage->goal_x, stage->goal_y);
    draw_texture(g_tex_player, player->x, player->y);

    SDL_RenderPresent(g_renderer);

    char title[128];
    snprintf(title, sizeof(title), "Stage %d/%d | Time %.2fs", current_stage, total_stages, elapsed_time);
    SDL_SetWindowTitle(g_window, title);
}
