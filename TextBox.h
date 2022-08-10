#ifndef TextBox_HEADER_GUARD
#define TextBox_HEADER_GUARD

#include "UTFString.h"
#include "TextLine.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL.h>

typedef struct TextBox{
    int w;
    int h;

    TextLine* cursor_line;
    TextLine* first_line;

    size_t cursor_offset_x;
    size_t cursor_offset_y;

    TTF_Font* font;
    SDL_Texture* texture;

    SDL_Renderer* renderer;
    int offset_y;
}TextBox;

TextBox* text_box_create(const char* text, size_t w, size_t h, TTF_Font* font, SDL_Renderer* renderer);
void text_box_destroy(TextBox* box);

void text_box_type(TextBox* box, char* c);

void text_box_render(TextBox* box);

void text_box_move_cursor_left(TextBox* box);
void text_box_move_cursor_right(TextBox* box);
void text_box_move_cursor_up(TextBox* box);
void text_box_move_cursor_down(TextBox* box);

void text_box_delete_a_character(TextBox* box);

#endif