#include "TextBox.h"

#include <SDL2/SDL.h>
#include <stdio.h>

TextBox* text_box_create(const char* text, size_t w, size_t h, TTF_Font* font, SDL_Renderer* renderer) {
	TextBox* box = malloc(sizeof(TextBox));
	box->w = w;
	box->h = h;

	box->str = utf_create(text);
	box->font = font;
	
	box->str_start = 0;
	box->str_end = text_box_calculate_str_end(box);

	box->cursor_pos = 0;

	box->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
	if (box->texture == NULL) {
		fprintf(stderr, "%s:%d:ERROR : Failed to create a texture for text bo\n", __FILE__, __LINE__);
		return NULL;
	}

	SDL_SetTextureBlendMode(box->texture, SDL_BLENDMODE_BLEND);

	return box;
}

void text_box_destroy(TextBox* box)
{
	if (!box) {
		return;
	}

	utf_destroy(box->str);
	if(box->texture)
		SDL_DestroyTexture(box->texture);
	free(box);
}

bool sv_fits(UTFStringView sv, TTF_Font* font, int w, size_t *text_count, int* text_width) {
	char tmp = sv.data[sv.data_size];
	sv.data[sv.data_size] = 0;

	int measured_count = 0;
	int measured_width = 0;

	//TODO : it doesn't acount for line wrapping rule
	//we just believe what TTF_MeasureText saids
	if (TTF_MeasureUTF8(font, sv.data, w, &measured_width, &measured_count) != 0) {
		fprintf(stderr, "%s:%d:ERROR : failed to measure a string!!! : %s\n", __FILE__, __LINE__, SDL_GetError());
		return 0;
	}

	sv.data[sv.data_size] = tmp;

	if(text_count)
		*text_count = measured_count;
	if (text_width)
		*text_width = measured_width;

	return measured_count == utf_sv_count(sv);
}

size_t text_box_calculate_str_end(TextBox* box) {
	size_t box_str_count = box->str->count;
	UTFStringView sv = utf_sv_sub_str(
		*(box->str), 
		box->str_start, 
		box_str_count);

	int font_heigt = TTF_FontHeight(box->font);
	int height_remainder = box->h;

	size_t str_end = box->str_start;

	while (height_remainder > 0 && str_end < box_str_count) {
		//TODO : account for windows new line \r\n
		int new_line_at = utf_sv_find(sv, utf_sv_from_cstr(u8"\n"));

		if (new_line_at < 0) {
			new_line_at = box_str_count;
		}
		else {
			//include new line
			new_line_at += 1;
		}

		UTFStringView line = utf_sv_sub_sv(sv, 0, new_line_at);

		size_t cut_off = 0;
		if (sv_fits(line, box->font, box->w, &cut_off, NULL)) {
			str_end += line.count;
			sv = utf_sv_trim_left(sv, new_line_at);
		}
		else {
			str_end += cut_off;
			sv = utf_sv_trim_left(sv, cut_off);
		}
		
		height_remainder -= font_heigt;
	}

	return str_end;
}

size_t text_box_calculate_str_start(TextBox* box) {
	UTFStringView sv = utf_sv_sub_str(
		*(box->str),
		0,
		box->str_end);

	int font_heigt = TTF_FontHeight(box->font);
	int height_remainder = box->h;

	int str_start = box->str_end;

	while (height_remainder > 0 && str_start >= 0) {
		//TODO : account for windows new line \r\n
		int new_line_at = utf_sv_find_last(sv, utf_sv_from_cstr(u8"\n"));
		
		if (new_line_at < 0) {
			new_line_at = 0;
		}
		else {
			//don't include new line
			new_line_at += 1;
		}

		UTFStringView line = utf_sv_sub_sv(sv, new_line_at, str_start);

		size_t cut_off = 0;
		if (sv_fits(line, box->font, box->w, &cut_off, NULL)) {
			str_start -= line.count;
			sv = utf_sv_trim_right(sv, line.count);
		}
		else {
			size_t line_cont = line.count;
			line_cont -= cut_off;
			str_start -= line_cont;
			sv = utf_sv_trim_right(sv, line_cont);
		}

		height_remainder -= font_heigt;
	}
	
	if (str_start < 0) { str_start = 0; }

	return str_start;
}

size_t text_box_caclualte_line_start(TextBox* box, size_t from) {
	UTFStringView sv = utf_sv_sub_str(*(box->str), 0, from);

	if (utf_sv_ends_with(sv, utf_sv_from_cstr("\n"))) {
		from -= 1;
		sv = utf_sv_trim_right(sv,1);
	}

	int new_line_index = utf_sv_find_last(sv, utf_sv_from_cstr("\n"));

	if (new_line_index < 0) {
		new_line_index = 0;
	}

	UTFStringView line = utf_sv_sub_sv(sv, new_line_index, sv.count);

	size_t fit = 0;
	while (!sv_fits(line, box->font, box->w, &fit, NULL)) {
		line = utf_sv_trim_left(line, fit);
	}

	return from - line.count;
}

size_t text_box_caclualte_line_end(TextBox* box, size_t from) {
	UTFStringView sv = utf_sv_sub_str(*(box->str), from, box->str->count);

	int new_line_index = utf_sv_find(sv, utf_sv_from_cstr("\n"));

	if (new_line_index < 0) {
		new_line_index = sv.count;
	}

	UTFStringView line = utf_sv_sub_sv(sv, from, new_line_index);

	size_t fit = 0;
	sv_fits(line, box->font, box->w, &fit, NULL);
	return from + fit;
}

void calculate_cursor_pos(TextBox* box, int* cursor_x, int* cursor_y) 
{
	if (box->cursor_pos == 0) {
		*cursor_x = 0;
		*cursor_y = 0;
		return;
	}
	/////////////////////////////
	// loop through string
	/////////////////////////////
	UTFStringView sv = utf_sv_sub_str(*(box->str), box->str_start, box->cursor_pos);

	size_t sv_count = sv.count;

	int font_height = TTF_FontHeight(box->font);
	int y_offset = 0;
	size_t char_offset = 0;

	while (y_offset < box->h && char_offset < sv_count) {
		//TODO : account for windows new line \r\n
		int new_line_at = utf_sv_find(sv, utf_sv_from_cstr(u8"\n"));

		if (new_line_at < 0) {
			new_line_at = sv_count;
		}
		else {
			//include new line
			new_line_at += 1;
		}

		UTFStringView line = utf_sv_sub_sv(sv, 0, new_line_at);
		size_t line_count = line.count;

		size_t cut_off = 0;
		SDL_Surface* surface = NULL;

		if (sv_fits(line, box->font, box->w, &cut_off, cursor_x)) {
			char_offset += line_count;
			sv = utf_sv_trim_left(sv, new_line_at);
		}
		else {
			char_offset += cut_off;
			sv = utf_sv_trim_left(sv, cut_off);
			UTFStringView cutted = utf_sv_sub_sv(line, 0, cut_off);
		}

		y_offset += font_height;
	}

	y_offset -= font_height;

	*cursor_y = y_offset;
}



void text_box_render(TextBox* box, SDL_Renderer* renderer) {
	/////////////////////////////
	// Set Render Target
	/////////////////////////////
	SDL_Texture* prev_render_target = SDL_GetRenderTarget(renderer);

	SDL_SetRenderTarget(renderer, box->texture);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	/////////////////////////////
	// loop through string
	/////////////////////////////

	UTFStringView sv = utf_sv_sub_str(*(box->str), box->str_start, box->str_end);

	SDL_Color color = {.r = 255, .g = 255, .b = 255, .a = 255};

	size_t sv_count = sv.count;

	int font_height = TTF_FontHeight(box->font);
	int y_offset = 0;
	size_t char_offset = 0;

	while (y_offset < box->h && char_offset < sv_count) {
		//TODO : account for windows new line \r\n
		int new_line_at = utf_sv_find(sv, utf_sv_from_cstr(u8"\n"));
		
		if (new_line_at < 0) {
			new_line_at = sv_count;
		}
		else {
			//include new line
			new_line_at += 1;
		}

		UTFStringView line = utf_sv_sub_sv(sv, 0, new_line_at);
		size_t line_count = line.count;

		size_t cut_off = 0;
		SDL_Surface* surface = NULL;

		if (sv_fits(line, box->font, box->w, &cut_off, NULL)) {
			char_offset += line_count;
			sv = utf_sv_trim_left(sv, new_line_at);

			//temporarily make line null terminated
			char tmp = line.data[line.data_size];
			line.data[line.data_size] = 0;
			surface = TTF_RenderUTF8_Blended(box->font, line.data, color);
			if (!surface) {
				fprintf(stderr, "%s:%d:ERROR : failed to create surface %s\n", __FILE__, __LINE__, SDL_GetError());
				return;
			}
			line.data[line.data_size] = tmp;
		}
		else {
			char_offset += cut_off;
			sv= utf_sv_trim_left(sv, cut_off);
			UTFStringView cutted = utf_sv_sub_sv(line, 0, cut_off);

			//temporarily make line null terminated
			char tmp = cutted.data[cutted.data_size];
			cutted.data[cutted.data_size] = 0;
			surface = TTF_RenderUTF8_Blended(box->font, cutted.data, color);
			if (!surface) {
				fprintf(stderr, "%s:%d:ERROR : failed to create surface %s\n", __FILE__, __LINE__, SDL_GetError());
				return;
			}
			cutted.data[cutted.data_size] = tmp;
		}

		//render texts

		SDL_Rect rect = { .x = 0, .y = y_offset, .w = surface->w, .h= surface->h};
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_RenderCopy(renderer, texture, NULL, &rect);

		SDL_FreeSurface(surface);
		SDL_DestroyTexture(texture);

		y_offset += font_height;
	}

	/////////////////////////////
	// draw cursor
	/////////////////////////////

	int cursor_x = 0;
	int cursor_y = 0;
	calculate_cursor_pos(box, &cursor_x, &cursor_y);
	SDL_Rect cursor_rect = { .x = cursor_x, .y = cursor_y+2, .w = 2, .h = font_height-2 };
	printf("cursor x : %d  ", cursor_x);
	printf("cursor y : %d\n", cursor_y);

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &cursor_rect);

	//restore render target
	SDL_SetRenderTarget(renderer, prev_render_target);
}

void text_box_move_cursor_left(TextBox* box)
{
	if (box->cursor_pos <= 0) {
		return;
	}

	box->cursor_pos -= 1;
	
	if (box->cursor_pos < box->str_start) {
		UTFStringView sv = utf_sv_from_str(*(box->str));
		box->str_start = text_box_caclualte_line_start(box, box->cursor_pos);
		box->str_end = text_box_calculate_str_end(box);
	}
}

void text_box_move_cursor_right(TextBox* box)
{
	size_t box_text_count = box->str->count;

	if (box->cursor_pos >= box_text_count) {
		return;
	}

	box->cursor_pos += 1;

	if (box->cursor_pos > box->str_end) {
		UTFStringView sv = utf_sv_from_str(*(box->str));
		box->str_end = text_box_caclualte_line_end(box, box->cursor_pos);
		box->str_start = text_box_calculate_str_start(box);
	}
}