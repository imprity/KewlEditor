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

void calculate_cursor_pos(TextBox* box, int* cursor_x, int* cursor_y)
{
	int offset_y = 0;
	size_t line_number = box->cursor_line->line_number;

	{
		TextLine* line = box->first_line;
		for (size_t i = 0; i < line_number; i++) {
			offset_y += line->size_y;
			if (line->next) {
				line = line->next;
			}
			else {
				fprintf(stderr, "%s:%d:ERROR : Failed to get a cursor y offset\n", __FILE__, __LINE__);
				fprintf(stderr, "line %zu does not have a next line\n", line->line_number);
				return;
			}
		}
	}

	int font_height = TTF_FontHeight(box->font);

	TextLine* cursor_line = box->cursor_line;

	UTFStringView sv = utf_sv_sub_str(*(box->cursor_line->str), 0, box->cursor_offset);

	size_t fit_count = 0;
	int measured_x = 0;
	while (! sv_fits(sv, box->font, box->w, &fit_count, &measured_x) ) {
		sv = utf_sv_trim_left(sv, fit_count);
		offset_y += font_height;
	}

	if (cursor_x) {
		*cursor_x = measured_x;
	}
	if (cursor_y) {
		*cursor_y = offset_y;
	}
}

void calculate_line_size(TextBox* box, TextLine* line, int* size_x, int* size_y) {
	UTFStringView sv = utf_sv_from_str(*(line->str));
	int font_height = TTF_FontHeight(box->font);

	int y = 0;
	if (sv.count == 0) {
		y += font_height;
	}
	else {
		size_t fit = 0;
		while (sv.count > 0) {
			bool fits = sv_fits(sv, box->font, box->w, &fit, NULL);

			sv = utf_sv_trim_left(sv, fit);

			y += font_height;
		}
	}

	if (size_x) {
		*size_x = box->w;
	}
	if (size_y) {
		*size_y = y;
	}
}

TextBox* text_box_create(const char* text, size_t w, size_t h, TTF_Font* font, SDL_Renderer* renderer) 
{
	TextBox* box = malloc(sizeof(TextBox));
	box->w = w;
	box->h = h;

	box->font = font;

	box->offset_y = 0;

	box->renderer = renderer;
	box->cursor_offset = 0;

	box->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
	if (box->texture == NULL) {
		fprintf(stderr, "%s:%d:ERROR : Failed to create a texture for text box\n", __FILE__, __LINE__);
		return NULL;
	}

	SDL_SetTextureBlendMode(box->texture, SDL_BLENDMODE_BLEND);

	box->first_line = create_lines_from_str(text);
	box->cursor_line = box->first_line;

	//calculate line pixel width and height
	for (TextLine* line = box->first_line; line != NULL; line = line->next) {
		int size_x = 0;
		int size_y = 0;
		calculate_line_size(box, line, &size_x, &size_y);
		line->size_x = size_x;
		line->size_y = size_y;
	}

	return box;
}

void text_box_destroy(TextBox* box)
{
	if (!box) {
		return;
	}

	for (TextLine* line = box->first_line; line != NULL; ) {
		TextLine* tmp_next = line->next;
		text_line_destroy(line);
		line = tmp_next;
	}

	if(box->texture)
		SDL_DestroyTexture(box->texture);

	free(box);
}

void text_box_type(TextBox* box, char* c)
{
	TextLine* cursor_line = box->cursor_line;
	UTFStringView to_insert = utf_sv_from_cstr(c);

	int new_line_index = utf_sv_find(to_insert, utf_sv_from_cstr(u8"\n"));
	bool has_new_line = new_line_index >= 0;

	if (has_new_line) {
		UTFStringView before_new_line = utf_sv_sub_sv(to_insert, 0, new_line_index);
		UTFStringView after_new_line = utf_sv_sub_sv(to_insert, new_line_index+1, to_insert.count);

		UTFStringView after_insertion = utf_sv_sub_sv(utf_sv_from_str(*(cursor_line->str)), box->cursor_offset, cursor_line->str->count);

		TextLine* new_line = text_line_create(utf_from_sv(after_new_line), 0);
		utf_append_sv(new_line->str, after_insertion);
		utf_erase_right(cursor_line->str, cursor_line->str->count - box->cursor_offset);
		
		text_line_insert_right(cursor_line, new_line);
		text_line_set_number_right(new_line, cursor_line->line_number + 1);
		
		//recalculate line size
		int size_x = 0;
		int size_y = 0;
		calculate_line_size(box, cursor_line, &size_x, &size_y);
		cursor_line->size_x = size_x;
		cursor_line->size_y = size_y;
		calculate_line_size(box, new_line, &size_x, &size_y);
		new_line->size_x = size_x;
		new_line->size_y = size_y;

		box->cursor_line = new_line;
		box->cursor_offset = 0;
	}
	else {
		utf_insert(cursor_line->str, box->cursor_offset, c);
		box->cursor_offset += to_insert.count;
		int size_x = 0;
		int size_y = 0;
		calculate_line_size(box, cursor_line, &size_x, &size_y);
		cursor_line->size_x = size_x;
		cursor_line->size_y = size_y;
	}

	int font_height = TTF_FontHeight(box->font);

	int cursor_x = 0;
	int cursor_y = 0;
	calculate_cursor_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y > box->h - font_height) {
		box->offset_y = box->h - cursor_y - font_height;
	}
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

	int font_height = TTF_FontHeight(box->font);

	int offset_y = box->offset_y;
	SDL_Color color = {.r = 255, .g = 255, .b = 255, .a = 255};

	for (TextLine* line = box->first_line; line != NULL; line = line->next) {
		if (offset_y > box->h) {
			break;
		}
		else if(offset_y + line->size_y > 0) {
			if (line->str->count == 0) {
				offset_y += line->size_y;
				continue;
			}
			UTFStringView sv = utf_sv_from_str(*(line->str));

			while (sv.count > 0) {
				size_t fit_count = 0;
				bool fits = sv_fits(sv, box->font, box->w, &fit_count, NULL);
				UTFStringView line_sv = utf_sv_sub_sv(sv, 0, fit_count);
				sv = utf_sv_trim_left(sv, fit_count);

				char tmp = line_sv.data[line_sv.data_size];
				line_sv.data[line_sv.data_size] = 0;
				SDL_Surface* surface = TTF_RenderUTF8_Blended(box->font, line_sv.data, color);
				line_sv.data[line_sv.data_size] = tmp;

				if (!surface) {
					fprintf(stderr, "%s:%d:ERROR : Failed to create a surface for text box\n", __FILE__, __LINE__);
					fprintf(stderr, "%s\n", SDL_GetError());
					goto renderexit;
				}

				SDL_Texture* texture = SDL_CreateTextureFromSurface(box->renderer, surface);
				if (!texture) {
					fprintf(stderr, "%s:%d:ERROR : Failed to create a texture for text box\n", __FILE__, __LINE__);
					fprintf(stderr, "%s\n", SDL_GetError());
					SDL_FreeSurface(surface);
					goto renderexit;
				}

				SDL_Rect line_rect = {.x = 0, .y = offset_y, .w = surface->w, .h = surface->h};
				SDL_RenderCopy(box->renderer, texture, NULL, &line_rect);
				
				SDL_FreeSurface(surface);
				SDL_DestroyTexture(texture);
				
				offset_y += font_height;
			}
		}
		else {
			offset_y += line->size_y;
		}
	}

	/////////////////////////////
	// Render Cursor
	/////////////////////////////
	int cursor_x = 0;
	int cursor_y = 0;
	
	calculate_cursor_pos(box, &cursor_x, &cursor_y);
	SDL_Rect cursor_rect = { .x = cursor_x, .y = cursor_y + 2 + box->offset_y, .w = 2, .h = font_height - 2 };

	SDL_SetRenderDrawColor(box->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(box->renderer, &cursor_rect);

	renderexit:

	//restore render target
	SDL_SetRenderTarget(renderer, prev_render_target);
}

void text_box_move_cursor_left(TextBox* box)
{
	box->cursor_offset -= 1;

	if (box->cursor_offset < 0) {
		if (box->cursor_line == box->first_line) {
			box->cursor_offset = 0;
		}
		else {
			box->cursor_line = box->cursor_line->prev;
			box->cursor_offset = box->cursor_line->str->count;
		}
	}
	
	int cursor_x = 0;
	int cursor_y = 0;
	calculate_cursor_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y  < 0) {
		box->offset_y = -cursor_y;
	}
}

void text_box_move_cursor_right(TextBox* box)
{
	// TODO : Properly handle cursor movement
	box->cursor_offset += 1;

	if (box->cursor_offset > box->cursor_line->str->count) {
		if (box->cursor_line->next) {
			box->cursor_line = box->cursor_line->next;
			box->cursor_offset = 0;
		}
		else {
			box->cursor_offset = box->cursor_line->str->count;
		}
	}

	int font_height = TTF_FontHeight(box->font);

	int cursor_x = 0;
	int cursor_y = 0;
	calculate_cursor_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y > box->h - font_height) {
		box->offset_y = box->h - cursor_y - font_height;
	}
}