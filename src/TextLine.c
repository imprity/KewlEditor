#include "TextLine.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

TextLine* text_line_create(UTFString *str, size_t line_number, bool ends_with_lf, bool ends_with_crlf)
{
    TextLine *line = malloc(sizeof(TextLine));
    line->prev = NULL;
    line->next = NULL;

    line->ends_with_crlf = ends_with_crlf;
    line->ends_with_lf = ends_with_lf;

    if (str == NULL) {
        line->str = utf_from_cstr(u8"");
    }
    else{
        line->str = str;
    }

    line->wrapped_line_count = 1;
    line->wrapped_line_sizes[0] = line->str->count;

    line->size_x = 0;
    line->size_y = 0;

    line->line_number = line_number;

    return line;
}

void text_line_destroy(TextLine* line)
{
    if(line->str){
        utf_destroy(line->str);
    }

    free(line);
}

TextLine* text_line_first(TextLine* line)
{
    TextLine* current_line = line;
    while (true) {
        if (!current_line->prev) {
            return current_line;
        }
        current_line = current_line->prev;
    }
}

TextLine* text_line_last(TextLine* line)
{
    TextLine* current_line = line;
    while (true) {
        if (!current_line->next) {
            return current_line;
        }
        current_line = current_line->next;
    }
}

void text_line_set_number_left(TextLine* from, size_t line_num)
{
    TextLine *current_line = from;
    for(size_t i=line_num; i>0; i--){
        current_line->line_number = line_num;
        current_line = current_line->prev;
        if(!current_line){
            break;
        }
    }
}

void text_line_set_number_right(TextLine* from, size_t line_num)
{
    for(TextLine* line = from; line != NULL; line = line->next){
        line->line_number = line_num;
        line_num++;
    }
}

void text_line_push_back(TextLine* line, TextLine* to_push)
{
    TextLine* line_last = text_line_last(line);
    TextLine* to_push_first = text_line_first(to_push);
    line_last->next = to_push_first;
    to_push_first->prev = line_last;
}

void text_line_push_front(TextLine* line, TextLine* to_push)
{
    TextLine* line_first = text_line_last(line);
    TextLine* to_push_last = text_line_first(to_push);
    line_first->prev = to_push_last;
    to_push_last->next = line_first;
}

void text_line_insert_left(TextLine* line, TextLine* to_insert)
{
    TextLine* prev_left = line->prev;

    TextLine* to_insert_first = text_line_first(to_insert);
    TextLine* to_insert_last = text_line_last(to_insert);

    line->prev = to_insert_last;
    to_insert_last->next = line;

    to_insert_first->prev = prev_left;
    if(prev_left){
        prev_left->next = to_insert_first;
    }
}

void text_line_insert_right(TextLine* line, TextLine* to_insert)
{
    TextLine* prev_right = line->next;

    TextLine* to_insert_first = text_line_first(to_insert);
    TextLine* to_insert_last = text_line_last(to_insert);

    line->next = to_insert_first;
    to_insert_first->prev = line;

    to_insert_last->next = prev_right;
    if(prev_right){
        prev_right->prev = to_insert_last;
    }
}

TextLine* create_lines_from_cstr(const char *str)
{
    if (str == NULL) {
        return text_line_create(utf_from_cstr(NULL), 0, false, false);
    }

    UTFStringView sv = utf_sv_from_cstr(str);
    return create_lines_from_sv(sv);
}

TextLine* create_lines_from_str(UTFString* str)
{
    UTFStringView sv = utf_sv_from_str(str);
    return create_lines_from_sv(sv);
}

TextLine* create_lines_from_sv(UTFStringView sv)
{
    if (sv.count == 0) {
        return text_line_create(utf_from_sv(sv), 0, false, false);
    }

    TextLine* first = NULL;
    TextLine* last = NULL;

    size_t line_number = 0;

    while (sv.count != 0) {
        int lf_at = utf_sv_find(sv, utf_sv_from_cstr(u8"\n"));
        int crlf_at = utf_sv_find(sv, utf_sv_from_cstr(u8"\r\n"));

        bool ends_with_crlf = false;
        bool ends_with_lf = false;

        bool found_new_line = lf_at >= 0 || crlf_at >= 0;

        size_t new_line_at = 0;

        if (!found_new_line) {
            new_line_at = sv.count;
        }
        else {
            if(lf_at < 0 || (crlf_at < lf_at && crlf_at >= 0)){
                new_line_at = crlf_at;
                ends_with_crlf = true;
            }
            else{
                new_line_at = lf_at;
                ends_with_lf = true;
            }
        }

        UTFStringView line = utf_sv_sub_sv(sv, 0, new_line_at);
        sv = utf_sv_trim_left(sv, new_line_at);

        if (found_new_line) {
            if (ends_with_crlf) {
                sv = utf_sv_trim_left(sv, 2);
            }
            else if(ends_with_lf) {
                sv = utf_sv_trim_left(sv, 1);
            }
        }

        if (first == NULL) {
            first = text_line_create(utf_from_sv(line), line_number++, ends_with_lf, ends_with_crlf);
            last = first;
        }
        else {
            TextLine* to_push = text_line_create(utf_from_sv(line), line_number++, ends_with_lf, ends_with_crlf);
            text_line_push_back(last, to_push);
            last = to_push;
        }
    }
    if (last->ends_with_crlf || last->ends_with_lf) {
        TextLine* end = text_line_create(utf_from_cstr(""), line_number++, false, false);
        text_line_push_back(last, end);
    }

    return first;
}

void text_line_test()
{
    {
        TextLine* first = create_lines_from_cstr(
            u8"line 1\n"
            u8"line 2\r\n"
            u8"line 3\n"
        );

        TextLine* tmp = first;

        assert(utf_sv_cmp(utf_sv_from_str(tmp->str), utf_sv_from_cstr(u8"line 1")));
        assert(tmp->ends_with_lf == true);
        assert(tmp->ends_with_crlf == false);
        assert(tmp->line_number == 0);
        tmp = tmp->next;

        assert(utf_sv_cmp(utf_sv_from_str(tmp->str), utf_sv_from_cstr(u8"line 2")));
        assert(tmp->ends_with_lf == false);
        assert(tmp->ends_with_crlf == true);
        assert(tmp->line_number == 1);
        tmp = tmp->next;

        assert(utf_sv_cmp(utf_sv_from_str(tmp->str), utf_sv_from_cstr(u8"line 3")));
        assert(tmp->ends_with_lf == true);
        assert(tmp->ends_with_crlf == false);
        assert(tmp->line_number == 2);
        tmp = tmp->next;

        assert(utf_sv_cmp(utf_sv_from_str(tmp->str), utf_sv_from_cstr(u8"")));
        assert(tmp->ends_with_lf == false);
        assert(tmp->ends_with_crlf == false);
        assert(tmp->line_number == 3);
        tmp = tmp->next;

        tmp = first;

        while (tmp != NULL) {
            TextLine* next = tmp->next;
            text_line_destroy(tmp);
            tmp = next;
        }
    }
    {
        TextLine* first = create_lines_from_cstr(
            u8"line 1\n"
            u8"line 2"
        );

        TextLine* tmp = first;

        assert(utf_sv_cmp(utf_sv_from_str(tmp->str), utf_sv_from_cstr(u8"line 1")));
        assert(tmp->ends_with_lf == true);
        assert(tmp->ends_with_crlf == false);
        assert(tmp->line_number == 0);
        tmp = tmp->next;

        assert(utf_sv_cmp(utf_sv_from_str(tmp->str), utf_sv_from_cstr(u8"line 2")));
        assert(tmp->ends_with_lf == false);
        assert(tmp->ends_with_crlf == false);
        assert(tmp->line_number == 1);
        tmp = tmp->next;

        tmp = first;

        while (tmp != NULL) {
            TextLine* next = tmp->next;
            text_line_destroy(tmp);
            tmp = next;
        }
    }
}
