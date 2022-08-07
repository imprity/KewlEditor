#ifndef UTFString_HEADER_GUARD
#define UTFString_HEADER_GUARD

#include <stdlib.h>
#include <stdbool.h>

typedef struct UTFString {
    char* data;
    size_t raw_size;
    size_t data_size; //does not include null terminated character
}UTFString;

size_t utf8_get_length(const char* str);
size_t utf_to_byte_index(const char* str, size_t index);

UTFString* utf_create(const char* str);
void utf_destroy(UTFString* str);

size_t utf_count(UTFString str);
size_t utf_count_left_from(UTFString str, size_t from);
size_t utf_count_right_from(UTFString str, size_t from);

size_t utf_next(UTFString str, size_t pos);
size_t utf_prev(UTFString str, size_t pos);

void utf_append(UTFString* str, const char* to_append);
void utf_insert(UTFString* str, size_t at, const char* to_append);

void utf_erase_range(UTFString* str, size_t from, size_t to);
void utf_erase_right(UTFString* str, size_t how_many);
void utf_erase_left(UTFString* str, size_t how_many);

typedef struct UTFStringView {
    char* data;
    size_t data_size; //does not include null terminated character
}UTFStringView;

size_t utf_sv_to_byte_index(UTFStringView sv, size_t index);

UTFStringView utf_sv_from_cstr(const char * str);
UTFStringView utf_sv_from_str(UTFString str);
UTFStringView utf_sv_sub_str(UTFString str, size_t from, size_t to);
UTFStringView utf_sv_sub_sv(UTFStringView str, size_t from, size_t to);

size_t utf_sv_count(UTFStringView sv);
size_t utf_sv_count_left_from(UTFStringView sv, size_t from);
size_t utf_sv_count_right_from(UTFStringView sv, size_t from);

size_t utf_sv_next(UTFStringView sv, size_t pos);
size_t utf_sv_prev(UTFStringView sv, size_t pos);

UTFStringView utf_sv_trim_left(UTFStringView sv, size_t how_many);
UTFStringView utf_sv_trim_right(UTFStringView sv, size_t how_many);

bool utf_sv_cmp(UTFStringView str1, UTFStringView str2);

int utf_sv_find(UTFStringView str, UTFStringView to_find);
int utf_sv_find_last(UTFStringView str, UTFStringView to_find);

bool utf_test();

#endif