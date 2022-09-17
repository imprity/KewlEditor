#ifndef TextBox_HEADER_GUARD
#define TextBox_HEADER_GUARD

#include "UTFString.h"
#include "TextLine.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL.h>
#include "OS.h"


typedef struct TextCursor {
    size_t line_number;
    size_t char_offset;

    // This is for the special case when cursor has to be
    // at the end of the line wrapped character
    //
    // For most cases, if cursor is at the end of the wrapped line
    // cursor should move to the start of the next line
    //
    // but consider this case
    //
    // text box is like this
    //
    // |abcde |
    // |abcdeI|
    //
    // if user moves the cursor up
    //
    // |abcdeI|
    // |abcde |
    //
    // then by the above rule cursor will move to the beginning of the next line
    //
    // | abcde|
    // |Iabcde|
    //
    // we don't want that to happen so below value is there to
    // handle these specific cases
    bool place_after_last_char_before_wrapping;
} TextCursor ;

typedef struct Selection {
    size_t start_line_number;
    size_t end_line_number;

    size_t start_char;
    size_t end_char;
} Selection;

typedef void (*PreeditPosSetter)(int x, int y);

typedef struct TextBox{
    int w;
    int h;

    TextLine* first_line;

    TextCursor cursor;

    TTF_Font* font;
    SDL_Surface* render_surface;

    int offset_y;

    Selection selection;
    bool is_selecting;

    SDL_Color bg_color;
    SDL_Color text_color;
    SDL_Color selection_bg;
    SDL_Color selection_fg;
    SDL_Color cursor_color;

    bool need_to_render;

    UTFString* composite_str;

    PreeditPosSetter preedit_pos_setter;
}TextBox;

TextBox* text_box_create(
    const char* text,
    size_t w, size_t h,
    TTF_Font* font,
    SDL_Color bg_color, SDL_Color text_color, SDL_Color selection_bg, SDL_Color selection_fg, SDL_Color cursor_color,
    PreeditPosSetter pos_setter
    );

void text_box_destroy(TextBox* box);

void text_box_handle_event(TextBox* box, OS_Event* event);

TextCursor text_box_type(TextBox* box, TextCursor cursor, UTFStringView sv);

void text_box_render(TextBox* box);

TextCursor text_box_move_cursor_left(TextBox* box, TextCursor cursor);
TextCursor text_box_move_cursor_right(TextBox* box, TextCursor cursor);
TextCursor text_box_move_cursor_up(TextBox* box, TextCursor cursor);
TextCursor text_box_move_cursor_down(TextBox* box, TextCursor cursor);

TextCursor text_box_move_cursor_left_word(TextBox* box, TextCursor cursor);
TextCursor text_box_move_cursor_right_word(TextBox* box, TextCursor cursor);

TextCursor text_box_delete_a_character(TextBox* box, TextCursor cursor);
TextCursor text_box_delete_range(TextBox* box, Selection selection);

UTFString* text_box_get_selection_str(TextBox* box, Selection selection);

void text_box_resize(TextBox* box, int w, int h);

#endif
