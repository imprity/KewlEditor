#include "UTFString.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>


//https://stackoverflow.com/questions/32936646/getting-the-string-length-on-utf-8-in-c#:~:text=The%20number%20of%20code%20points,and%20stopping%20at%20'%5C0'%20.
size_t utf8_get_length(const char* str) {
    size_t count = 0;
    while (*str) {
        count += (*str++ & 0xC0) != 0x80;
    }
    return count;
}

size_t utf_to_byte_index(const char* str, size_t index) {
    //TODO : there probably was a better way to calculate this
    //without using this many ifs...
    size_t byte_offset = 0;
    const uint8_t b1 = 0b10000000;
    const uint8_t b2 = 0b11100000;
    const uint8_t b3 = 0b11110000;
    const uint8_t b4 = 0b11111000;

    while (index > 0) {
        uint8_t byte = *(str + byte_offset);
        index--;
        if ((byte & b1) == 0b00000000) { byte_offset += 1; }
        else if ((byte & b4) == 0b11110000) { byte_offset += 4; }
        else if ((byte & b3) == 0b11100000) { byte_offset += 3; }
        else if ((byte & b2) == 0b11000000) { byte_offset += 2; }
        else { assert(0); }
    }
    return byte_offset;
}

size_t utf_sv_to_byte_index(UTFStringView sv, size_t index) {
    //TODO : this is fucking terrible
    char tmp = sv.data[sv.data_size];
    sv.data[sv.data_size] = 0;
    size_t byte_offset = utf_to_byte_index(sv.data, index);
    sv.data[sv.data_size] = tmp;
    return byte_offset;
}

////////////////////////////
// String Functions
////////////////////////////

#define UTF_STR_DEFAULT_ALLOC 128

size_t calculate_size(size_t needed_size) {
    size_t current_size = UTF_STR_DEFAULT_ALLOC;
    //TODO : there probably was a better way to calculate this number
    //without using while loop.....
    while (current_size < needed_size) {
        current_size *= 2;
    }
    return current_size;
}

void utf_grow(UTFString* str, size_t needed_size) {
    size_t calculated_needed = calculate_size(needed_size);
    if (str->raw_size < calculated_needed) {
        uint8_t* new_block = realloc(str->data, calculated_needed);
        if (new_block) {
            str->data = new_block;
            str->raw_size = calculated_needed;
        }
        else {
            fprintf(stderr, "%s:%d:ERROR : failed to grow a string!!!\n", __FILE__, __LINE__);
        }
    }
}

UTFString* utf_create(const char* str) {
    UTFString* to_return = malloc(sizeof(UTFString));

    //copy str
    if (str) {
        size_t str_len = strlen(str);
        size_t null_included = str_len + 1;

        to_return->raw_size = UTF_STR_DEFAULT_ALLOC;

        to_return->raw_size = calculate_size(null_included);

        to_return->data = malloc(to_return->raw_size);
        to_return->data_size = str_len;
        memcpy(to_return->data, str, null_included);
    }
    else {
        to_return->raw_size = UTF_STR_DEFAULT_ALLOC;
        to_return->data = malloc(to_return->raw_size);
        to_return->data[0] = 0;
        to_return->data_size = 0;
    }

    return to_return;
}

void utf_destroy(UTFString* str) {
    if (!str) { return; }
    if (str->data) { free(str->data); }
    free(str);
}

size_t utf_count(UTFString str)
{
    return utf_sv_count(utf_sv_from_str(str));
}

size_t utf_count_left_from(UTFString str, size_t from)
{
    return utf_sv_count_left_from(utf_sv_from_str(str), from);
}

size_t utf_count_right_from(UTFString str, size_t from)
{
    return utf_sv_count_right_from(utf_sv_from_str(str), from);
}

size_t utf_next(UTFString str, size_t pos)
{
    return utf_sv_next(utf_sv_from_str(str), pos);
}

size_t utf_prev(UTFString str, size_t pos)
{
    return utf_sv_prev(utf_sv_from_str(str), pos);
}

void utf_append(UTFString* str, const char* to_append) {
    size_t str_len = strlen(to_append);
    size_t null_included = str_len + 1;

    utf_grow(str, null_included + str->data_size);

    memcpy(str->data + str->data_size, to_append, null_included);
    str->data_size += str_len;
}

void utf_erase_right(UTFString* str, size_t how_many) {
    if (how_many >= utf8_get_length(str->data)) {
        str->data_size = 0;
        str->data[str->data_size] = 0;
        return;
    }

    str->data_size -= how_many;
    str->data[str->data_size] = 0;
}

void utf_erase_left(UTFString* str, size_t how_many) {
    if (how_many >= utf8_get_length(str->data)) {
        str->data_size = 0;
        str->data[str->data_size] = 0;
        return;
    }

    for (size_t i = how_many; i < str->data_size; i++) {
        str->data[i - how_many] = str->data[i];
    }
    str->data_size -= how_many;
    str->data[str->data_size] = 0;
}

void utf_insert(UTFString* str, size_t at, const char* to_insert) {
    if (at > str->data_size) {
        fprintf(stderr, "%s:%d:ERROR : at(%zu) is bigger than string size(%zu)\n", __FILE__, __LINE__, at, str->data_size);
        return;
    }

    size_t str_len = strlen(to_insert);
    size_t null_included = str_len + 1;

    utf_grow(str, null_included + str->data_size);

    //last+1 > at instead of last >= at because it wraps aroud when at is 0
    for (size_t last = str->data_size - 1; last+1 > at; last--) {
        str->data[last + str_len] = str->data[last];
    }

    memcpy(str->data + at, to_insert, str_len);
    str->data_size += str_len;
    str->data[str->data_size] = 0;
}

void utf_erase_range(UTFString* str, size_t from, size_t to){
    if (to <= from) {
        fprintf(stderr, "%s:%d:ERROR : from(%zu) is bigger than to(%zu)\n", __FILE__, __LINE__, from, to);
        return;
    }

    if (!(str->data_size >= to && str->data_size >= from)) {
        fprintf(stderr, "%s:%d:ERROR : from(%zu) or to(%zu) is bigger than char length(%zu)\n", __FILE__, __LINE__, from, to, str->data_size);
        return;
    }

    size_t delete_length = to - from;
    size_t distance = to - from;

    for (size_t i = from; i < str->data_size; i++) {
        str->data[i - distance] = str->data[i];
    }

    str->data_size -= distance;
    str->data[str->data_size] = 0;
}

////////////////////////////
// Strinv View Functions
////////////////////////////

UTFStringView utf_sv_from_cstr(const char* str)
{
    UTFStringView sv = { .data = str, sv.data_size = strlen(str) };
    return sv;
}

UTFStringView utf_sv_from_str(UTFString str) {
    UTFStringView sv = {.data = str.data, .data_size = str.data_size};
    return sv;
}

UTFStringView utf_sv_sub_str(UTFString str, size_t from, size_t to) {
    if (to <= from) {
        fprintf(stderr, "%s:%d:WARN : from(%zu) is bigger than to(%zu)\n", __FILE__, __LINE__, from, to);
        size_t tmp = to;
        to = from;
        from = to;
    }

    if (str.data_size < to) {
        fprintf(stderr, "%s:%d:WARN : to(%zu) is bigger than char length(%zu)\n", __FILE__, __LINE__, to, str.data_size);
        to = str.data_size;
    }

    if (str.data_size < from) {
        fprintf(stderr, "%s:%d:WARN : from(%zu) is bigger than char length(%zu)\n", __FILE__, __LINE__, from, str.data_size);
        from = str.data_size;
    }

    UTFStringView sv;
    sv.data_size = to - from;
    sv.data = str.data + from;

    return sv;
}

UTFStringView utf_sv_sub_sv(UTFStringView str, size_t from, size_t to) 
{
    if (to <= from) {
        fprintf(stderr, "%s:%d:WARN : from(%zu) is bigger than to(%zu)\n", __FILE__, __LINE__, from, to);
        size_t tmp = to;
        to = from;
        from = to;
    }

    if (str.data_size < to) {
        fprintf(stderr, "%s:%d:WARN : to(%zu) is bigger than char length(%zu)\n", __FILE__, __LINE__, to, str.data_size);
        to = str.data_size;
    }

    if (str.data_size < from) {
        fprintf(stderr, "%s:%d:WARN : from(%zu) is bigger than char length(%zu)\n", __FILE__, __LINE__, from, str.data_size);
        from = str.data_size;
    }

    UTFStringView sv;
    sv.data_size = to - from;
    sv.data = str.data + from;
    return sv;
}

size_t utf_sv_count(UTFStringView sv)
{
    size_t count = 0;
    while (sv.data_size--) {
        count += (*sv.data++ & 0xC0) != 0x80;
    }
    return count;
}

size_t utf_sv_count_left_from(UTFStringView sv, size_t from)
{
    size_t count = 0;
    char* ptr = sv.data + from;
    while (from--) {
        count += (*ptr-- & 0xC0) != 0x80;
    }
    return count;
}

size_t utf_sv_count_right_from(UTFStringView sv, size_t from)
{
    size_t count = 0;
    char* ptr = sv.data + from;
    while (from++ < sv.data_size) {
        count += (*ptr++ & 0xC0) != 0x80;
    }
    return count;
}

size_t utf_sv_next(UTFStringView sv, size_t pos)
{
    if (pos >= sv.data_size) {
        return sv.data_size;
    }

    if ((sv.data[pos] & 0b11000000) != 0b10000000) {
        pos++;
    }
    while (pos < sv.data_size && (sv.data[pos] & 0b11000000) == 0b10000000) {
        pos++;
    }
    return pos;
}

size_t utf_sv_prev(UTFStringView sv, size_t pos)
{
    if (pos == 0) {
        return 0;
    }
    if (pos > sv.data_size) {
        pos = sv.data_size;
    }

    if ((sv.data[pos] & 0b11000000) != 0b10000000) {
        pos--;
    }
    while (pos > 0 && (sv.data[pos] & 0b11000000) == 0b10000000) {
        pos--;
    }
    return pos;
}

UTFStringView utf_sv_trim_left(UTFStringView sv, size_t how_many)
{
    if (how_many >= sv.data_size) {
        how_many = sv.data_size;
    }
    sv.data += how_many;
    sv.data_size -= how_many;
    return sv;
}

UTFStringView utf_sv_trim_right(UTFStringView sv, size_t how_many)
{
    if (how_many >= sv.data_size) {
        how_many = sv.data_size;
    }
    sv.data_size -= how_many;
    return sv;
}

bool utf_sv_cmp(UTFStringView str1, UTFStringView str2) {
    if (str1.data_size != str2.data_size) {
        return false;
    }

    for (size_t i = 0; i < str1.data_size; i++) {
        if (str1.data[i] != str2.data[i]) {
            return false;
        }
    }
    return true;
}

int utf_sv_find(UTFStringView str, UTFStringView to_find)
{
    if (str.data_size < to_find.data_size) {
        return -1;
    }

    int char_count = 0;
    size_t byte_offset = 0;

    while (byte_offset + to_find.data_size <= str.data_size) {
        UTFStringView sub = utf_sv_sub_sv(str, byte_offset, byte_offset + to_find.data_size);
        if (utf_sv_cmp(sub, to_find)) {
            return byte_offset;
        }
        byte_offset++;
    }
    return -1;
}

int utf_sv_find_last(UTFStringView str, UTFStringView to_find) {
    if (str.data_size < to_find.data_size) {
        return -1;
    }

    int char_count = 0;
    int byte_offset = str.data_size - to_find.data_size;

    while (byte_offset >= 0) {
        UTFStringView sub = utf_sv_sub_sv(str, byte_offset, byte_offset + to_find.data_size);
        if (utf_sv_cmp(sub, to_find)) {
            return byte_offset;
        }
        byte_offset--;
    }
    return -1;
}

void utf_sv_fprint(UTFStringView sv, FILE* file)
{
    for (int i = 0; i < sv.data_size; i++) {
        fputc(sv.data[i], file);
    }
}

void utf_sv_fprintln(UTFStringView sv, FILE* file)
{
    for (int i = 0; i < sv.data_size; i++) {
        fputc(sv.data[i], file);
    }
    fputc('\n', file);
}

bool utf_test()
{
    FILE* file = fopen("uft test cases.txt", "wb");

    UTFString* str = utf_create(u8"안녕 세상아!! hello world!!");
    UTFStringView sv = utf_sv_from_str(*str);

    utf_sv_fprintln(sv, file);

    {
        size_t p = 0;
        while (p != sv.data_size) {
            size_t n = utf_sv_next(sv, p);
            UTFStringView sub = utf_sv_sub_sv(sv, p, n);
            utf_sv_fprintln(sub, file);
            p = n;
        }
    }

    {
        size_t loc = utf_sv_find(sv, utf_sv_from_cstr(u8"h"));
        UTFStringView sub = utf_sv_sub_sv(sv, loc, sv.data_size);
        utf_sv_fprintln(sub, file);
    }

    {
        size_t loc = utf_sv_find_last(sv, utf_sv_from_cstr(u8" hello world"));
        UTFStringView sub = utf_sv_sub_sv(sv, 0, loc);
        utf_sv_fprintln(sub, file);
    }

    {
        assert(utf_sv_find(sv, utf_sv_from_cstr(u8"존재하지 않는 것")) < 0);
        assert(utf_sv_find_last(sv, utf_sv_from_cstr(u8"존재하지 않는 것")) < 0);

        UTFStringView seven = utf_sv_from_cstr(u8"글씨 일곱개.");
        assert(utf_sv_count(seven) == 7);
        size_t middle = utf_sv_find(seven, utf_sv_from_cstr(u8"일"));
        assert(utf_sv_count_left_from(seven, middle) == 3);
        assert(utf_sv_count_right_from(seven, middle) == 4);
        middle += 1;
        assert(utf_sv_count_left_from(seven, middle) == 3);
        assert(utf_sv_count_right_from(seven, middle) == 3);
    }

    {
        UTFString* pony = utf_create(u8"마이 리틀 포니!! my little pony!!! 아아아아");
        UTFStringView pony_sv = utf_sv_from_str(*pony);

        size_t pos = utf_sv_find(pony_sv, utf_sv_from_cstr(u8"리틀"));
        utf_erase_left(pony,pos);
        utf_sv_fprintln(utf_sv_from_str(*pony), file);

        utf_erase_right(pony, 10000);
        utf_sv_fprintln(utf_sv_from_str(*pony), file);
        utf_destroy(pony);
    }

    {
        UTFString* hello = utf_create(u8"안녕하세요!!");
        UTFStringView hello_sv = utf_sv_from_str(*hello);

        utf_insert(hello, 0, u8"Hello. ");
        utf_insert(hello, hello->data_size, u8" 만나서 반갑습니다");
        utf_sv_fprintln(utf_sv_from_str(*hello), file);
    }

    utf_destroy(str);

    fclose(file);

    return true;
}