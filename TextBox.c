#include "TextBox.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <assert.h>

#define TEST_TEXT_ENGLISH (\
"When danger makes me want to hide, you'll Rainbow Dash to my side\n"  \
"Kindness is never in short supply, once smitten twice Fluttershy\n"   \
"For honesty no pony can deny, you are the Applejack of my eye\n"      \
"A heart that shines so beautiful, a Rarity to come by\n"              \
"And you all make funand laughter as easy as Pinkie Pie\n")

#define TEST_TEXT_KOREAN (\
u8"사과의 특징\n"                                                                                         \
u8"과육은 기본적으로 노란색~연두색이며, 맛은 품종마다 다르다.아래 사과 품종 문단을 참고하자.\n"                     \
u8"일반적으로 한국에서 말하는 사과 맛은 달콤새콤 + 아삭아삭하게 씹히는 탄력이 있고 단단한 과육의 식감을 말한다.\n"     \
u8"종마다 다르지만 잘 익은 사과는 껍질이 벗겨지지 않은 상태에서도 청량감이 있는 좋은 냄새가 난다.\n"                 \
u8"\n"                                                                                                   \
u8"야생 사과는 키르기스스탄과 중국 사이에 위치한 톈산 산맥과 타림 분지가 원산지로, 이후 전 세계에 퍼지게 되었다.\n"    \
u8"참고로 다른 과일인 배와 복숭아도 같은 지역이 원산지이다.\n")


bool sv_fits(UTFStringView sv, TTF_Font* font, int w, size_t* text_count, int* text_width) {
	if (sv.count == 0) {
		if (text_count) {
			*text_count = 0;
		}
		if (*text_width) {
			*text_width = 0;
		}
		return true;
	}

	int measured_count = 0;
	int measured_width = 0;
	bool fits = true;

	char tmp = sv.data[sv.data_size];
	sv.data[sv.data_size] = 0;
	TTF_MeasureUTF8(font, sv.data, w, &measured_width, &measured_count);
	sv.data[sv.data_size] = tmp;

	if (text_count) {
		*text_count = measured_count;
	}
	if (text_width) {
		*text_width = measured_width;
	}

	return sv.count == measured_count;
}

size_t get_cursor_char_offset(TextBox* box) {
	TextLine* cursor_line = box->cursor_line;
	UTFStringView sv = utf_sv_from_str(cursor_line->str);

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

void get_screen_pos_from_line_and_char_offset(
	TextBox* box, TextLine* line, size_t char_offset,
	int* pos_x, int* pos_y
)
{
	int pixel_offset_y = 0;

	size_t line_number = line->line_number;

	{
		TextLine* line = box->first_line;
		for (size_t i = 0; i < line_number; i++) {
			pixel_offset_y += line->size_y;
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

	size_t c = 0;
	for (size_t i = 0; i < line->wrapped_line_count; i++) {
		if (c + line->wrapped_line_sizes[i] >= char_offset) {
			break;
		}
		pixel_offset_y += font_height;
		c += line->wrapped_line_sizes[i];
	}

	UTFStringView sv = utf_sv_sub_str((line->str), c, char_offset);

	int pixel_offset_x = 0;

	sv_fits(sv, box->font, box->w, NULL, &pixel_offset_x);

	if (pos_x) {
		*pos_x = pixel_offset_x;
	}
	if (pos_y) {
		*pos_y = pixel_offset_y;
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

	UTFStringView sv = utf_sv_sub_str((box->cursor_line->str), 0, get_cursor_char_offset(box));

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

void text_box_handle_event(TextBox* box, SDL_Event* event)
{
	switch (event->type) {

	case SDL_TEXTINPUT: {
		text_box_type(box, event->text.text);
	}break;
	case SDL_KEYDOWN: {
		SDL_Keysym key = event->key.keysym;
		switch (key.scancode) {

		case SDL_SCANCODE_RETURN: {
			text_box_type(box, u8"\n");
		}break;
		case SDL_SCANCODE_LEFT: {
			text_box_move_cursor_left(box);
		}break;
		case SDL_SCANCODE_RIGHT: {
			text_box_move_cursor_right(box);
		}break;
		case SDL_SCANCODE_UP: {
			text_box_move_cursor_up(box);
		}break;
		case SDL_SCANCODE_DOWN: {
			text_box_move_cursor_down(box);
		}break;
		case SDL_SCANCODE_BACKSPACE: {
			if (box->is_selecting) {
				text_box_delete_range(box, box->selection);
			}
			else{
				text_box_delete_a_character(box);
			}
			
		}break;
		case SDL_SCANCODE_F1: {
			text_box_type(box, TEST_TEXT_ENGLISH);
		}break;
		case SDL_SCANCODE_F2: {
			text_box_type(box, TEST_TEXT_KOREAN);
		}break;
		case SDL_SCANCODE_F3: {
			if (box->is_selecting) {
				text_box_end_selection(box);
			}
			else {
				text_box_start_selection(box);
			}
		}break;

		}
	}break;

	}
}

void update_text_line(TextBox* box, TextLine* line)
{
	UTFStringView sv = utf_sv_from_str(line->str);
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

TextBox* text_box_create(
	const char* text,
	size_t w, size_t h,
	TTF_Font* font,
	SDL_Color bg_color, SDL_Color text_color, SDL_Color selection_bg, SDL_Color selection_fg, SDL_Color cursor_color,
	SDL_Renderer* renderer)
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

	box->selection.start_char = 0;
	box->selection.end_char = 0;

	box->selection.start_line = NULL;
	box->selection.end_line = NULL;

	box->is_selecting = false;

	box->bg_color = bg_color;
	box->text_color = text_color;
	box->selection_bg = selection_bg;
	box->selection_fg = selection_fg;
	box->cursor_color = cursor_color;

	box->need_to_render = true;

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

	if (box->texture)
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
		UTFStringView after_new_line = utf_sv_sub_sv(sv, new_line_index + 1, sv.count);

		//create new sub string from original string that starts from insertion point
		size_t insertion_point = get_cursor_char_offset(box);
		UTFString* after_insertion = utf_sub_str(cursor_line->str, insertion_point, cursor_line->str->count);

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
			utf_append_cstr(new_lines_last->str, after_insertion->data);
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

	box->need_to_render = true;
}

size_t get_line_num_from_line_and_char_offset(TextLine* line, size_t char_offset)
{
	size_t offset = 0;
	for (size_t i = 0; i < line->wrapped_line_count; i++) {
		if (char_offset < offset + line->wrapped_line_sizes[i]) {
			return i;
		}
		offset += line->wrapped_line_sizes[i];
	}
	return line->wrapped_line_count - 1;
}

bool draw_sv(
	TextBox* box,
	UTFStringView sv,
	int pos_x,
	int pos_y,
	bool shaded,
	SDL_Color text_color,
	SDL_Color fg_color,
	SDL_Color bg_color,
	int* rendered_size_x,
	int* rendered_size_y
) {
	if (sv.count == 0) {
		if (rendered_size_x) {
			*rendered_size_x = 0;
		}
		if (rendered_size_y) {
			*rendered_size_y = 0;
		}
		return true;
	}
	SDL_Surface* surface;
	char tmp = sv.data[sv.data_size];
	sv.data[sv.data_size] = 0;

	if (!shaded) {
		//surface = TTF_RenderUTF8_Blended(box->font, sv.data, text_color);
		surface = TTF_RenderUTF8_LCD(box->font, sv.data, text_color, box->bg_color);
	}
	else {
		surface = TTF_RenderUTF8_LCD(box->font, sv.data, fg_color, bg_color);
	}

	sv.data[sv.data_size] = tmp;

	if (!surface) {
		fprintf(stderr, "%s:%d:ERROR : Failed to create a surface for text box\n", __FILE__, __LINE__);
		fprintf(stderr, "%s\n", SDL_GetError());
		return false;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(box->renderer, surface);
	if (!texture) {
		fprintf(stderr, "%s:%d:ERROR : Failed to create a texture for text box\n", __FILE__, __LINE__);
		fprintf(stderr, "%s\n", SDL_GetError());
		SDL_FreeSurface(surface);
		return false;
	}

	SDL_Rect line_rect = { .x = pos_x, .y = pos_y, .w = surface->w, .h = surface->h };

	SDL_RenderCopy(box->renderer, texture, NULL, &line_rect);

	if (rendered_size_x) {
		*rendered_size_x = surface->w;
	}
	if (rendered_size_y) {
		*rendered_size_y = surface->h;
	}

	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);

	return true;
}

Selection normalize_selection(Selection selection)
{
	Selection to_return = selection;

	if (selection.start_line == NULL || selection.end_line == NULL) {
		return to_return;
	}
	
	if (selection.start_line == selection.end_line) {
		if (selection.start_char > selection.end_char) {
			to_return.end_char = selection.start_char;
			to_return.start_char = selection.end_char;
		}
	}
	else {
		if (selection.start_line->line_number > selection.end_line->line_number) {
			to_return.start_line = selection.end_line;
			to_return.start_char = selection.end_char;

			to_return.end_line = selection.start_line;
			to_return.end_char = selection.start_char;
		}
	}

	return to_return;
}


void text_box_render(TextBox* box) {
	if (!box->need_to_render) {
		return;
	}
	/////////////////////////////
	// Set Render Target
	/////////////////////////////
	SDL_Renderer* renderer = box->renderer;

	SDL_Texture* prev_render_target = SDL_GetRenderTarget(box->renderer);
	SDL_BlendMode prev_blend_mode = 0;
	SDL_GetRenderDrawBlendMode(renderer, &prev_blend_mode);

	SDL_SetRenderTarget(box->renderer, box->texture);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer, box->bg_color.r, box->bg_color.g, box->bg_color.b, box->bg_color.a);
	SDL_RenderClear(renderer);

	int font_height = TTF_FontHeight(box->font);


	bool no_selection = !box->is_selecting;
	no_selection = no_selection ||
		box->selection.start_line == box->selection.end_line &&
		box->selection.start_char == box->selection.end_char;

	Selection selection = normalize_selection(box->selection);


	/////////////////////////////
	// Render Text
	/////////////////////////////
	int pixel_offset_y = box->offset_y;

	for (TextLine* line = box->first_line; line != NULL; line = line->next) {
		if (pixel_offset_y > box->h) {
			goto text_render_end;
		}
		else if(pixel_offset_y + line->size_y <= 0) {
			pixel_offset_y += line->size_y;
		}
		else {
			if (line->str->count == 0) {
				pixel_offset_y += line->size_y;
				continue;
			}

			bool completely_inside_selection = false;
			bool partially_inside_selection = false;
			bool outside_selecton = true;

			if (!no_selection) {
				completely_inside_selection = (
					line->line_number > selection.start_line->line_number &&
					line->line_number < selection.end_line->line_number
					);
				partially_inside_selection = (
					line->line_number == selection.start_line->line_number ||
					line->line_number == selection.end_line->line_number
					);
				outside_selecton = !completely_inside_selection && !partially_inside_selection;
			}

			if (outside_selecton || completely_inside_selection) {
				UTFStringView sv = utf_sv_from_str(line->str);

				size_t char_offset = 0;

				for (size_t i = 0; i < line->wrapped_line_count; i++) {
					if (pixel_offset_y > box->h) {
						goto text_render_end;
					}
					size_t line_start = char_offset;
					size_t line_end = line->wrapped_line_sizes[i] + char_offset;
						
					UTFStringView line_sv = utf_sv_sub_sv(sv, line_start, line_end);
					if (
						!draw_sv(box, line_sv, 0, pixel_offset_y,
							completely_inside_selection && box->is_selecting,
							box->text_color, box->selection_fg, box->selection_bg,
							NULL, NULL
						)) 
					{
						goto renderexit;
					}
					char_offset += line->wrapped_line_sizes[i];
					pixel_offset_y += font_height;
				}
			}
			else {
				UTFStringView sv = utf_sv_from_str(line->str);

				size_t char_offset = 0;

				for (size_t i = 0; i < line->wrapped_line_count; i++) {
					if (pixel_offset_y > box->h) {
						goto text_render_end;
					}
					size_t line_start = char_offset;
					size_t line_end = line->wrapped_line_sizes[i] + char_offset;
						
					UTFStringView line_sv = utf_sv_sub_sv(sv, line_start, line_end);

					bool left_shaded = selection.end_char <= line_end && selection.end_char >= line_start;
					left_shaded = left_shaded && (selection.end_line == line);

					bool right_shaded = selection.start_char <= line_end && selection.start_char >= line_start;
					right_shaded = right_shaded && (selection.start_line == line);

					bool un_shaded = line == selection.start_line && line_end < selection.start_char && line_start < selection.start_char;
					un_shaded = un_shaded || (line == selection.end_line && line_start > selection.end_char && line_end > selection.end_char);


					if (left_shaded && right_shaded) {
						UTFStringView left = utf_sv_sub_sv(line_sv, 0, selection.start_char - char_offset);
						UTFStringView right = utf_sv_sub_sv(line_sv, selection.end_char - char_offset, line_sv.count);
						UTFStringView between = utf_sv_sub_sv(line_sv, selection.start_char - char_offset, selection.end_char - char_offset);

						int pixel_offset_x = 0;
						int rendered_size_x = 0;
						if (
							!draw_sv(box, left, 0, pixel_offset_y,
								false,
								box->text_color, box->selection_fg, box->selection_bg,
								&rendered_size_x, NULL
							)) 
						{
							goto renderexit;
						}
						pixel_offset_x += rendered_size_x;
						if (
							!draw_sv(box, between, pixel_offset_x, pixel_offset_y,
								true,
								box->text_color, box->selection_fg, box->selection_bg,
								&rendered_size_x, NULL
							)) 
						{
							goto renderexit;
						}
						pixel_offset_x += rendered_size_x;
						if (
							!draw_sv(box, right, pixel_offset_x, pixel_offset_y,
								false,
								box->text_color, box->selection_fg, box->selection_bg,
								NULL, NULL
							)) 
						{
							goto renderexit;
						}
					}
					else if (left_shaded) {
						UTFStringView left = utf_sv_sub_sv(line_sv, 0, selection.end_char - char_offset);
						UTFStringView right = utf_sv_sub_sv(line_sv, selection.end_char - char_offset, line_sv.count);
						int pixel_offset_x = 0;
						int rendered_size_x = 0;
						if (
							!draw_sv(box, left, 0, pixel_offset_y,
								true,
								box->text_color, box->selection_fg, box->selection_bg,
								&rendered_size_x, NULL
							)) 
						{
							goto renderexit;
						}
						pixel_offset_x += rendered_size_x;
						if (
							!draw_sv(box, right, pixel_offset_x, pixel_offset_y,
								false,
								box->text_color, box->selection_fg, box->selection_bg,
								NULL, NULL
							)) 
						{
							goto renderexit;
						}
					}
					else if (right_shaded) {
						UTFStringView left = utf_sv_sub_sv(line_sv, 0, selection.start_char - char_offset);
						UTFStringView right = utf_sv_sub_sv(line_sv, selection.start_char - char_offset, line_sv.count);
						int pixel_offset_x = 0;
						int rendered_size_x = 0;
						if (
							!draw_sv(box, left, 0, pixel_offset_y,
								false,
								box->text_color, box->selection_fg, box->selection_bg,
								&rendered_size_x, NULL
							)) 
						{
							goto renderexit;
						}
						pixel_offset_x += rendered_size_x;
						if (
							!draw_sv(box, right, pixel_offset_x, pixel_offset_y,
								true,
								box->text_color, box->selection_fg, box->selection_bg,
								NULL, NULL
							)) 
						{
							goto renderexit;
						}
					}
					else {
						if (
							!draw_sv(box, line_sv, 0, pixel_offset_y,
								!un_shaded,
								box->text_color, box->selection_fg, box->selection_bg,
								NULL, NULL
							))
						{
							goto renderexit;
						}
					}
					char_offset += line->wrapped_line_sizes[i];
					pixel_offset_y += font_height;
				}
			}
		}
	}
text_render_end:

	/////////////////////////////
	// Render Cursor
	/////////////////////////////
	int cursor_x = 0;
	int cursor_y = 0;

	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	SDL_Rect cursor_rect = { .x = cursor_x, .y = cursor_y + 2 + box->offset_y, .w = 2, .h = font_height - 2 };

	SDL_SetRenderDrawColor(box->renderer, box->cursor_color.r, box->cursor_color.g, box->cursor_color.b, box->cursor_color.a);
	SDL_RenderFillRect(box->renderer, &cursor_rect);

renderexit:

	box->need_to_render = false;

	//restore render target
	SDL_SetRenderTarget(renderer, prev_render_target);
	//restore blend mode
	SDL_SetRenderDrawBlendMode(renderer,prev_blend_mode);
}

void text_box_update_cursor_selection(TextBox* box) {
	box->selection.end_line = box->cursor_line;
	box->selection.end_char = get_cursor_char_offset(box);
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

	if (box->is_selecting) {
		text_box_update_cursor_selection(box);
	}

	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y < 0) {
		box->offset_y = -cursor_y;
	}

	box->need_to_render = true;
}

void text_box_move_cursor_right(TextBox* box)
{
	TextLine* cursor_line = box->cursor_line;
	size_t cursor_line_count = cursor_line->wrapped_line_sizes[min(box->cursor_offset_y, cursor_line->wrapped_line_count - 1)];
	if (box->cursor_offset_x >= cursor_line_count)
	{
		if (box->cursor_offset_y + 1 < cursor_line->wrapped_line_count) {
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

	if (box->is_selecting) {
		text_box_update_cursor_selection(box);
	}

	int font_height = TTF_FontHeight(box->font);

	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y > box->h - font_height) {
		box->offset_y = box->h - cursor_y - font_height;
	}

	box->need_to_render = true;
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

	if (box->is_selecting) {
		text_box_update_cursor_selection(box);
	}

	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y < 0) {
		box->offset_y = -cursor_y;
	}

	box->need_to_render = true;
}

void text_box_move_cursor_down(TextBox* box)
{
	if (box->cursor_line->wrapped_line_count > 1 && box->cursor_offset_y < box->cursor_line->wrapped_line_count - 1) {
		box->cursor_offset_y++;
	}
	else {
		if (box->cursor_line->next) {
			box->cursor_line = box->cursor_line->next;
			box->cursor_offset_y = 0;
		}
	}

	if (box->is_selecting) {
		text_box_update_cursor_selection(box);
	}

	int font_height = TTF_FontHeight(box->font);

	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y > box->h - font_height) {
		box->offset_y = box->h - cursor_y - font_height;
	}

	box->need_to_render = true;
}

void text_box_delete_a_character(TextBox* box)
{
	if (box->cursor_line == box->first_line &&
		box->cursor_offset_x == 0 &&
		box->cursor_offset_y == 0) {
		return;
	}

	size_t char_offset = get_cursor_char_offset(box);
	if (char_offset <= 0) {
		if (box->cursor_line->prev == NULL) {
			return;
		}
		TextLine* prev_line = box->cursor_line->prev;
		size_t line_count = prev_line->str->count;
		utf_append_str(prev_line->str, box->cursor_line->str);

		prev_line->next = box->cursor_line->next;
		if (box->cursor_line->next) {
			box->cursor_line->next->prev = prev_line;
		}
		text_line_destroy(box->cursor_line);
		box->cursor_line = prev_line;

		text_line_set_number_right(box->cursor_line, box->cursor_line->line_number);
		update_text_line(box, box->cursor_line);

		set_cursor_offsets_from_char_offset(box, line_count);
	}
	else {
		utf_erase_range(box->cursor_line->str, char_offset - 1, char_offset);
		update_text_line(box, box->cursor_line);
		set_cursor_offsets_from_char_offset(box, char_offset - 1);
	}

	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y < 0) {
		box->offset_y = -cursor_y;
	}

	box->need_to_render = true;
}

void text_box_delete_range(TextBox* box, Selection selection)
{
	selection = normalize_selection(selection);

	if (selection.end_line == NULL || selection.start_line == NULL) {
		return;
	}

	if (selection.start_line == selection.end_line) {
		utf_erase_range(selection.start_line->str, selection.start_char, selection.end_char);
		set_cursor_offsets_from_char_offset(box, selection.start_char);
		update_text_line(box, selection.start_line);
		box->need_to_render = true;

		// TODOOOOOOOOOO : All these funcitons violate single responsibility principle...
		text_box_end_selection(box);
	}
	else {
		//first get texts after selection end char
		UTFString *start_str = selection.start_line->str;
		UTFString *end_str = selection.end_line->str;
		UTFStringView after_selection_end_char = utf_sv_sub_str(end_str, selection.end_char, end_str->count);

		//erase start_str after selection start char
		utf_erase_range(start_str, selection.start_char, start_str->count);
		//append after_selection_end_char 
		utf_append_sv(start_str, after_selection_end_char);

		//free lines between start and end
		assert(selection.start_line->next != NULL);
		for (TextLine* line = selection.start_line->next; line != selection.end_line;) {
			TextLine* tmp = line->next;
			text_line_destroy(line);
			line = tmp;
		}
		selection.start_line->next = selection.end_line->next;
		if (selection.end_line->next) {
			selection.end_line->next->prev = selection.start_line;
		}
		text_line_destroy(selection.end_line);

		update_text_line(box, selection.start_line);
		box->cursor_line = selection.start_line;
		set_cursor_offsets_from_char_offset(box, selection.start_char);

		text_line_set_number_right(selection.start_line, selection.start_line->line_number);

		// TODOOOOOOOOOO : All these funcitons violate single responsibility principle...
		text_box_end_selection(box);
	}
}

void text_box_start_selection(TextBox* box)
{
	box->is_selecting = true;

	size_t char_offset = get_cursor_char_offset(box);

	box->selection.start_line = box->cursor_line;
	box->selection.start_char = char_offset;

	box->selection.end_line = box->cursor_line;
	box->selection.end_char = char_offset;

	box->need_to_render = true;
}

void text_box_end_selection(TextBox* box)
{
	box->is_selecting = false;

	box->need_to_render = true;
}