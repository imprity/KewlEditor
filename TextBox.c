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

size_t get_cursor_char_offset(TextBox* box) {
	TextLine* cursor_line = box->cursor_line;
	UTFStringView sv = utf_sv_from_str((*cursor_line->str));

	if (cursor_line->wrapped_line_count <= 1) {
		return min(sv.count, box->cursor_offset_x);
	}
	
	size_t char_offset = 0;
	for (size_t i = 0; i < cursor_line->wrapped_line_count; i++) {
		if (i >= box->cursor_offset_y) {
			return char_offset + min(box->cursor_offset_x, cursor_line->wrapped_line_sizes[i]);
		}

		//if we are at last line and cursor offset y is bigger then cached wrapped line sizes
		//then just return the char offset from last line
		if (i + 1 >= cursor_line->wrapped_line_count) {	
			return char_offset + min(box->cursor_offset_x, cursor_line->wrapped_line_sizes[i]);
		}
		char_offset += cursor_line->wrapped_line_sizes[i];
	}
}

void set_cursor_offsets_from_char_offset(TextBox* box, size_t char_offset)
{
	if (char_offset == 0) {
		box->cursor_offset_x = 0;
		box->cursor_offset_y = 0;
		return;
	}
	size_t offset_y = 0;
	size_t total_char = 0;
	for (int i = 0; i < box->cursor_line->wrapped_line_count; i++) {
		if (char_offset <= total_char + box->cursor_line->wrapped_line_sizes[i]) {
			box->cursor_offset_x = char_offset - total_char;
			box->cursor_offset_y = offset_y;
			return;
		}
		offset_y++;
		total_char += box->cursor_line->wrapped_line_sizes[i];
	}
}

void get_cursor_screen_pos(TextBox* box, int* cursor_x, int* cursor_y)
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

	offset_y += font_height * box->cursor_offset_y;

	TextLine* cursor_line = box->cursor_line;

	UTFStringView sv = utf_sv_sub_str(*(box->cursor_line->str), 0, get_cursor_char_offset(box));

	for (size_t i = 0; i < min(cursor_line->wrapped_line_count, box->cursor_offset_y); i++) {
		sv = utf_sv_trim_left(sv, cursor_line->wrapped_line_sizes[i]);
	}

	int measured_x = 0;
	sv_fits(sv, box->font, box->w, NULL, &measured_x);

	if (cursor_x) {
		*cursor_x = measured_x;
	}
	if (cursor_y) {
		*cursor_y = offset_y;
	}
}


void update_text_line(TextBox* box, TextLine* line) 
{
	UTFStringView sv = utf_sv_from_str(*(line->str));
	int font_height = TTF_FontHeight(box->font);

	if (sv.count == 0) {
		line->size_y = font_height;
		line->size_x = box->w;
		line->wrapped_line_count = 1;
		line->wrapped_line_sizes[0] = 0;
		return;
	}

	line->size_x = box->w;

	size_t size_offset = 0;
	line->wrapped_line_count = 0;
	line->size_y = 0;

	while (true) {
		size_t measured_count = 0;
		bool fits = sv_fits(sv, box->font, box->w, &measured_count, NULL);
		line->wrapped_line_sizes[size_offset++] = measured_count;
		line->wrapped_line_count++;
		line->size_y += font_height;
		sv = utf_sv_trim_left(sv, measured_count);
		if (fits) {
			break;
		}
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

	box->cursor_offset_x = 0;
	box->cursor_offset_y = 0;

	box->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
	if (box->texture == NULL) {
		fprintf(stderr, "%s:%d:ERROR : Failed to create a texture for text box\n", __FILE__, __LINE__);
		return NULL;
	}

	SDL_SetTextureBlendMode(box->texture, SDL_BLENDMODE_BLEND);

	box->first_line = create_lines_from_cstr(text);
	box->cursor_line = box->first_line;

	//calculate line pixel width and height
	for (TextLine* line = box->first_line; line != NULL; line = line->next) {
		update_text_line(box, line);
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
	UTFStringView sv = utf_sv_from_cstr(c);
	int new_line_index = utf_sv_find(sv, utf_sv_from_cstr(u8"\n"));
	bool has_new_line = new_line_index >= 0;

	TextLine* cursor_line = box->cursor_line;

	if (has_new_line) {
		//divide typed lines to before new line and after new line
		UTFStringView before_new_line = utf_sv_sub_sv(sv, 0, new_line_index);
		UTFStringView after_new_line = utf_sv_sub_sv(sv, new_line_index+1, sv.count);

		//create new sub string from original string that starts from insertion point
		size_t insertion_point = get_cursor_char_offset(box);
		UTFString *after_insertion = utf_sub_str(cursor_line->str, insertion_point, cursor_line->str->count);

		//trim current line to where insertion happens
		utf_erase_right(cursor_line->str, cursor_line->str->count - insertion_point);
		//append before new line to current line
		utf_append_sv(cursor_line->str, before_new_line);

		size_t cursor_char_pos = 0;

		//create new text lines from after new line
		TextLine* new_lines;

		if (after_new_line.count == 0) {
			//if after_new_line is empty then new_lines is just after insertion
			cursor_char_pos = 0;
			new_lines = text_line_create(after_insertion, 0);
		}
		else {
			//else we create new lines from after_new_line and append after_insertion at the end
			new_lines = create_lines_from_sv(after_new_line);
			//text_line_push_back(new_lines, text_line_create(after_insertion, 0));
			TextLine* new_lines_last = text_line_last(new_lines);
			cursor_char_pos = new_lines_last->str->count;
			utf_append(new_lines_last->str, after_insertion->data);
			utf_destroy(after_insertion);
		}

		TextLine* new_line_last = text_line_last(new_lines);
		//update new lines
		for (TextLine* line = new_lines; line != NULL; line = line->next) {
			update_text_line(box, line);
		}
		//insert new lines after current line
		text_line_insert_right(cursor_line, new_lines);
		text_line_set_number_right(cursor_line, cursor_line->line_number);
		//update current line
		update_text_line(box, box->cursor_line);

		//calulate cursor pos
		box->cursor_line = new_line_last;
		set_cursor_offsets_from_char_offset(box, cursor_char_pos);
	}
	else {
		//utf_append(box->cursor_line->str, c);
		size_t char_offset = get_cursor_char_offset(box);
		utf_insert_sv(box->cursor_line->str, char_offset, sv);
		update_text_line(box, box->cursor_line);
		set_cursor_offsets_from_char_offset(box, char_offset + sv.count);
	}

	int font_height = TTF_FontHeight(box->font);

	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y > box->h - font_height) {
		box->offset_y = box->h - cursor_y - font_height;
	}

	printf("cursor offset x : %zu\n", box->cursor_offset_x);
	printf("cursor offset y : %zu\n", box->cursor_offset_y);
	printf("line number     : %zu\n", box->cursor_line->line_number);
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
	
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	SDL_Rect cursor_rect = { .x = cursor_x, .y = cursor_y + 2 + box->offset_y, .w = 2, .h = font_height - 2 };

	SDL_SetRenderDrawColor(box->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(box->renderer, &cursor_rect);

	renderexit:

	//restore render target
	SDL_SetRenderTarget(renderer, prev_render_target);
}

void text_box_move_cursor_left(TextBox* box)
{
	if (box->cursor_offset_x <= 0) {
		if (box->cursor_offset_y <= 0) {
			if (box->cursor_line == box->first_line) {
				return;
			}
			else {
				box->cursor_line = box->cursor_line->prev;
				box->cursor_offset_y = box->cursor_line->wrapped_line_count - 1;
				box->cursor_offset_x = box->cursor_line->wrapped_line_sizes[box->cursor_line->wrapped_line_count - 1];
			}
		}
		else {
			box->cursor_offset_y--;
			box->cursor_offset_x = box->cursor_line->wrapped_line_sizes[box->cursor_offset_y];
		}
	}
	else {
		size_t current_line_size = box->cursor_line->wrapped_line_sizes[min(box->cursor_offset_y, box->cursor_line->wrapped_line_count - 1)];
		if (box->cursor_offset_x > current_line_size)
		{
			box->cursor_offset_x = current_line_size;
		}
		box->cursor_offset_x--;
	}
	
	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y  < 0) {
		box->offset_y = -cursor_y;
	}

	printf("cursor offset x : %zu\n", box->cursor_offset_x);
	printf("cursor offset y : %zu\n", box->cursor_offset_y);
	printf("line number     : %zu\n", box->cursor_line->line_number);
	printf("\n");
}

void text_box_move_cursor_right(TextBox* box)
{
	TextLine* cursor_line = box->cursor_line;
	size_t cursor_line_count = cursor_line->wrapped_line_sizes[min(box->cursor_offset_y, cursor_line->wrapped_line_count - 1)];
	if (box->cursor_offset_x >= cursor_line_count)
	{
		if (box->cursor_offset_y+1 < cursor_line->wrapped_line_count) {
			box->cursor_offset_y++;
			box->cursor_offset_x = 0;
		}
		else if (box->cursor_line->next != NULL) {
			box->cursor_line = box->cursor_line->next;
			box->cursor_offset_x = 0;
			box->cursor_offset_y = 0;
		}
		else {
			return;
		}
	}
	else {
		box->cursor_offset_x++;
	}

	int font_height = TTF_FontHeight(box->font);

	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y > box->h - font_height) {
		box->offset_y = box->h - cursor_y - font_height;
	}

	printf("cursor offset x : %zu\n", box->cursor_offset_x);
	printf("cursor offset y : %zu\n", box->cursor_offset_y);
	printf("line number     : %zu\n", box->cursor_line->line_number);
	printf("\n");
}

void text_box_move_cursor_up(TextBox* box)
{
	if (box->cursor_offset_y > 0) {
		box->cursor_offset_y--;
	}
	else {
		if (box->cursor_line != box->first_line) {
			box->cursor_line = box->cursor_line->prev;
			box->cursor_offset_y = box->cursor_line->wrapped_line_count - 1;
		}
	}

	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y < 0) {
		box->offset_y = -cursor_y;
	}

	printf("cursor offset x : %zu\n", box->cursor_offset_x);
	printf("cursor offset y : %zu\n", box->cursor_offset_y);
	printf("line number     : %zu\n", box->cursor_line->line_number);
	printf("\n");
}

void text_box_move_cursor_down(TextBox* box)
{
	if (box->cursor_line->wrapped_line_count > 1 && box->cursor_offset_y < box->cursor_line->wrapped_line_count -1) {
		box->cursor_offset_y++;
	}
	else {
		if (box->cursor_line->next) {
			box->cursor_line = box->cursor_line->next;
			box->cursor_offset_y = 0;
		}
	}

	int font_height = TTF_FontHeight(box->font);

	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y > box->h - font_height) {
		box->offset_y = box->h - cursor_y - font_height;
	}

	printf("cursor offset x : %zu\n", box->cursor_offset_x);
	printf("cursor offset y : %zu\n", box->cursor_offset_y);
	printf("line number     : %zu\n", box->cursor_line->line_number);
	printf("\n");
}