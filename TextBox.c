#include "TextBox.h"

#include <stdio.h>

TextBox* text_box_create(const char* text, size_t w, size_t h, TTF_Font* font) {
	TextBox* box = malloc(sizeof(TextBox));
	box->w = w;
	box->h = h;

	box->str = utf_create(text);
	box->font = font;
	
	//TODO : calculate string start and string end properly with
	//box size and
	box->str_start = 0;
	box->str_end = 0;

	box->cursor_pos = 0;
}

bool sv_fits(UTFStringView sv, TTF_Font* font, int w, size_t *cuts_off) {
	char tmp = sv.data[sv.data_size];
	sv.data[sv.data_size] = 0;

	int measured_count = 0;

	//TODO : it doesn't acount for line wrapping rule
	//we just believe what TTF_MeasureText saids
	if (TTF_MeasureUTF8(font, sv.data, w, NULL, &measured_count) != 0) {
		fprintf(stderr, "%s:%d:ERROR : failed to measure a string!!! : %s\n", __FILE__, __LINE__, SDL_GetError());
		return 0;
	}

	sv.data[sv.data_size] = tmp;

	*cuts_off = measured_count;

	return measured_count < sv.data_size;
}

size_t text_box_calculate_str_end(TextBox* box) {
	UTFStringView sv = utf_sv_sub_str(
		*(box->str), 
		utf_to_byte_index(box->str->data, box->str_start), 
		box->str->data_size);

	int font_heigt = TTF_FontHeight(box->font);
	int height_remainder = box->h;

	size_t str_end = box->str_start;

	while (height_remainder > 0) {
		//TODO : account for windows new line \r\n
		int new_line_at = utf_sv_find(sv, utf_sv_from_cstr(u8"\n"));
		//include new line
		new_line_at += 1;

		UTFStringView line = utf_sv_sub_sv(sv, new_line_at,sv.data_size);

		size_t cut_off = 0;
		if (sv_fits(line, box->font, box->w, &cut_off)) {
			str_end += utf_sv_count(line);
			utf_sv_trim_left(sv, new_line_at);
		}
		else {
			cut_off = utf_sv_to_byte_index(sv, cut_off);
			str_end += utf_sv_count_left_from(sv, cut_off);
			utf_sv_trim_left(sv, cut_off);
		}
		
		height_remainder -= font_heigt;
	}
}