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

// 고정 해상도 + 고정 배율을 유지해 맵 크기와 무관하게 일정한 뷰포트를 제공한다.
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define VIEWPORT_TILES_X 26
#define VIEWPORT_TILES_Y 15
#define CAMERA_TILE_PADDING 1

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

typedef struct
{
    double pixel_x;
    double pixel_y;
    int tile_start_x;
    int tile_end_x;
    int tile_start_y;
    int tile_end_y;
    int tile_size;
    int viewport_pixel_w;
    int viewport_pixel_h;
    int viewport_offset_x;
    int viewport_offset_y;
} Camera;

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

static SDL_Texture *g_tex_student_w_left = NULL;  // w/l (여학생 좌향)
static SDL_Texture *g_tex_student_w_right = NULL; // W/L (여학생 우향)
static SDL_Texture *g_tex_student_m_left = NULL;  // m (남학생 좌향)
static SDL_Texture *g_tex_student_m_right = NULL; // M (남학생 우향)

static SDL_Texture *g_tex_pulpit = NULL;          // p (강단 장식)

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
static int g_tile_render_size = TILE_SIZE;

static void update_tile_render_metrics(void)
{
    int size_from_width = WINDOW_WIDTH / VIEWPORT_TILES_X;
    int size_from_height = WINDOW_HEIGHT / VIEWPORT_TILES_Y;
    int selected = (size_from_width < size_from_height) ? size_from_width : size_from_height;
    if (selected <= 0)
    {
        selected = TILE_SIZE;
    }
    g_tile_render_size = selected;
}

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

static void compute_camera(const Stage *stage, const Player *player, Camera *camera)
{
    if (!stage || !player || !camera)
    {
        return;
    }

    const int stage_width = (stage->width > 0) ? stage->width : MAX_X;
    const int stage_height = (stage->height > 0) ? stage->height : MAX_Y;

    const int tile_size = g_tile_render_size;
    const int viewport_w = tile_size * VIEWPORT_TILES_X;
    const int viewport_h = tile_size * VIEWPORT_TILES_Y;
    const int viewport_offset_x = (WINDOW_WIDTH - viewport_w) / 2;
    const int viewport_offset_y = (WINDOW_HEIGHT - viewport_h) / 2;

    camera->tile_size = tile_size;
    camera->viewport_pixel_w = viewport_w;
    camera->viewport_pixel_h = viewport_h;
    camera->viewport_offset_x = viewport_offset_x;
    camera->viewport_offset_y = viewport_offset_y;

    const double player_world_x = (double)player->world_x / SUBPIXELS_PER_TILE;
    const double player_world_y = (double)player->world_y / SUBPIXELS_PER_TILE;
    const double player_pixel_x = player_world_x * tile_size;
    const double player_pixel_y = player_world_y * tile_size;

    double desired_x = player_pixel_x + (tile_size / 2.0) - (viewport_w / 2.0);
    double desired_y = player_pixel_y + (tile_size / 2.0) - (viewport_h / 2.0);

    const double stage_pixel_w = stage_width * tile_size;
    const double stage_pixel_h = stage_height * tile_size;
    const double max_camera_x = (stage_pixel_w > viewport_w) ? (stage_pixel_w - viewport_w) : 0.0;
    const double max_camera_y = (stage_pixel_h > viewport_h) ? (stage_pixel_h - viewport_h) : 0.0;

    if (desired_x < 0.0)
        desired_x = 0.0;
    else if (desired_x > max_camera_x)
        desired_x = max_camera_x;

    if (desired_y < 0.0)
        desired_y = 0.0;
    else if (desired_y > max_camera_y)
        desired_y = max_camera_y;

    camera->pixel_x = desired_x;
    camera->pixel_y = desired_y;

    camera->tile_start_x = (int)floor(camera->pixel_x / tile_size) - CAMERA_TILE_PADDING;
    camera->tile_start_y = (int)floor(camera->pixel_y / tile_size) - CAMERA_TILE_PADDING;
    if (camera->tile_start_x < 0)
        camera->tile_start_x = 0;
    if (camera->tile_start_y < 0)
        camera->tile_start_y = 0;

    camera->tile_end_x = (int)ceil((camera->pixel_x + viewport_w) / tile_size) + CAMERA_TILE_PADDING;
    camera->tile_end_y = (int)ceil((camera->pixel_y + viewport_h) / tile_size) + CAMERA_TILE_PADDING;
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

static int is_rect_visible(const SDL_Rect *rect)
{
    return !(rect->x + rect->w <= 0 || rect->y + rect->h <= 0 || rect->x >= WINDOW_WIDTH || rect->y >= WINDOW_HEIGHT);
}

static void draw_texture_with_pixel_offset(SDL_Texture *texture, int x, int y, int offset_x, int offset_y, const Camera *camera)
{
    if (!texture)
    {
        return;
    }

    SDL_Rect dst = {camera->viewport_offset_x + x * camera->tile_size - (int)camera->pixel_x + offset_x,
                    camera->viewport_offset_y + y * camera->tile_size - (int)camera->pixel_y + offset_y,
                    camera->tile_size,
                    camera->tile_size};
    if (!is_rect_visible(&dst))
    {
        return;
    }
    SDL_RenderCopy(g_renderer, texture, NULL, &dst);
}

static void draw_texture_at_world(SDL_Texture *texture, double world_x, double world_y, const Camera *camera)
{
    if (!texture)
        return;

    SDL_Rect dst = {camera->viewport_offset_x + (int)lround(world_x * camera->tile_size - camera->pixel_x),
                    camera->viewport_offset_y + (int)lround(world_y * camera->tile_size - camera->pixel_y),
                    camera->tile_size,
                    camera->tile_size};
    if (!is_rect_visible(&dst))
    {
        return;
    }
    SDL_RenderCopy(g_renderer, texture, NULL, &dst);
}

static void draw_texture_scaled(SDL_Texture *texture, double world_x, double world_y, double scale, const Camera *camera)
{
    if (!texture)
    {
        return;
    }

    const int base_size = camera->tile_size;
    const int width = (int)lround(base_size * scale);
    const int height = (int)lround(base_size * scale);

    const int offset_x = (base_size - width) / 2;
    const int offset_y = (base_size - height) / 2;

    SDL_Rect dst = {camera->viewport_offset_x + (int)lround(world_x * base_size - camera->pixel_x) + offset_x,
                    camera->viewport_offset_y + (int)lround(world_y * base_size - camera->pixel_y) + offset_y,
                    width,
                    height};
    if (!is_rect_visible(&dst))
    {
        return;
    }
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

    const int initial_w = WINDOW_WIDTH;
    const int initial_h = WINDOW_HEIGHT;
    g_window = SDL_CreateWindow("Professor Dodge",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                initial_w,
                                initial_h,
                                SDL_WINDOW_SHOWN);
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

    SDL_RenderSetLogicalSize(g_renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    update_tile_render_metrics();

    g_tex_floor = load_texture("assets/image/floor64.png");
    g_tex_wall = load_texture("assets/image/wall64.png");
    g_tex_goal = load_texture("assets/image/backpack64.png");
    g_tex_exit = load_texture("assets/image/exit.PNG");

    g_tex_professor_1 = load_texture("assets/image/김명석교수님.png");
    // g_tex_professor_2 = load_texture("assets/image/김진욱교수님테스트용.png");
    g_tex_professor_2 = load_texture("assets/image/이종택교수님.png");
    g_tex_professor_3 = load_texture("assets/image/한명균교수님.png");
    g_tex_professor_4 = load_texture("assets/image/한명균교수님.png");
    g_tex_professor_5 = load_texture("assets/image/한명균교수님.png");
    g_tex_professor_6 = load_texture("assets/image/한명균교수님.png");

    g_tex_obstacle = load_texture("assets/image/professor64.png"); // X (일반)
    g_tex_spinner = load_texture("assets/image/professor64.png");  // R (스피너)

    g_tex_item_shield = load_texture("assets/image/shield64.png");   // I 아이템 전용 텍스처
    g_tex_item_scooter = load_texture("assets/image/scooter64.png"); // E 아이템 전용 텍스처
    g_tex_item_supply = load_texture("assets/image/supply.png");     // A 아이템 투사체 보급

    g_tex_student_w_left = load_texture("assets/image/w_left.png");
    g_tex_student_w_right = load_texture("assets/image/w_right.PNG");
    g_tex_student_m_left = load_texture("assets/image/m_left.png");
    g_tex_student_m_right = load_texture("assets/image/m_right.png");

    g_tex_pulpit = load_texture("assets/image/pulpit64.png");

    g_tex_trap = load_texture("assets/image/trap.png");         // 트랩 (일반타일로 의문사 또는 실제 보이게 해서 못 지나가도록)
    g_tex_wall_break = load_texture("assets/image/wall64.png"); // 깨지는 벽

    g_tex_projectile = load_texture("assets/image/ball.png");      // 투사체 임시 렌더링
    g_tex_shield_on = load_texture("assets/image/shieldon64.png"); // 보호막 활성화 표현

    if (!g_tex_projectile || !g_tex_item_shield || !g_tex_item_scooter || !g_tex_shield_on)
        return -1;

    if (!g_tex_floor || !g_tex_wall || !g_tex_goal || !g_tex_professor_1 || !g_tex_exit ||
        !g_tex_student_w_left || !g_tex_student_w_right || !g_tex_student_m_left ||
        !g_tex_student_m_right || !g_tex_pulpit)
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
    destroy_texture(&g_tex_student_w_left);
    destroy_texture(&g_tex_student_w_right);
    destroy_texture(&g_tex_student_m_left);
    destroy_texture(&g_tex_student_m_right);

    destroy_texture(&g_tex_pulpit);

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
    (void)stage;
    if (!g_window || !g_renderer)
        return;

    const int target_w = WINDOW_WIDTH;
    const int target_h = WINDOW_HEIGHT;

    if (target_w != g_window_w || target_h != g_window_h)
    {
        SDL_SetWindowSize(g_window, target_w, target_h);
        SDL_RenderSetLogicalSize(g_renderer, target_w, target_h);
        g_window_w = target_w;
        g_window_h = target_h;
        update_tile_render_metrics();
    }
}

static SDL_Texture *texture_for_cell(char cell)
{
    switch (cell)
    {
    case '#':
    case '@':
        return g_tex_wall;
    case 'T':
        return g_tex_trap;
    case 'p':
        return g_tex_pulpit;
    default:
        return g_tex_floor;
    }
}

static SDL_Texture *texture_for_student(char cell)
{
    switch (cell)
    {
    case 'w':
    case 'l':
        return g_tex_student_w_left;
    case 'W':
    case 'L':
        return g_tex_student_w_right;
    case 'm':
        return g_tex_student_m_left;
    case 'M':
        return g_tex_student_m_right;
    default:
        return NULL;
    }
}

static void draw_texture(SDL_Texture *texture, int x, int y, const Camera *camera);

static void render_professor_clones(const Stage *stage,
                                    SDL_Texture *texture,
                                    const Camera *camera,
                                    const unsigned char visibility[MAX_Y][MAX_X])
{
    if (!stage || !texture || !camera)
    {
        return;
    }

    if (stage->num_professor_clones <= 0)
    {
        return;
    }

    int stage_width = (stage->width > 0) ? stage->width : MAX_X;
    int stage_height = (stage->height > 0) ? stage->height : MAX_Y;

    for (int i = 0; i < MAX_PROFESSOR_CLONES; ++i)
    {
        const ProfessorClone *clone = &stage->professor_clones[i];
        if (!clone->active)
        {
            continue;
        }

        int tx = clone->tile_x;
        int ty = clone->tile_y;
        if (tx < 0 || ty < 0 || tx >= stage_width || ty >= stage_height)
        {
            continue;
        }

        if (!visibility[ty][tx])
        {
            continue;
        }

        draw_texture(texture, tx, ty, camera);
    }
}

static void draw_texture(SDL_Texture *texture, int x, int y, const Camera *camera)
{
    if (!texture)
        return;
    SDL_Rect dst = {camera->viewport_offset_x + x * camera->tile_size - (int)camera->pixel_x,
                    camera->viewport_offset_y + y * camera->tile_size - (int)camera->pixel_y,
                    camera->tile_size,
                    camera->tile_size};
    if (!is_rect_visible(&dst))
    {
        return;
    }
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

    Camera camera = {0};
    compute_camera(stage, player, &camera);

    int draw_start_x = camera.tile_start_x;
    if (draw_start_x < 0)
        draw_start_x = 0;
    int draw_end_x = camera.tile_end_x;
    if (draw_end_x > stage_width)
        draw_end_x = stage_width;

    int draw_start_y = camera.tile_start_y;
    if (draw_start_y < 0)
        draw_start_y = 0;
    int draw_end_y = camera.tile_end_y;
    if (draw_end_y > stage_height)
        draw_end_y = stage_height;

    SDL_SetRenderDrawColor(g_renderer, 15, 15, 15, 255);
    SDL_RenderClear(g_renderer);

    for (int y = draw_start_y; y < draw_end_y; y++)
    {
        for (int x = draw_start_x; x < draw_end_x; x++)
        {
            char render_cell = stage->render_map[y][x];
            if (render_cell == '\0')
            {
                render_cell = ' ';
            }

            if (render_cell == ' ')
            {
                char logical_cell = stage->map[y][x];
                if (texture_for_student(logical_cell))
                {
                    render_cell = ' ';
                }
                else
                {
                    render_cell = logical_cell;
                }
            }

            SDL_Texture *base = texture_for_cell(render_cell);
            draw_texture(base ? base : g_tex_floor, x, y, &camera);
        }
    }

    for (int y = draw_start_y; y < draw_end_y; ++y)
    {
        for (int x = draw_start_x; x < draw_end_x; ++x)
        {
            char logical_cell = stage->map[y][x];
            SDL_Texture *student_tex = texture_for_student(logical_cell);
            if (!student_tex)
            {
                continue;
            }

            if (!visibility[y][x])
            {
                continue;
            }

            draw_texture(student_tex, x, y, &camera);
        }
    }

    if (!player->has_backpack)
    {
        int gx = stage->goal_x;
        int gy = stage->goal_y;

        if (gx >= 0 && gy >= 0)
        {
            int offset = (int)round(sin(elapsed_time * 6.0) * 2);
            draw_texture_with_pixel_offset(g_tex_goal, gx, gy, 0, offset, &camera);
        }
    }

    if (player->has_backpack)
    {
        int fx = stage->exit_x;
        int fy = stage->exit_y;

        if (fx >= 0 && fy >= 0)
        {
            draw_texture(g_tex_exit, fx, fy, &camera);
        }
    }

    SDL_Texture *current_prof_tex = g_tex_professor_1; // 기본값
    int stage_identifier = stage->id > 0 ? stage->id : current_stage;

    switch (stage_identifier)
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
            draw_texture_at_world(tex_to_draw, obstacle_world_x, obstacle_world_y, &camera);
        }
    }

    render_professor_clones(stage, current_prof_tex, &camera, visibility);

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
        draw_texture_with_pixel_offset(item_tex, tile_x, tile_y, 0, offset_y, &camera);
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
            draw_texture_at_world(g_tex_projectile, projectile_world_x, projectile_world_y, &camera);
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
    draw_texture_at_world(player_tex, player_world_x, player_world_y, &camera);

    if (player->shield_count > 0 && g_tex_shield_on)
    {
        draw_texture_scaled(g_tex_shield_on, player_world_x, player_world_y, 1.2, &camera);
    }

    if (player->is_confused)
    {
        // 렌더러의 색상을 반투명한 검은색으로 설정 (50% 투명도)
        SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(g_renderer, 20, 20, 20, 200); // R, G, B, Alpha (210은 불투명정도, 해당 수치로 투명도 조절 가능)

        // 화면 전체를 덮는 사각형 정의 (창 크기 사용)
        SDL_Rect screen_rect = {0, 0, g_window_w, g_window_h};

        // 화면 전체에 반투명한 검은색 사각형을 채웁니다.
        SDL_RenderFillRect(g_renderer, &screen_rect);

        // 블렌드 모드와 색상을 복구합니다.
        SDL_SetRenderDrawColor(g_renderer, 15, 15, 15, 255); // 배경색과 동일하게 복구
        SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
    }

    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_renderer, 40, 40, 40, 200);
    for (int y = draw_start_y; y < draw_end_y; ++y)
    {
        for (int x = draw_start_x; x < draw_end_x; ++x)
        {
            if (!visibility[y][x])
            {
                SDL_Rect fog = {camera.viewport_offset_x + x * camera.tile_size - (int)camera.pixel_x,
                                camera.viewport_offset_y + y * camera.tile_size - (int)camera.pixel_y,
                                camera.tile_size,
                                camera.tile_size};
                if (!is_rect_visible(&fog))
                {
                    continue;
                }
                SDL_RenderFillRect(g_renderer, &fog);
            }
        }
    }
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);

    // 패턴 확인용 주석처리
    SDL_RenderPresent(g_renderer);

    char title[128];
    if (player->has_backpack)
    {

        snprintf(title, sizeof(title), "Stage %d/%d | Time %.2fs | Return to Exit | reamin ball: %d",
                 current_stage, total_stages, elapsed_time, stage->remaining_ammo);
    }
    else
    {

        snprintf(title, sizeof(title), "Stage %d/%d | Time %.2fs | remain ball: %d",
                 current_stage, total_stages, elapsed_time, stage->remaining_ammo);
    }
    SDL_SetWindowTitle(g_window, title);
}
