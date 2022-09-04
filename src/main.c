#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <locale.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "OS.h"

#define TEST_FONT "NotoSansKR-Medium.otf"

#define WIDTH 600
#define HEIGHT 500

#include "TextBox.h"

#include "linux/LinuxMain.h"


int main(int argc, char* argv[])
{
    text_line_test();
    utf_test();

    bool init_success = true;

    ////////////////////////////////
    // load font
    ////////////////////////////////
    if (TTF_Init() < 0)
    {
        printf("ERROR: Failed to initializing SDL_TTF: %s\n", SDL_GetError());
        init_success = false;
    }

    TTF_Font* font = TTF_OpenFont(TEST_FONT, 20);
    if (!font)
    {
        printf("ERROR: Failed to load font: %s. %s\n", TEST_FONT,SDL_GetError());
        init_success = false;
    }
    ////////////////////////////////
    // create text box
    ////////////////////////////////

    SDL_Color bg_color = { .r = 30, .g = 30, .b = 30, .a = 255 };
    SDL_Color selection_bg = { .r = 240, .g = 240, .b = 240, .a = 255 };
    SDL_Color selection_fg = { .r = 40, .g = 40, .b = 40, .a = 255 };
    SDL_Color text_color = { .r = 240, .g = 240, .b = 240, .a = 255 };
    SDL_Color cursor_color = { .r = 240, .g = 240, .b = 240, .a = 255 };

    TextBox* box = text_box_create("", WIDTH, HEIGHT, font,
                                   bg_color, text_color, selection_bg, selection_fg, cursor_color,
                                   NULL);

    if (!box)
    {
        printf("ERROR: Failed to create text box\n");
        init_success = false;
    }

    if (!init_success)
    {
        goto cleanup;
    }

    text_box_render(box);

    int ret_val = linux_main(box, argc, argv);

cleanup :
    if (font)
    {
        TTF_CloseFont(font);
    }
    if (box)
    {
        text_box_destroy(box);
    }
    TTF_Quit();
    SDL_Quit();

    return ret_val;
}

