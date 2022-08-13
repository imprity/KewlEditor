#ifndef TextBox_HEADER_GUARD
#define TextBox_HEADER_GUARD

#include "UTFString.h"
#include "TextLine.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL.h>


typedef struct Cursor {
    size_t line_number;
    size_t char_offset;

    /// <summary>
    /// This is for the special case when cursor has to be
    /// at the end of the line wrapped character
    /// 
    /// For most cases, if cursor is at the end of the wrapped line
    /// cursor should move to the start of the next line
    /// 
    /// but consider this case
    /// 
    /// text box is like this
    /// 
    /// |abcde |
    /// |abcdeI|
    /// 
    /// if user moves the cursor up
    /// 
    /// |abcdeI|
    /// |abcde |
    /// 
    /// then by the above rule cursor will move to the beginning of the next line
    /// 
    /// | abcde|
    /// |Iabcde|
    /// 
    /// we don't want that to happen so below value is there to
    /// handle these specific cases
    /// </summary>
    bool place_after_last_char_before_wrapping;
} Cursor;

typedef struct Selection {
    size_t start_line_number;
    size_t end_line_number;

    size_t start_char;
    size_t end_char;
} Selection;

typedef struct TextBox{
    int w;
    int h;

    TextLine* first_line;

    Cursor cursor;

    TTF_Font* font;
    SDL_Texture* texture;

    SDL_Renderer* renderer;
    int offset_y;

    Selection selection;
    bool is_selecting;

    SDL_Color bg_color;
    SDL_Color text_color;
    SDL_Color selection_bg;
    SDL_Color selection_fg;
    SDL_Color cursor_color;

    bool need_to_render;
}TextBox;



TextBox* text_box_create(
    const char* text, 
    size_t w, size_t h, 
    TTF_Font* font, 
    SDL_Color bg_color, SDL_Color text_color, SDL_Color selection_bg, SDL_Color selection_fg, SDL_Color cursor_color,
    SDL_Renderer* renderer);

void text_box_destroy(TextBox* box);

void text_box_handle_event(TextBox* box, SDL_Event* event);

Cursor text_box_type(TextBox* box, Cursor cursor, char* c);

void text_box_render(TextBox* box);

Cursor text_box_move_cursor_left(TextBox* box, Cursor cursor);
Cursor text_box_move_cursor_right(TextBox* box, Cursor cursor);
Cursor text_box_move_cursor_up(TextBox* box, Cursor cursor);
Cursor text_box_move_cursor_down(TextBox* box, Cursor cursor);

Cursor text_box_delete_a_character(TextBox* box, Cursor cursor);
Cursor text_box_delete_range(TextBox* box, Selection selection);

void text_box_start_selection(TextBox* box);
void text_box_end_selection(TextBox* box);

#endif