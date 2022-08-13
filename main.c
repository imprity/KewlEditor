#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define TEST_FONT "NotoSansKR-Medium.otf"

#define WIDTH 600
#define HEIGHT 600

#include "UTFString.h"
#include "TextBox.h"
#include "TextLine.h"
#include <Windows.h>


int main(int argc, char* argv[])
{
    SetConsoleOutputCP(65001);

    text_line_test();
    utf_test();
    bool init_success = true;
    ////////////////////////////////
    // init sdl
    ////////////////////////////////
    // returns zero on success else non-zero
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("ERROR: Failed to initializing SDL: %s\n", SDL_GetError());
        init_success = false;
    }

    if (!SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1")) {
        printf("ERROR: Failed to set SDL_HINT_IME_SHOW_UI\n");
        init_success = false;
    }
    if (!SDL_SetHint(SDL_HINT_IME_INTERNAL_EDITING, "1")) {
        printf("ERROR: Failed to set SDL_HINT_IME_INTERNAL_EDITING\n");
        init_success = false;
    }
    

    SDL_Window* win = SDL_CreateWindow("Kewl Editor",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        //600, 600, SDL_WINDOW_RESIZABLE);
        600, 600, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        printf("ERROR: Failed to initializing SDL renderer: %s\n", SDL_GetError());
        init_success = false;
    }

    ////////////////////////////////
    // load font
    ////////////////////////////////
    if (TTF_Init() < 0) {
        printf("ERROR: Failed to initializing SDL_TTF: %s\n", SDL_GetError());
        init_success = false;
    }

    TTF_Font* font = TTF_OpenFont(TEST_FONT, 20);
    if (!font) {
        printf("ERROR: Failed to load font: %s. %s\n", TEST_FONT,SDL_GetError());
        init_success = false;
    }
    ////////////////////////////////
    // create text box
    ////////////////////////////////

    /*TextBox* box = text_box_create(
        u8"This Is a very long sample text Yo.\n"
        u8"So very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, very\n"
        u8"\n"
        u8"\n"
        u8"\n"
        u8"\n"
        u8"LOOOOOOOOOOOOOOOONG~~~~~~\n", 
        WIDTH, HEIGHT, font, renderer);*/

    SDL_Color bg_color = { .r = 30, .g = 30, .b = 30, .a = 255 };
    SDL_Color selection_bg = { .r = 240, .g = 240, .b = 240, .a = 255 };
    SDL_Color selection_fg = { .r = 40, .g = 40, .b = 40, .a = 255 };
    SDL_Color text_color = { .r = 240, .g = 240, .b = 240, .a = 255 };
    SDL_Color cursor_color = { .r = 240, .g = 240, .b = 240, .a = 255 };

    TextBox* box = text_box_create("", WIDTH, HEIGHT, font, 
        bg_color, text_color, selection_bg, selection_fg, cursor_color, 
        renderer);

    ////////////////////////////////
    // event loop
    ////////////////////////////////
    
    SDL_Rect rect = { .x = 0,.y = 0,.w = WIDTH, .h = HEIGHT };
    SDL_SetTextInputRect(&rect);
    SDL_StartTextInput();
    assert(SDL_IsTextInputActive());
    SDL_Event event;
    bool quit = false;

    if (!init_success) {
        goto cleanup;
    }

    text_box_render(box);

    while  (!quit) {

        SDL_WaitEvent(&event);
        uint64_t start_time = SDL_GetTicks64();

        //while (SDL_PollEvent(&event)) {
        text_box_handle_event(box, &event);

        switch (event.type) {

        case SDL_QUIT: {
            quit = true;
        }break;

        }

        text_box_render(box);

        SDL_RenderClear(renderer);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(renderer, box->texture, NULL, NULL);
        
        SDL_RenderPresent(renderer);
        
        uint64_t end_time = (SDL_GetTicks64() - start_time);
        double fps = end_time == 0 ? 0 : 1000.0 / (double)(end_time);
        static char tmp_buffer[1024];
        sprintf_s(tmp_buffer, 1023, "Kewl Editor  FPS : %5.2lf", fps);
        SDL_SetWindowTitle(win, tmp_buffer);
    }

cleanup :
    if (win) { SDL_DestroyWindow(win); }
    if (renderer) { SDL_DestroyRenderer(renderer); }
    if (font) { TTF_CloseFont(font); }
    if (box) { text_box_destroy(box); }
    TTF_Quit();
    SDL_Quit();

    return 0;
}