#include "TextBox.h"

#include <SDL2/SDL.h>
#include <stdio.h>

bool sv_fits(UTFStringView sv, TTF_Font* font, int w, size_t* text_count, int* text_width) {
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

	if (text_count)
		*text_count = measured_count;
	if (text_width)
		*text_width = measured_width;

	return measured_count == utf_sv_count(sv);
}

int text_box_calculate_needed_height(TextBox* box) {
	size_t char_offset = 0;
	size_t y_offset = 0;
	UTFStringView sv = utf_sv_from_str(*(box->str));
	size_t font_height = TTF_FontHeight(box->font);

	while (char_offset < box->str->count) {
		size_t new_line_index = utf_sv_find_right_from(sv, utf_sv_from_cstr(u8"\n"), char_offset);
		bool found_new_line = new_line_index >= 0;

		if (!found_new_line) {
			new_line_index = box->str->count;
		}

		UTFStringView line = utf_sv_sub_sv(sv, char_offset, new_line_index);

		size_t fit = 0;

		while (!sv_fits(line, box->font, box->w, &fit, NULL)) {
			line = utf_sv_trim_left(line,fit);
			char_offset += fit;
			y_offset += font_height;
		}

		char_offset += line.count;

		if (found_new_line) {
			char_offset += 1;
		}

		y_offset += font_height;
	}

	return y_offset;
	//return 300;
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
	UTFStringView sv = utf_sv_sub_str(*(box->str), 0, box->cursor_pos);

	size_t sv_count = sv.count;

	int font_height = TTF_FontHeight(box->font);
	int y_offset = 0;
	size_t char_offset = 0;

	while (y_offset < box->paper_texture_h && char_offset < sv_count) {
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

	if (y_offset < 0) {
		y_offset = 0;
	}

	*cursor_y = y_offset;
}

void text_box_update_paper_texture(TextBox* box) {
	if (box->paper_texture) {
		SDL_DestroyTexture(box->paper_texture);
	}

	box->paper_texture_w = box->w;
	box->paper_texture_h = text_box_calculate_needed_height(box);

	box->paper_texture = SDL_CreateTexture(
		box->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, box->paper_texture_w,
		box->paper_texture_h
	);

	if (box->paper_texture == NULL) {
		fprintf(stderr, "%s:%d:ERROR : Failed to create a texture for paper texture\n", __FILE__, __LINE__);
		fprintf(stderr, "%s\n", SDL_GetError());
		return;
	}

	

	SDL_Texture* prev_render_target = SDL_GetRenderTarget(box->renderer);
	
	SDL_SetRenderTarget(box->renderer, box->paper_texture);
	SDL_SetRenderDrawBlendMode(box->renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(box->renderer, 0, 0, 0, 0);
	SDL_RenderClear(box->renderer);

	SDL_Color color = { .r = 255, .g = 255, .b = 255, .a = 255 };

	size_t char_offset = 0;
	size_t y_offset = 0;
	UTFStringView sv = utf_sv_from_str(*(box->str));
	size_t font_height = TTF_FontHeight(box->font);

	while (char_offset < box->str->count) {
		size_t new_line_index = utf_sv_find_right_from(sv, utf_sv_from_cstr(u8"\n"), char_offset);
		bool found_new_line = new_line_index >= 0;

		if (!found_new_line) {
			new_line_index = box->str->count;
		}

		UTFStringView line = utf_sv_sub_sv(sv, char_offset, new_line_index);

		if (line.count == 0) {
			char_offset++;
			y_offset += font_height;
			continue;
		}

		size_t measured_count = 0;
		int measured_width = 0;

		while(true){
			bool fits = sv_fits(line, box->font, box->w, &measured_count, &measured_width);
			UTFStringView to_render = utf_sv_sub_sv(line, 0, measured_count);

			char tmp = to_render.data[to_render.data_size];
			to_render.data[to_render.data_size] = 0;
			SDL_Surface* surface = TTF_RenderUTF8_Blended(box->font, to_render.data, color);
			to_render.data[to_render.data_size] = tmp;

			if (surface == NULL) {
				fprintf(stderr, "%s:%d:ERROR : Failed to create a surface for paper texture\n", __FILE__, __LINE__);
				fprintf(stderr, "%s\n", SDL_GetError());
				return;
			}

			SDL_Texture* line_texture = SDL_CreateTextureFromSurface(box->renderer, surface);
			if (line_texture == NULL) {
				fprintf(stderr, "%s:%d:ERROR : Failed to create a texture for paper texture\n", __FILE__, __LINE__);
				fprintf(stderr, "%s\n", SDL_GetError());
				return;
			}
			SDL_Rect line_rect = { .x = 0, .y = y_offset, .w = surface->w, .h = surface->h };

			SDL_RenderCopy(box->renderer, line_texture, NULL, &line_rect);

			SDL_FreeSurface(surface);
			SDL_DestroyTexture(line_texture);

			y_offset += font_height;
			char_offset += measured_count;

			line = utf_sv_trim_left(line, measured_count);

			if (fits) { break; }
		}

		if (found_new_line) {
			char_offset++;
		}
	}

	/////////////////////////////
	// draw cursor
	/////////////////////////////

	//SDL_Color color = { .r = 255, .g = 255, .b = 255, .a = 255 };
	//int font_height = TTF_FontHeight(box->font);

	int cursor_x = 0;
	int cursor_y = 0;
	calculate_cursor_pos(box, &cursor_x, &cursor_y);
	SDL_Rect cursor_rect = { .x = cursor_x, .y = cursor_y + 2, .w = 2, .h = font_height - 2 };

	SDL_SetRenderDrawColor(box->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(box->renderer, &cursor_rect);

	SDL_SetRenderTarget(box->renderer, prev_render_target);
}

TextBox* text_box_create(const char* text, size_t w, size_t h, TTF_Font* font, SDL_Renderer* renderer) 
{
	TextBox* box = malloc(sizeof(TextBox));
	box->w = w;
	box->h = h;

	box->str = utf_create(text);
	box->font = font;

	box->cursor_pos = 0;

	box->offset_y = 0;

	box->renderer = renderer;

	box->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
	if (box->texture == NULL) {
		fprintf(stderr, "%s:%d:ERROR : Failed to create a texture for text box\n", __FILE__, __LINE__);
		return NULL;
	}

	SDL_SetTextureBlendMode(box->texture, SDL_BLENDMODE_BLEND);

	box->paper_texture = NULL;
	text_box_update_paper_texture(box);

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



void text_box_type(TextBox* box, char* c)
{
	utf_insert(box->str, box->cursor_pos, c);
	text_box_move_cursor_right(box);
	text_box_update_paper_texture(box);

	printf("box offset : %d\n", box->offset_y);
	printf("cursor pos : %d\n", box->cursor_pos);
	printf("\n");
}

void text_box_render(TextBox* box) {
	/////////////////////////////
	// Set Render Target
	/////////////////////////////
	SDL_Renderer* renderer = box->renderer;

	SDL_Texture* prev_render_target = SDL_GetRenderTarget(box->renderer);

	SDL_SetRenderTarget(box->renderer, box->texture);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	
	/////////////////////////////
	// Render Text
	/////////////////////////////

	SDL_Rect paper_rect = { .x = 0, .y = -box->offset_y,.w = box->paper_texture_w, .h = box->paper_texture_h};

	if (box->offset_y < 0) {
		paper_rect.h = box->paper_texture_h + box->offset_y;
		if (paper_rect.h > box->h) { 
			paper_rect.h = box->h; 
		}
	}
	/*else if (box->offset_y + box->paper_texture_h > box->h) {
		paper_rect.h = (box->h - box->offset_y);
	}*/

	SDL_Rect render_rect = { .x = 0, .y = box->offset_y,
		.w = paper_rect.w, .h = paper_rect.h };

	if (box->offset_y < 0) {
		render_rect.y = 0;
	}

	SDL_RenderCopy(renderer, box->paper_texture, &paper_rect, &render_rect);

	//restore render target
	SDL_SetRenderTarget(renderer, prev_render_target);
}

void text_box_move_cursor_left(TextBox* box)
{
	if (box->cursor_pos <= 0) {
		return;
	}

	box->cursor_pos -= 1;
	
	int cursor_x = 0;
	int cursor_y = 0;
	calculate_cursor_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y  < 0) {
		box->offset_y = -cursor_y;
	}
	text_box_update_paper_texture(box);

	printf("box offset : %d\n", box->offset_y);
	printf("cursor pos : %d\n", box->cursor_pos);
	printf("\n");
}

void text_box_move_cursor_right(TextBox* box)
{
	size_t box_text_count = box->str->count;

	if (box->cursor_pos >= box_text_count) {
		return;
	}

	box->cursor_pos += 1;

	int font_height = TTF_FontHeight(box->font);

	int cursor_x = 0;
	int cursor_y = 0;
	calculate_cursor_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y > box->h - font_height) {
		box->offset_y = box->h - cursor_y - font_height;
	}
	text_box_update_paper_texture(box);

	printf("box offset : %d\n", box->offset_y);
	printf("cursor pos : %d\n", box->cursor_pos);
	printf("\n");
}