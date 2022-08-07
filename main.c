#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define TEST_FONT "NotoSansKR-Medium.otf"

#include "UTFString.h"
#include <Windows.h>


int main(int argc, char* argv[])
{
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
        600, 600, SDL_WINDOW_RESIZABLE);

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

    UTFString* edit_str = utf_create(u8"시작 : ");

    ////////////////////////////////
    // event loop
    ////////////////////////////////
    
    SDL_Rect rect = { .x = 0,.y = 0,.w = 300, .h = 300 };
    SDL_SetTextInputRect(&rect);
    SDL_StartTextInput();
    //assert(SDL_IsTextInputActive());
    SDL_Event event;
    bool quit = false;
    bool text_changed = true;
    SDL_Texture* text_texture = NULL;
    int texture_width = 0;
    int texture_height = 0;

    if (!init_success) {
        goto cleanup;
    }

    while  (!quit) {
        
        while (SDL_PollEvent(&event)) {
            switch (event.type) {

            case SDL_QUIT: {
                quit = true;
            }break;
            
            case SDL_TEXTINPUT: {
                utf_append(edit_str, event.text.text);
                text_changed = true;
                printf("text edited!!\n");
            }

            case SDL_TEXTEDITING: {
            }break;
            }
        }
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderClear(renderer);

        if (text_changed) {
            if (text_texture) {
                SDL_DestroyTexture(text_texture);
            }
            SDL_Color color = { 0,0,0,255 };
            SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(font, edit_str->data, color, 500);

            texture_width = surface->w;
            texture_height = surface->h;
            text_texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (!text_texture) {
                printf("ERROR: Failed to create texture form surface!!! : %s\n", SDL_GetError());
                goto cleanup;
            }
            SDL_FreeSurface(surface);
            text_changed = false;
        }

        if (text_texture) {
            SDL_Rect rect = {.x = 10,.y = 10,.w = texture_width,.h = texture_height };
            SDL_RenderCopy(renderer, text_texture, NULL, &rect);
        }
        
        SDL_RenderPresent(renderer);
    }

cleanup :
    if (win) { SDL_DestroyWindow(win); }
    if (renderer) { SDL_DestroyRenderer(renderer); }
    if (font) { TTF_CloseFont(font); }
    if (edit_str) { utf_destroy(edit_str); }
    if (text_texture) { SDL_DestroyTexture(text_texture); }
    SDL_Quit();

    return 0;
}