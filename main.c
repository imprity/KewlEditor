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
        WIDTH, HEIGHT, font, renderer);*/
    TextBox* box = text_box_create("", WIDTH, HEIGHT, font, renderer);

    ////////////////////////////////
    // event loop
    ////////////////////////////////
    
    SDL_Rect rect = { .x = 0,.y = 0,.w = WIDTH, .h = HEIGHT };
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
            }

            case SDL_TEXTEDITING: {
                /////////////////////
                // dump text editing event
                /////////////////////
                
                //printf("\n");
                //printf(u8"event time tamp : %u\n", event.edit.timestamp);
                //printf(u8"window id       : %u\n", event.edit.windowID);
                //printf(u8"edit text       : %s\n", event.edit.text);
                //printf(u8"edit start loc  : %d\n", event.edit.start);
                //printf(u8"edit length     : %d\n", event.edit.length);
                //printf("\n");

                /////////////////////
                // end of dumping
                /////////////////////
            }break;
            case SDL_KEYDOWN: {
                SDL_Keysym key = event.key.keysym;
                if (key.scancode == SDL_SCANCODE_RETURN) {
                    text_box_type(box, u8"\n");
                    text_changed = true;
                }
                if (key.scancode == SDL_SCANCODE_LEFT) {
                    text_box_move_cursor_left(box);
                    text_changed = true;
                }
                if (key.scancode == SDL_SCANCODE_RIGHT) {
                    text_box_move_cursor_right(box);
                    text_changed = true;
                }
                if (key.scancode == SDL_SCANCODE_UP) {
                    text_box_move_cursor_up(box);
                    text_changed = true;
                }
                if (key.scancode == SDL_SCANCODE_DOWN) {
                    text_box_move_cursor_down(box);
                    text_changed = true;
                }
                if (key.scancode == SDL_SCANCODE_BACKSPACE) {
                    text_box_delete_a_character(box);
                    text_changed = true;
                }
                ////////////////////////////
                //for testing copy and pating
                ////////////////////////////
                if (key.scancode == SDL_SCANCODE_F1) {
                    text_box_type(box,
                        u8"very long text\n"
                        u8"so so very long!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
                        u8"This is a test string to see if my text editor can handle copy and pasting\n"
                        u8"press f1!!\n"
                    );
                    text_changed = true;
                }
                if (key.scancode == SDL_SCANCODE_F2) {
                    text_box_type(box,
                        u8"인간의 천자만홍이 그러므로 곳이 시들어 것은 것이다. 것은 구할 사라지지 얼음에 하였으며, 말이다. 있는 보내는 곧 봄바람이다. 불어 인간의 가슴에 구할 몸이 끓는다. 불어 기관과 뜨거운지라, 쓸쓸한 동력은 봄바람이다. 없으면 할지라도 청춘에서만 사랑의 곧 품으며, 사는가 속에 쓸쓸하랴? 이상 커다란 끓는 희망의 미인을 것은 찬미를 부패뿐이다. 하여도 인생을 되려니와, 자신과 가는 보배를 가는 방황하였으며, 보라. 청춘의 안고, 따뜻한 것이다. 그들의 오아이스도 튼튼하며, 위하여, 우는 힘차게 얼마나 하여도 새가 듣는다. 청춘 놀이 가장 앞이 발휘하기 주는 구할 때문이다.\n"
                    );
                    text_changed = true;
                }
                ////////////////////////////
                //for testing copy and pating
                ////////////////////////////
                if (key.scancode == SDL_SCANCODE_F3) {
                    if (box->is_selecting) {
                        text_box_end_selection(box);
                        printf("selection off\n");
                    }
                    else {
                        text_box_start_selection(box);
                        printf("selection on\n");
                    }
                    text_changed = true;
                }
            }break;
            }
        }
        if (text_changed) {
            text_box_render(box);
            text_changed = false;
        }
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderClear(renderer);

        SDL_Rect rect = { .x = 0, .y = 0, .w = WIDTH, .h = HEIGHT };
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