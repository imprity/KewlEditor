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

    size_t line_number;
} TextLine;

TextLine* text_line_create(UTFString *str, size_t line_number);
void text_line_destroy(TextLine* line);

void text_line_set_number_left(TextLine* from, size_t line_num);
void text_line_set_number_right(TextLine* from, size_t line_num);

void text_line_push_back(TextLine* line, TextLine* to_push);
void text_line_push_front(TextLine* line, TextLine* to_push);

void text_line_insert_left(TextLine* line, TextLine* to_insert);
void text_line_insert_right(TextLine* line, TextLine* to_insert);

TextLine* create_lines_from_str(char *str);

void text_line_test();

#endif