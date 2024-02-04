#ifndef TextLine_HEADER_GUARD
#define TextLine_HEADER_GUARD

#include "UTFString.h"
#include <stdbool.h>

typedef struct TextLine{
    struct TextLine* prev;
    struct TextLine* next;

    UTFString *str;

    int size_x;
    int size_y;

    //if line is wrapped due to text count
    //then it might be displayed in multiple lines
    //these values are here to handle that situation
    size_t wrapped_line_count;
    //TODO perhaps make vector for ints
    int wrapped_line_sizes[128];

    /////////////////////////////
    // !!!!!!!IMPORTANT!!!!!!!!!
    // These values does not indicate whether or not str it self ends with crlf or lf
    // TextLine str should not have crlf of lf at the end in the first place
    /////////////////////////////
    bool ends_with_crlf;
    bool ends_with_lf;

    size_t line_number;
} TextLine;

TextLine* text_line_create(UTFString* str, size_t line_number, bool ends_with_lf, bool ends_with_crlf);
void text_line_destroy(TextLine* line);

TextLine* text_line_first(TextLine* line);
TextLine* text_line_last(TextLine* line);

void text_line_set_number_left(TextLine* from, size_t line_num);
void text_line_set_number_right(TextLine* from, size_t line_num);

void text_line_push_back(TextLine* line, TextLine* to_push);
void text_line_push_front(TextLine* line, TextLine* to_push);

void text_line_insert_left(TextLine* line, TextLine* to_insert);
void text_line_insert_right(TextLine* line, TextLine* to_insert);

TextLine* create_lines_from_cstr(const char *str);
TextLine* create_lines_from_str(UTFString* str);
TextLine* create_lines_from_sv(UTFStringView sv);

void text_line_test();

#endif