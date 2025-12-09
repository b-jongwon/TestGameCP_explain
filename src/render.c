// ============================================================================
// render.c
// ----------------------------------------------------------------------------
// SDL2 + SDL2_image를 이용해 타일 기반 맵을 이미지로 렌더링한다.
// Stage.map에 있는 문자 정보를 실제 PNG 텍스처와 매핑하여 화면에 표시한다.
// ============================================================================

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/game.h"
#include "../include/render.h"

#define TILE_SIZE 32
#define ARRAY_LEN(arr) ((int)(sizeof(arr) / sizeof((arr)[0])))

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

static const char *stage_label_for_id(int stage_id)
{
    switch (stage_id)
    {
    case 1:
        return "B1";
    case 2:
        return "1F";
    case 3:
        return "2F";
    case 4:
        return "3F";
    case 5:
        return "4F";
    case 6:
        return "5F";
    default:
        return "UNKNOWN";
    }
}

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
static SDL_Texture *g_tex_professor_bullet = NULL; // 교수 탄환

static SDL_Texture *g_tex_trap = NULL;       // 트랩
static SDL_Texture *g_tex_wall_break = NULL; // 깨지는 벽

static SDL_Texture *g_tex_exit = NULL;
static SDL_Texture *g_tex_menu_background = NULL;
static SDL_Texture *g_tex_game_over_image = NULL;

static SDL_Texture *g_player_textures[PLAYER_VARIANT_COUNT][PLAYER_FACING_COUNT][PLAYER_FRAME_COUNT] = {{{NULL}}};
static int g_window_w = 0;
static int g_window_h = 0;
static int g_tile_render_size = TILE_SIZE;

#define HUD_FONT_WIDTH 5
#define HUD_FONT_HEIGHT 7
#define HUD_FONT_SCALE 2
#define HUD_CHAR_SPACING 1
#define HUD_LINE_SPACING (HUD_FONT_HEIGHT * HUD_FONT_SCALE + 8)
#define HUD_MARGIN 12

typedef struct
{
    char ch;
    unsigned char rows[HUD_FONT_HEIGHT];
} HudFontGlyph;

// SDL_ttf를 추가 도입하지 않고도 간단한 HUD 문구를 표시하기 위해 5x7 비트맵 폰트를 직접 정의한다.
static const HudFontGlyph kHudFontGlyphs[] = {
    {'0', {0x1E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x1E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
    {'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
    {':', {0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x12, 0x11, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
};

#define PROFESSOR_TEXTURE_COUNT 6
#define PROFESSOR_LABEL_TEXT_MAX 32
#define PROFESSOR_LABEL_PADDING_X 8
#define PROFESSOR_LABEL_PADDING_Y 4
#define PROFESSOR_LABEL_ABOVE_OFFSET 8
#define PROFESSOR_LABEL_FONT_SIZE 22

static const char *kProfessorLabelFontCandidates[] = {
    "assets/font/ProfessorLabel.ttf",
    "assets/font/ProfessorLabel.otf",
    "assets/font/D2Coding-Ver1.3.2-20180524-all.ttc",
    "/usr/share/fonts/truetype/noto/NotoSansKR-Regular.otf",
    "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/truetype/nanum/NanumGothic.ttf",
    "/usr/share/fonts/truetype/nanum/NanumGothicCoding.ttf",
    "/usr/share/fonts/truetype/ubuntu/UbuntuSans[wght].ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
};

static char g_professor_label_texts[PROFESSOR_TEXTURE_COUNT][PROFESSOR_LABEL_TEXT_MAX];
static SDL_Texture *g_professor_label_textures[PROFESSOR_TEXTURE_COUNT] = {NULL};
static int g_professor_label_width[PROFESSOR_TEXTURE_COUNT] = {0};
static int g_professor_label_height[PROFESSOR_TEXTURE_COUNT] = {0};
static TTF_Font *g_professor_label_font = NULL;
static char g_professor_label_font_path[512] = {0};
static TTF_Font *g_ui_font_large = NULL;
static TTF_Font *g_ui_font_small = NULL;
static int g_ttf_initialized = 0;

typedef struct
{
    SDL_Texture *texture;
    int width;
    int height;
} StaticTexture;

typedef struct
{
    SDL_Texture *normal;
    SDL_Texture *highlight;
    int width;
    int height;
    const char *label;
} MenuOptionTexture;

typedef struct
{
    SDL_Texture *texture;
    int width;
    int height;
    SDL_Color color;
    char last_text[128];
    int valid;
} CachedText;

static MenuOptionTexture g_title_menu_options[3];
static StaticTexture g_title_hint_texture = {NULL, 0, 0};
static CachedText g_title_logo_cache = {NULL, 0, 0, {0, 0, 0, 0}, "", 0};
static CachedText g_records_best_cache = {NULL, 0, 0, {0, 0, 0, 0}, "", 0};
static StaticTexture g_records_hint_texture = {NULL, 0, 0};
static CachedText g_game_over_title_cache = {NULL, 0, 0, {0, 0, 0, 0}, "", 0};
static StaticTexture g_game_over_hint_texture = {NULL, 0, 0};

static const HudFontGlyph *find_hud_glyph(char c)
{
    const size_t count = sizeof(kHudFontGlyphs) / sizeof(kHudFontGlyphs[0]);
    for (size_t i = 0; i < count; ++i)
    {
        if (kHudFontGlyphs[i].ch == c)
        {
            return &kHudFontGlyphs[i];
        }
    }
    return &kHudFontGlyphs[count - 1]; // 공백 기본값
}

static void draw_hud_glyph(char c, int x, int y)
{
    const HudFontGlyph *glyph = find_hud_glyph(c);
    for (int row = 0; row < HUD_FONT_HEIGHT; ++row)
    {
        unsigned char mask = glyph->rows[row];
        for (int col = 0; col < HUD_FONT_WIDTH; ++col)
        {
            int bit = HUD_FONT_WIDTH - 1 - col;
            if (mask & (1 << bit))
            {
                SDL_Rect pixel = {
                    x + col * HUD_FONT_SCALE,
                    y + row * HUD_FONT_SCALE,
                    HUD_FONT_SCALE,
                    HUD_FONT_SCALE};
                SDL_RenderFillRect(g_renderer, &pixel);
            }
        }
    }
}

static void draw_hud_text(const char *text, int x, int y)
{
    int pen_x = x;
    for (const char *p = text; *p; ++p)
    {
        if (*p == '\n')
        {
            y += HUD_LINE_SPACING;
            pen_x = x;
            continue;
        }
        draw_hud_glyph(*p, pen_x, y);
        pen_x += HUD_FONT_WIDTH * HUD_FONT_SCALE + HUD_CHAR_SPACING;
    }
}

static int is_file_readable(const char *path)
{
    return (path && access(path, R_OK) == 0);
}

static void remember_professor_font_path(const char *path)
{
    if (!path)
    {
        g_professor_label_font_path[0] = '\0';
        return;
    }
    strncpy(g_professor_label_font_path, path, sizeof(g_professor_label_font_path) - 1);
    g_professor_label_font_path[sizeof(g_professor_label_font_path) - 1] = '\0';
}

static SDL_Texture *create_text_texture(TTF_Font *font, const char *text, SDL_Color color, int *out_w, int *out_h)
{
    if (!font || !text)
    {
        return NULL;
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface)
    {
        fprintf(stderr, "TTF_RenderUTF8_Blended failed for '%s': %s\n", text, TTF_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(g_renderer, surface);
    if (!texture)
    {
        fprintf(stderr, "SDL_CreateTextureFromSurface failed for '%s': %s\n", text, SDL_GetError());
        SDL_FreeSurface(surface);
        return NULL;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    if (out_w)
    {
        *out_w = surface->w;
    }
    if (out_h)
    {
        *out_h = surface->h;
    }
    SDL_FreeSurface(surface);
    return texture;
}

static void destroy_static_texture(StaticTexture *target)
{
    if (!target)
    {
        return;
    }
    if (target->texture)
    {
        SDL_DestroyTexture(target->texture);
    }
    target->texture = NULL;
    target->width = 0;
    target->height = 0;
}

static void destroy_menu_option(MenuOptionTexture *option)
{
    if (!option)
    {
        return;
    }
    if (option->normal)
    {
        SDL_DestroyTexture(option->normal);
    }
    if (option->highlight)
    {
        SDL_DestroyTexture(option->highlight);
    }
    option->normal = NULL;
    option->highlight = NULL;
    option->width = 0;
    option->height = 0;
}

static void destroy_cached_text(CachedText *cache)
{
    if (!cache)
    {
        return;
    }
    if (cache->texture)
    {
        SDL_DestroyTexture(cache->texture);
    }
    cache->texture = NULL;
    cache->width = 0;
    cache->height = 0;
    cache->color = (SDL_Color){0, 0, 0, 0};
    cache->last_text[0] = '\0';
    cache->valid = 0;
}

static int cached_text_matches(const CachedText *cache, const char *text, SDL_Color color)
{
    if (!cache || !cache->valid)
    {
        return 0;
    }
    if (!text)
    {
        return 0;
    }
    if (cache->color.r != color.r || cache->color.g != color.g ||
        cache->color.b != color.b || cache->color.a != color.a)
    {
        return 0;
    }
    return strncmp(cache->last_text, text, sizeof(cache->last_text)) == 0;
}

static void update_cached_text(CachedText *cache, TTF_Font *font, const char *text, SDL_Color color)
{
    if (!cache || !font || !text)
    {
        return;
    }

    if (cached_text_matches(cache, text, color))
    {
        return;
    }

    SDL_Texture *texture = create_text_texture(font, text, color, &cache->width, &cache->height);
    if (!texture)
    {
        return;
    }

    if (cache->texture)
    {
        SDL_DestroyTexture(cache->texture);
    }
    cache->texture = texture;
    cache->color = color;
    strncpy(cache->last_text, text, sizeof(cache->last_text) - 1);
    cache->last_text[sizeof(cache->last_text) - 1] = '\0';
    cache->valid = 1;
}

static int build_title_menu_textures(void)
{
    if (!g_ui_font_large)
    {
        return -1;
    }

    const char *labels[] = {"시작하기", "기록보기", "종료하기"};
    SDL_Color normal_color = {200, 200, 200, 255};
    SDL_Color highlight_color = {255, 255, 255, 255};

    for (int i = 0; i < ARRAY_LEN(g_title_menu_options); ++i)
    {
        destroy_menu_option(&g_title_menu_options[i]);
        g_title_menu_options[i].label = labels[i];

        int normal_w = 0;
        int normal_h = 0;
        int highlight_w = 0;
        int highlight_h = 0;

        g_title_menu_options[i].normal = create_text_texture(g_ui_font_large,
                                                             labels[i],
                                                             normal_color,
                                                             &normal_w,
                                                             &normal_h);
        g_title_menu_options[i].highlight = create_text_texture(g_ui_font_large,
                                                                labels[i],
                                                                highlight_color,
                                                                &highlight_w,
                                                                &highlight_h);
        if (!g_title_menu_options[i].normal || !g_title_menu_options[i].highlight)
        {
            return -1;
        }

        g_title_menu_options[i].width = (normal_w > highlight_w) ? normal_w : highlight_w;
        g_title_menu_options[i].height = (normal_h > highlight_h) ? normal_h : highlight_h;
    }
    return 0;
}

static int build_static_textures(void)
{
    if (!g_ui_font_small)
    {
        return -1;
    }

    destroy_static_texture(&g_title_hint_texture);
    destroy_static_texture(&g_records_hint_texture);
    destroy_static_texture(&g_game_over_hint_texture);

    SDL_Color hint_color = {180, 180, 180, 255};
    g_title_hint_texture.texture = create_text_texture(g_ui_font_small,
                                                       "W/S 또는 ↑/↓ 로 이동, Enter로 선택",
                                                       hint_color,
                                                       &g_title_hint_texture.width,
                                                       &g_title_hint_texture.height);
    g_records_hint_texture.texture = create_text_texture(g_ui_font_small,
                                                         "아무 키나 누르면 돌아갑니다",
                                                         hint_color,
                                                         &g_records_hint_texture.width,
                                                         &g_records_hint_texture.height);
    g_game_over_hint_texture.texture = create_text_texture(g_ui_font_small,
                                                           "아무 키나 누르면 시작화면으로",
                                                           hint_color,
                                                           &g_game_over_hint_texture.width,
                                                           &g_game_over_hint_texture.height);

    if (!g_title_hint_texture.texture || !g_records_hint_texture.texture || !g_game_over_hint_texture.texture)
    {
        return -1;
    }
    return 0;
}

static void destroy_title_screen_artifacts(void)
{
    for (int i = 0; i < ARRAY_LEN(g_title_menu_options); ++i)
    {
        destroy_menu_option(&g_title_menu_options[i]);
    }
    destroy_static_texture(&g_title_hint_texture);
    destroy_cached_text(&g_title_logo_cache);
}

static void destroy_overlay_textures(void)
{
    destroy_static_texture(&g_records_hint_texture);
    destroy_static_texture(&g_game_over_hint_texture);
    destroy_cached_text(&g_records_best_cache);
    destroy_cached_text(&g_game_over_title_cache);
}

static TTF_Font *open_professor_label_font(void)
{
    const char *override_path = getenv("PROFESSOR_LABEL_FONT");
    if (is_file_readable(override_path))
    {
        TTF_Font *font = TTF_OpenFont(override_path, PROFESSOR_LABEL_FONT_SIZE);
        if (font)
        {
            printf("Professor label font: %s\n", override_path);
            remember_professor_font_path(override_path);
            return font;
        }
        fprintf(stderr, "TTF_OpenFont failed for %s: %s\n", override_path, TTF_GetError());
    }

    const size_t candidate_count = sizeof(kProfessorLabelFontCandidates) / sizeof(kProfessorLabelFontCandidates[0]);
    for (size_t i = 0; i < candidate_count; ++i)
    {
        const char *candidate = kProfessorLabelFontCandidates[i];
        if (!is_file_readable(candidate))
        {
            continue;
        }
        TTF_Font *font = TTF_OpenFont(candidate, PROFESSOR_LABEL_FONT_SIZE);
        if (font)
        {
            printf("Professor label font: %s\n", candidate);
            remember_professor_font_path(candidate);
            return font;
        }
        fprintf(stderr, "TTF_OpenFont failed for %s: %s\n", candidate, TTF_GetError());
    }

    fprintf(stderr, "Unable to find readable font for professor labels. Set PROFESSOR_LABEL_FONT to a Hangul-capable TTF.\n");
    return NULL;
}

static TTF_Font *open_additional_ui_font(int point_size)
{
    if (g_professor_label_font_path[0] == '\0')
    {
        fprintf(stderr, "Professor font path unresolved; cannot open UI font size %d\n", point_size);
        return NULL;
    }

    TTF_Font *font = TTF_OpenFont(g_professor_label_font_path, point_size);
    if (!font)
    {
        fprintf(stderr, "TTF_OpenFont failed for UI font (%s, %dpt): %s\n",
                g_professor_label_font_path, point_size, TTF_GetError());
    }
    return font;
}

static TTF_Font *get_large_ui_font(void)
{
    return g_ui_font_large ? g_ui_font_large : g_professor_label_font;
}

static TTF_Font *get_small_ui_font(void)
{
    return g_ui_font_small ? g_ui_font_small : g_professor_label_font;
}

static void destroy_professor_label_texture(int index)
{
    if (index < 0 || index >= PROFESSOR_TEXTURE_COUNT)
    {
        return;
    }
    if (g_professor_label_textures[index])
    {
        SDL_DestroyTexture(g_professor_label_textures[index]);
        g_professor_label_textures[index] = NULL;
    }
    g_professor_label_width[index] = 0;
    g_professor_label_height[index] = 0;
}

static void rebuild_professor_label_texture(int index)
{
    if (index < 0 || index >= PROFESSOR_TEXTURE_COUNT)
    {
        return;
    }

    destroy_professor_label_texture(index);

    if (!g_professor_label_font || !g_renderer)
    {
        return;
    }
    if (g_professor_label_texts[index][0] == '\0')
    {
        return;
    }

    SDL_Color color = {245, 245, 245, 255};
    SDL_Surface *surface = TTF_RenderUTF8_Blended(g_professor_label_font,
                                                  g_professor_label_texts[index],
                                                  color);
    if (!surface)
    {
        fprintf(stderr, "TTF_RenderUTF8_Blended failed for label %d: %s\n", index, TTF_GetError());
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(g_renderer, surface);
    if (!texture)
    {
        fprintf(stderr, "SDL_CreateTextureFromSurface failed for label %d: %s\n", index, SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    g_professor_label_width[index] = surface->w;
    g_professor_label_height[index] = surface->h;
    SDL_FreeSurface(surface);
    g_professor_label_textures[index] = texture;
}

static void rebuild_all_professor_label_textures(void)
{
    for (int i = 0; i < PROFESSOR_TEXTURE_COUNT; ++i)
    {
        rebuild_professor_label_texture(i);
    }
}

static void destroy_all_professor_label_textures(void)
{
    for (int i = 0; i < PROFESSOR_TEXTURE_COUNT; ++i)
    {
        destroy_professor_label_texture(i);
    }
}

static void extract_professor_label_from_path(const char *path, char *out, size_t out_size)
{
    if (!out || out_size == 0)
    {
        return;
    }

    out[0] = '\0';
    if (!path)
    {
        return;
    }

    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    size_t len = strcspn(filename, ".");
    if (len >= out_size)
    {
        len = out_size - 1;
    }

    memcpy(out, filename, len);
    out[len] = '\0';
}

static void cache_professor_label_text(int index, const char *texture_path)
{
    if (index < 0 || index >= PROFESSOR_TEXTURE_COUNT)
    {
        return;
    }

    extract_professor_label_from_path(texture_path,
                                      g_professor_label_texts[index],
                                      sizeof(g_professor_label_texts[index]));

    if (g_professor_label_font && g_renderer)
    {
        rebuild_professor_label_texture(index);
    }
}

static int get_professor_label_index_for_stage(int stage_identifier)
{
    int idx = stage_identifier - 1;
    if (idx < 0 || idx >= PROFESSOR_TEXTURE_COUNT)
    {
        return -1;
    }

    if (g_professor_label_texts[idx][0] == '\0')
    {
        return -1;
    }
    return idx;
}

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

// 교수 장애물의 머리 위에 반투명 배지를 띄워 이름을 표시한다.
static void draw_professor_nameplate(int label_index, double world_x, double world_y, const Camera *camera)
{
    if (!g_renderer || !camera)
    {
        return;
    }
    if (label_index < 0 || label_index >= PROFESSOR_TEXTURE_COUNT)
    {
        return;
    }

    SDL_Texture *label_tex = g_professor_label_textures[label_index];
    int text_w = g_professor_label_width[label_index];
    int text_h = g_professor_label_height[label_index];
    if (!label_tex || text_w <= 0 || text_h <= 0)
    {
        return;
    }

    const int tile_size = camera->tile_size;
    const int screen_x = camera->viewport_offset_x + (int)lround(world_x * tile_size - camera->pixel_x);
    const int screen_y = camera->viewport_offset_y + (int)lround(world_y * tile_size - camera->pixel_y);

    const int label_x = screen_x + (tile_size - text_w) / 2;
    const int label_y = screen_y - text_h - PROFESSOR_LABEL_ABOVE_OFFSET;

    SDL_Rect background = {label_x - PROFESSOR_LABEL_PADDING_X,
                           label_y - PROFESSOR_LABEL_PADDING_Y,
                           text_w + PROFESSOR_LABEL_PADDING_X * 2,
                           text_h + PROFESSOR_LABEL_PADDING_Y * 2};

    if (!is_rect_visible(&background))
    {
        return;
    }

    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_renderer, 10, 10, 10, 180);
    SDL_RenderFillRect(g_renderer, &background);

    SDL_Rect dst = {label_x, label_y, text_w, text_h};
    SDL_RenderCopy(g_renderer, label_tex, NULL, &dst);
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
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

    if (TTF_Init() != 0)
    {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    g_ttf_initialized = 1;

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

    g_professor_label_font = open_professor_label_font();
    if (!g_professor_label_font)
    {
        shutdown_renderer();
        return -1;
    }

    g_ui_font_large = open_additional_ui_font(40);
    g_ui_font_small = open_additional_ui_font(22);
    if (!g_ui_font_large || !g_ui_font_small)
    {
        shutdown_renderer();
        return -1;
    }

    if (build_title_menu_textures() != 0 || build_static_textures() != 0)
    {
        shutdown_renderer();
        return -1;
    }

    update_tile_render_metrics();

    g_tex_floor = load_texture("assets/image/floor64.png");
    g_tex_wall = load_texture("assets/image/wall64.png");
    g_tex_goal = load_texture("assets/image/backpack64.png");
    g_tex_exit = load_texture("assets/image/exit.PNG");

    g_tex_professor_1 = load_texture("assets/image/김명석교수님.png");
    cache_professor_label_text(0, "assets/image/김명석교수님.png");
    g_tex_professor_2 = load_texture("assets/image/이종택교수님.png");
    cache_professor_label_text(1, "assets/image/이종택교수님.png");
    g_tex_professor_3 = load_texture("assets/image/김진욱교수님.png");
    cache_professor_label_text(2, "assets/image/김진욱교수님.png");
    g_tex_professor_4 = load_texture("assets/image/김명옥교수님.png");
    cache_professor_label_text(3, "assets/image/김명옥교수님.png");
    g_tex_professor_5 = load_texture("assets/image/김정근교수님.png");
    cache_professor_label_text(4, "assets/image/김정근교수님.png");
    g_tex_professor_6 = load_texture("assets/image/한명균교수님.png");
    cache_professor_label_text(5, "assets/image/한명균교수님.png");

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

    g_tex_trap = load_texture("assets/image/floor64.png");         // 트랩 (일반타일로 의문사 또는 실제 보이게 해서 못 지나가도록)
    g_tex_wall_break = load_texture("assets/image/wall64.png"); // 깨지는 벽

    g_tex_projectile = load_texture("assets/image/ball.png");       // 플레이어 투사체
    g_tex_professor_bullet = load_texture("assets/image/bullet.png"); // 교수 스킬 탄환
    g_tex_shield_on = load_texture("assets/image/shieldon64.png");  // 보호막 활성화 표현
    g_tex_menu_background = load_texture("assets/image/menu.png");

    const char *game_over_candidates[] = {
        "assets/image/gameover.png",
        "assets/image/gameover.PNG",
        "assets/image/over.png",
        "assets/image/over.PNG"};
    const char *game_over_path = NULL;
    for (size_t i = 0; i < sizeof(game_over_candidates) / sizeof(game_over_candidates[0]); ++i)
    {
        if (is_file_readable(game_over_candidates[i]))
        {
            game_over_path = game_over_candidates[i];
            break;
        }
    }
    if (game_over_path)
    {
        g_tex_game_over_image = load_texture(game_over_path);
    }

    if (!g_tex_projectile || !g_tex_professor_bullet || !g_tex_item_shield || !g_tex_item_scooter || !g_tex_shield_on)
        return -1;

    if (!g_tex_floor || !g_tex_wall || !g_tex_goal || !g_tex_professor_1 || !g_tex_exit ||
        !g_tex_student_w_left || !g_tex_student_w_right || !g_tex_student_m_left ||
        !g_tex_student_m_right || !g_tex_pulpit || !g_tex_menu_background ||
        !g_tex_game_over_image)
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
    destroy_title_screen_artifacts();
    destroy_overlay_textures();
    destroy_all_professor_label_textures();
    if (g_professor_label_font)
    {
        TTF_CloseFont(g_professor_label_font);
        g_professor_label_font = NULL;
    }
    if (g_ui_font_large)
    {
        TTF_CloseFont(g_ui_font_large);
        g_ui_font_large = NULL;
    }
    if (g_ui_font_small)
    {
        TTF_CloseFont(g_ui_font_small);
        g_ui_font_small = NULL;
    }

    destroy_texture(&g_tex_floor);
    destroy_texture(&g_tex_wall);
    destroy_texture(&g_tex_goal);
    destroy_texture(&g_tex_exit);
    destroy_texture(&g_tex_menu_background);
    destroy_texture(&g_tex_game_over_image);

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

    destroy_texture(&g_tex_projectile);        // 플레이어 투사체
    destroy_texture(&g_tex_professor_bullet);  // 교수 탄환
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
    if (g_ttf_initialized)
    {
        TTF_Quit();
        g_ttf_initialized = 0;
    }
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

// 상단 HUD는 플레이 진행 정보(시간/남은 탄/스쿠터 타이머)를 카메라와 무관하게 고정 좌표에 그린다.
// 상단 HUD는 플레이 진행 정보(스테이지/시간/남은 탄/스쿠터 타이머)를 고정 좌표에 출력한다.
static void render_hud(const Stage *stage, const Player *player, double elapsed_time)
{
    if (!g_renderer || !stage || !player)
    {
        return;
    }

    SDL_SetRenderDrawColor(g_renderer, 245, 245, 245, 255);

    int line_y = HUD_MARGIN;

    char stage_text[32];
    const char *stage_label = stage_label_for_id(stage->id);
    snprintf(stage_text, sizeof(stage_text), "STAGE %s", stage_label);
    draw_hud_text(stage_text, HUD_MARGIN, line_y);

    line_y += HUD_LINE_SPACING;

    char time_text[32];
    int minutes = (int)(elapsed_time / 60.0);
    int seconds = (int)fmod(elapsed_time, 60.0);
    if (seconds < 0)
        seconds = 0;
    snprintf(time_text, sizeof(time_text), "TIME %02d:%02d", minutes, seconds);

    draw_hud_text(time_text, HUD_MARGIN, line_y);

    line_y += HUD_LINE_SPACING;

    const int ammo_icon_size = 16;
    const int ammo_spacing = 4;
    int ammo_line_height = ammo_icon_size;
    if (g_tex_projectile && stage->remaining_ammo > 0)
    {
        SDL_Rect ammo_dst = {HUD_MARGIN, line_y, ammo_icon_size, ammo_icon_size};
        for (int i = 0; i < stage->remaining_ammo; ++i)
        {
            ammo_dst.x = HUD_MARGIN + i * (ammo_icon_size + ammo_spacing);
            SDL_RenderCopy(g_renderer, g_tex_projectile, NULL, &ammo_dst);
        }
    }
    else if (g_tex_projectile)
    {
        SDL_Rect ammo_dst = {HUD_MARGIN, line_y, ammo_icon_size, ammo_icon_size};
        SDL_SetTextureColorMod(g_tex_projectile, 255, 80, 80);
        SDL_RenderCopy(g_renderer, g_tex_projectile, NULL, &ammo_dst);
        SDL_SetTextureColorMod(g_tex_projectile, 255, 255, 255);
    }
    else
    {
        char ammo_text[32];
        snprintf(ammo_text, sizeof(ammo_text), "BALLS %d", stage->remaining_ammo);
        draw_hud_text(ammo_text, HUD_MARGIN, line_y);
        ammo_line_height = HUD_FONT_HEIGHT * HUD_FONT_SCALE;
    }

    line_y += ammo_line_height + 8;

    if (player->has_scooter)
    {
        SDL_Texture *scooter_icon = g_tex_item_scooter ? g_tex_item_scooter : g_tex_projectile;
        SDL_Rect scooter_rect = {HUD_MARGIN, line_y, 18, 18};
        if (scooter_icon)
        {
            SDL_RenderCopy(g_renderer, scooter_icon, NULL, &scooter_rect);
        }
        else
        {
            SDL_RenderFillRect(g_renderer, &scooter_rect);
        }

        double scooter_remaining = player->scooter_expire_time - elapsed_time;
        if (scooter_remaining < 0.0)
        {
            scooter_remaining = 0.0;
        }

        int scooter_total_seconds = (int)(scooter_remaining + 0.5);
        int scooter_minutes = scooter_total_seconds / 60;
        int scooter_seconds = scooter_total_seconds % 60;

        char scooter_text[32];
        snprintf(scooter_text, sizeof(scooter_text), ": %02d:%02d", scooter_minutes, scooter_seconds);
        draw_hud_text(scooter_text, scooter_rect.x + scooter_rect.w + 8, line_y);
    }

    SDL_SetRenderDrawColor(g_renderer, 15, 15, 15, 255);
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
    int current_prof_label_index = get_professor_label_index_for_stage(stage_identifier);

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
            if (o->kind == OBSTACLE_KIND_PROFESSOR && current_prof_label_index >= 0)
            {
                draw_professor_nameplate(current_prof_label_index, obstacle_world_x, obstacle_world_y, &camera);
            }
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

    if (g_tex_professor_bullet)
    {
        for (int i = 0; i < MAX_PROFESSOR_BULLETS; ++i)
        {
            const ProfessorBullet *bullet = &stage->professor_bullets[i];
            if (!bullet->active)
            {
                continue;
            }
            double tile_x = bullet->world_x;
            double tile_y = bullet->world_y;
            int cell_x = (int)floor(tile_x);
            int cell_y = (int)floor(tile_y);
            if (cell_x < 0 || cell_y < 0 || cell_x >= stage_width || cell_y >= stage_height)
            {
                continue;
            }
            if (!visibility[cell_y][cell_x])
            {
                continue;
            }
            draw_texture_at_world(g_tex_professor_bullet, tile_x, tile_y, &camera);
        }
    }

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

    render_hud(stage, player, elapsed_time);

    // 패턴 확인용 주석처리
    SDL_RenderPresent(g_renderer);

    (void)current_stage;
    (void)total_stages;
    (void)elapsed_time;
}

void render_title_screen(int selected_index)
{
    if (!g_renderer)
    {
        return;
    }

    if (selected_index < 0 || selected_index >= ARRAY_LEN(g_title_menu_options))
    {
        selected_index = 0;
    }

    SDL_SetRenderDrawColor(g_renderer, 6, 6, 12, 255);
    SDL_RenderClear(g_renderer);

    if (g_tex_menu_background)
    {
        SDL_Rect image_dst = {0, 0, WINDOW_WIDTH - 420, WINDOW_HEIGHT};
        SDL_RenderCopy(g_renderer, g_tex_menu_background, NULL, &image_dst);
    }

    SDL_Color title_color = {255, 255, 255, 255};
    update_cached_text(&g_title_logo_cache, get_large_ui_font(), "교수님 피하기", title_color);
    if (g_title_logo_cache.texture)
    {
        const int scaled_w = g_title_logo_cache.width * 2;
        const int scaled_h = g_title_logo_cache.height * 2;
        SDL_Rect logo_dst = {WINDOW_WIDTH / 2 - scaled_w / 2,
                             30,
                             scaled_w,
                             scaled_h};
        SDL_RenderCopy(g_renderer, g_title_logo_cache.texture, NULL, &logo_dst);
    }

    SDL_Rect panel = {WINDOW_WIDTH - 380, 60, 320, WINDOW_HEIGHT - 120};
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_renderer, 18, 18, 30, 240);
    SDL_RenderFillRect(g_renderer, &panel);

    const int base_y = panel.y + 120;
    const int option_spacing = 110;
    for (int i = 0; i < ARRAY_LEN(g_title_menu_options); ++i)
    {
        const MenuOptionTexture *option = &g_title_menu_options[i];
        SDL_Texture *tex = (i == selected_index) ? option->highlight : option->normal;
        if (!tex)
        {
            continue;
        }

        const int dst_x = panel.x + (panel.w - option->width) / 2;
        const int dst_y = base_y + i * option_spacing;
        if (i == selected_index)
        {
            SDL_Rect highlight = {panel.x + 20, dst_y - 18, panel.w - 40, option->height + 36};
            SDL_SetRenderDrawColor(g_renderer, 90, 120, 210, 110);
            SDL_RenderFillRect(g_renderer, &highlight);
        }

        SDL_Rect dst = {dst_x, dst_y, option->width, option->height};
        SDL_RenderCopy(g_renderer, tex, NULL, &dst);
    }

    if (g_title_hint_texture.texture)
    {
        SDL_Rect hint = {panel.x + (panel.w - g_title_hint_texture.width) / 2,
                         panel.y + panel.h - g_title_hint_texture.height - 24,
                         g_title_hint_texture.width,
                         g_title_hint_texture.height};
        SDL_RenderCopy(g_renderer, g_title_hint_texture.texture, NULL, &hint);
    }

    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
    SDL_RenderPresent(g_renderer);
}

void render_records_screen(double best_time)
{
    if (!g_renderer)
    {
        return;
    }

    SDL_SetRenderDrawColor(g_renderer, 5, 5, 9, 255);
    SDL_RenderClear(g_renderer);

    if (g_tex_menu_background)
    {
        SDL_Rect image_dst = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
        SDL_RenderCopy(g_renderer, g_tex_menu_background, NULL, &image_dst);
    }

    SDL_Rect overlay = {40, 40, WINDOW_WIDTH - 80, WINDOW_HEIGHT - 80};
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_renderer, 8, 8, 12, 220);
    SDL_RenderFillRect(g_renderer, &overlay);

    char record_text[64];
    if (best_time > 0.0)
    {
        snprintf(record_text, sizeof(record_text), "최고 기록: %.3fs", best_time);
    }
    else
    {
        snprintf(record_text, sizeof(record_text), "기록 없음");
    }

    SDL_Color text_color = {240, 240, 240, 255};
    update_cached_text(&g_records_best_cache, get_large_ui_font(), record_text, text_color);
    if (g_records_best_cache.texture)
    {
        SDL_Rect dst = {
            WINDOW_WIDTH / 2 - g_records_best_cache.width / 2,
            WINDOW_HEIGHT / 2 - g_records_best_cache.height / 2,
            g_records_best_cache.width,
            g_records_best_cache.height};
        SDL_RenderCopy(g_renderer, g_records_best_cache.texture, NULL, &dst);
    }

    if (g_records_hint_texture.texture)
    {
        SDL_Rect dst = {WINDOW_WIDTH / 2 - g_records_hint_texture.width / 2,
                         overlay.y + overlay.h - g_records_hint_texture.height - 30,
                         g_records_hint_texture.width,
                         g_records_hint_texture.height};
        SDL_RenderCopy(g_renderer, g_records_hint_texture.texture, NULL, &dst);
    }

    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
    SDL_RenderPresent(g_renderer);
}

void render_game_over_screen(void)
{
    if (!g_renderer)
    {
        return;
    }

    SDL_SetRenderDrawColor(g_renderer, 4, 2, 2, 255);
    SDL_RenderClear(g_renderer);

    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_renderer, 80, 10, 10, 190);
    SDL_Rect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(g_renderer, &overlay);

    SDL_Color title_color = {255, 120, 120, 255};
    update_cached_text(&g_game_over_title_cache, get_large_ui_font(), "재수강", title_color);
    if (g_game_over_title_cache.texture)
    {
        const int scaled_w = g_game_over_title_cache.width * 2;
        const int scaled_h = g_game_over_title_cache.height * 2;
        SDL_Rect dst = {WINDOW_WIDTH / 2 - scaled_w / 2,
                         60,
                         scaled_w,
                         scaled_h};
        SDL_RenderCopy(g_renderer, g_game_over_title_cache.texture, NULL, &dst);
    }

    const int image_top = 170;
    const int image_bottom_margin = 150;
    if (g_tex_game_over_image)
    {
        int tex_w = 0;
        int tex_h = 0;
        if (SDL_QueryTexture(g_tex_game_over_image, NULL, NULL, &tex_w, &tex_h) != 0)
        {
            tex_w = WINDOW_WIDTH / 2;
            tex_h = WINDOW_HEIGHT / 2;
        }

        int max_w = WINDOW_WIDTH - 200;
        int max_h = WINDOW_HEIGHT - image_top - image_bottom_margin;
        if (max_w < 100)
            max_w = 100;
        if (max_h < 100)
            max_h = 100;

        double scale = 1.0;
        if (tex_w > 0 && tex_h > 0)
        {
            double scale_w = (double)max_w / (double)tex_w;
            double scale_h = (double)max_h / (double)tex_h;
            scale = fmin(1.0, fmin(scale_w, scale_h));
        }

        int draw_w = (int)(tex_w * scale);
        int draw_h = (int)(tex_h * scale);
        if (draw_w <= 0 || draw_h <= 0)
        {
            draw_w = max_w;
            draw_h = max_h;
        }

        SDL_Rect img_dst = {
            WINDOW_WIDTH / 2 - draw_w / 2,
            image_top + (max_h - draw_h) / 2,
            draw_w,
            draw_h};
        SDL_RenderCopy(g_renderer, g_tex_game_over_image, NULL, &img_dst);
    }

    if (g_game_over_hint_texture.texture)
    {
        SDL_Rect dst = {WINDOW_WIDTH / 2 - g_game_over_hint_texture.width / 2,
                         WINDOW_HEIGHT - g_game_over_hint_texture.height - 40,
                         g_game_over_hint_texture.width,
                         g_game_over_hint_texture.height};
        SDL_RenderCopy(g_renderer, g_game_over_hint_texture.texture, NULL, &dst);
    }

    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
    SDL_RenderPresent(g_renderer);
}
