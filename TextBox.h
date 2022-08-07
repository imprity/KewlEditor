#ifndef TextBox_HEADER_GUARD
#define TextBox_HEADER_GUARD

#include "UTFString.h"
#include <SDL2/SDL_ttf.h>

typedef struct TextBox{
    UTFString* str;
    size_t w;
    size_t h;
    size_t str_start;
    size_t str_end;
    size_t cursor_pos;
    TTF_Font* font;
}TextBox;

#endif