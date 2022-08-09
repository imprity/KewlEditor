#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define TEST_FONT "NotoSansKR-Medium.otf"

#include "UTFString.h"
#include "TextBox.h"
#include "TextLine.h"
#include <Windows.h>


int main(int argc, char* argv[])
{
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
    

    SDL_Window* win = SDL_CreateWindow("GAME",
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

    TTF_Font* font = TTF_OpenFont(TEST_FONT, 24);
    if (!font) {
        printf("ERROR: Failed to load font: %s. %s\n", TEST_FONT,SDL_GetError());
        init_success = false;
    }
    ////////////////////////////////
    // end of load font
    ////////////////////////////////

    /*TextBox* box = text_box_create(
        u8"This Is a very long sample text Yo.\n"
        u8"So very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, very, very\n"
        u8"\n"
        u8"\n"
        u8"\n"
        u8"\n"
        u8"LOOOOOOOOOOOOOOOONG~~~~~~\n", 
        600, 600, font, renderer);*/
    TextBox* box = text_box_create("", 600, 600, font, renderer);

    ////////////////////////////////
    // event loop
    ////////////////////////////////
    
    SDL_Rect rect = { .x = 0,.y = 0,.w = 600, .h = 600 };
    SDL_SetTextInputRect(&rect);
    SDL_StartTextInput();
    assert(SDL_IsTextInputActive());
    SDL_Event event;
    bool text_changed = true;
    bool quit = false;

    if (!init_success) {
        goto cleanup;
    }

    text_box_render(box);

    while  (!quit) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {

            case SDL_QUIT: {
                quit = true;
            }break;
            
            case SDL_TEXTINPUT: {
                text_box_type(box, event.text.text);
                text_changed = true;
                text_box_render(box, renderer);
            }

            case SDL_TEXTEDITING: {
            }break;
            case SDL_KEYDOWN: {
                SDL_Keysym key = event.key.keysym;
                if (key.scancode == SDL_SCANCODE_RETURN) {
                    text_box_type(box, u8"\n");
                    text_box_render(box);
                }
                if (key.scancode == SDL_SCANCODE_LEFT) {
                    text_box_move_cursor_left(box);
                    text_box_render(box);
                }
                if (key.scancode == SDL_SCANCODE_RIGHT) {
                    text_box_move_cursor_right(box);
                    text_box_render(box);
                }
            }break;
            }
        }
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderClear(renderer);

        SDL_Rect rect = { .x = 0, .y = 0, .w = 600, .h = 600 };
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(renderer, box->texture, NULL, NULL);
        
        SDL_RenderPresent(renderer);
    }

cleanup :
    if (win) { SDL_DestroyWindow(win); }
    if (renderer) { SDL_DestroyRenderer(renderer); }
    if (font) { TTF_CloseFont(font); }
    if (box) { text_box_destroy(box); }
    SDL_Quit();

    return 0;
}