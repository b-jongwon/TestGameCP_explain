#include <SDL2/SDL.h>

#include "signal_handler.h"
#include "input.h"

static int translate_key(SDL_Keycode key)
{
    switch (key)
    {
        case SDLK_w:
        case SDLK_UP:
            return 'w';
        case SDLK_s:
        case SDLK_DOWN:
            return 's';
        case SDLK_a:
        case SDLK_LEFT:
            return 'a';
        case SDLK_d:
        case SDLK_RIGHT:
            return 'd';
        case SDLK_q:
        case SDLK_ESCAPE:
            return 'q';
        default:
            return (key >= 0 && key < 128) ? (int)key : -1;
    }
}

void init_input(void)
{
    SDL_StartTextInput();
}

void restore_input(void)
{
    SDL_StopTextInput();
}

int read_input(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            g_running = 0;
            return -1;
        }

        if (event.type == SDL_KEYDOWN && !event.key.repeat)
        {
            int mapped = translate_key(event.key.keysym.sym);
            if (mapped != -1)
            {
                return mapped;
            }
        }
    }

    return -1;
}

int poll_input(void)
{
    return read_input();
}
