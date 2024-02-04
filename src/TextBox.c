#include "TextBox.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <assert.h>

#pragma execution_character_set("utf-8")

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


#define MISSING_GLYPH "?"

#define min(a, b) ((a) > (b) ?  b : a)

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

	/*
	char tmp = sv.data[sv.data_size];
	sv.data[sv.data_size] = 0;
	TTF_MeasureUTF8(font, sv.data, w, &measured_width, &measured_count);
	sv.data[sv.data_size] = tmp;
	*/
	UTFString *tmp = utf_from_sv(sv);
	TTF_MeasureUTF8(font, tmp->data, w, &measured_width, &measured_count);
	utf_destroy(tmp);

	if (text_count) {
		*text_count = measured_count;
	}
	if (text_width) {
		*text_width = measured_width;
	}

	return sv.count == (size_t)measured_count;
}

TextLine* get_line_from_line_number(TextBox* box, size_t line_number) {
	TextLine* line = box->first_line;
	for (size_t i = 0; i < line_number; i++) {
		if (line->next == NULL) {
			return line;
		}
		line = line->next;
	}
	return line;
}

//TextLine* get_cursor_line(TextBox* box) {
//	return get_line_from_line_number(box, box->cursor.line_number);
//}

void get_char_coord_from_cursor(
	TextBox* box, TextCursor cursor, size_t* x, size_t* y
)
{
	if (cursor.char_offset == 0) {
		if (x) {*x = 0;}
		if (y) { *y = 0; }
		return;
	}

	TextLine* line = get_line_from_line_number(box, cursor.line_number);

	size_t total_char_size = 0;
	for (size_t i = 0; i < line->wrapped_line_count; i++) {
		size_t line_size = line->wrapped_line_sizes[i];
		size_t line_start = total_char_size;
		size_t line_end = total_char_size + line_size;
		total_char_size += line_size;

		if (cursor.char_offset >= line_start && cursor.char_offset <= line_end) {
			//char offset is at the end of the wrapped line
			if (cursor.char_offset  == line_end && i + 1 < line->wrapped_line_count) {
				if (cursor.place_after_last_char_before_wrapping) {
					if (x) { *x = cursor.char_offset - line_start; }
					if (y) { *y = i; }
					return;
				}
				else {
					if (x) { *x = 0; }
					if (y) { *y = i + 1; }
					return;
				}
			}
			else {
				if (x) { *x = cursor.char_offset - line_start; }
				if (y) { *y = i ; }
				return;
			}
		}
	}
	if (x) { *x = line->wrapped_line_sizes[line->wrapped_line_count-1]; }
	if (y) { *y = line->wrapped_line_count-1; }
}

UTFString* replace_missing_glyph_with_char(UTFStringView sv, TTF_Font* font, UTFStringView replacement) {

	size_t bit_offset = 0;
	const char *ptr = sv.data;

	UTFString* copy = utf_from_cstr("");

	if (sv.count == 0) {
		return copy;
	}

	for (size_t i = 0; i < sv.count; i++) {
		//size_t next = utf_next(str, bit_offset);
		size_t next = utf_sv_next(sv, bit_offset);
		uint32_t codepoint = 0;
		size_t size = 1;
		utf8_to_32(ptr + bit_offset, next - bit_offset, &codepoint, &size);
		
		if (!TTF_GlyphIsProvided32(font, codepoint)) {
			utf_append_sv(copy, replacement);
		}
		else {
			UTFStringView character = utf_sv_sub_sv(sv, i, i + 1);
			utf_append_sv(copy, character);
		}
		bit_offset = next;
	}

	return copy;
}

void get_cursor_screen_pos(TextBox* box, int* cursor_x, int* cursor_y)
{
	TextLine* cursor_line = get_line_from_line_number(box, box->cursor.line_number);

	size_t cursor_char_x, cursor_char_y;

	get_char_coord_from_cursor(box, box->cursor, &cursor_char_x, &cursor_char_y);

	int offset_y = 0;
	size_t line_number = box->cursor.line_number;

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

	offset_y += font_height * cursor_char_y;

	UTFStringView sv = utf_sv_sub_str((cursor_line->str), 0, box->cursor.char_offset);

	for (size_t i = 0; i < min(cursor_line->wrapped_line_count, cursor_char_y); i++) {
		sv = utf_sv_trim_left(sv, cursor_line->wrapped_line_sizes[i]);
	}

	UTFString* copy = replace_missing_glyph_with_char(sv, box->font, utf_sv_from_cstr(MISSING_GLYPH));

	int measured_x = 0;
	sv_fits(utf_sv_from_str(copy), box->font, box->w, NULL, &measured_x);

	if (cursor_x) {
		*cursor_x = measured_x;
	}
	if (cursor_y) {
		*cursor_y = offset_y;
	}

	utf_destroy(copy);
}

int calculate_new_box_offset_y(TextBox* box, TextCursor cursor) {
	int font_height = TTF_FontHeight(box->font);

	int offset_y = box->offset_y;

	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y < 0) {
		offset_y = -cursor_y;
	}
	else if (cursor_y + box->offset_y > box->h - font_height) {
		offset_y = box->h - cursor_y - font_height;
	}

	return offset_y;
}

Selection set_selection_to_cursor(TextCursor cursor) {
	Selection selection = {
		.start_char = cursor.char_offset, .start_line_number = cursor.line_number,
		.end_char = cursor.char_offset, .end_line_number = cursor.line_number,
	};
	return selection;
}

Selection set_selection_end(Selection selection, TextCursor cursor) {
	selection.end_char = cursor.char_offset;
	selection.end_line_number = cursor.line_number;
	return selection;
}

bool has_selection(TextBox* box, Selection selection)
{
	bool no_selection = selection.start_line_number == selection.end_line_number &&
		selection.start_char == selection.end_char;
	no_selection = !box->is_selecting || no_selection;
	return !no_selection;
}

void text_box_handle_event(TextBox* box, OS_Event* event)
{
	OS_Keymod key_mode =  os_get_mod_state();

	bool holding_shift = key_mode & OS_KMOD_SHIFT;
	bool holding_ctrl = key_mode & OS_KMOD_CTRL;

	//if holding shift and text box is not selecting
	//then begin selection
	if (holding_shift && !box->is_selecting) {
		box->selection = set_selection_to_cursor(box->cursor);
		box->is_selecting = true;
	}

	bool have_selection = has_selection(box,box->selection);

	switch (event->type) {
        case OS_TEXT_INPUT_EVENT: {
            if (have_selection) {
                box->cursor = text_box_delete_range(box, box->selection);
            }
            box->cursor = text_box_type(box, box->cursor, event->text_input_event.text_sv);
            box->selection = set_selection_to_cursor(box->cursor);
            if (!holding_shift) {
                box->is_selecting = false;
            }
            box->offset_y = calculate_new_box_offset_y(box, box->cursor);
        }break;

        case OS_KEY_PRESS_EVENT: {
            OS_KeySym key = event->keyboard_event.key_sym;
            switch (key) {
				//NOTE: previously we handled new line enter key in here but maybe we should handle it in text input event
                case OS_KEY_ENTER: {
                    if (have_selection) {
                        box->cursor = text_box_delete_range(box, box->selection);
                    }
                    //box->cursor = text_box_type(box, box->cursor, utf_sv_from_cstr("\n"));
					box->cursor = text_box_type(box, box->cursor, utf_sv_from_cstr("\r\n"));
                    box->selection = set_selection_to_cursor(box->cursor);
                    if (!holding_shift) {
                        box->is_selecting = false;
                    }
                    box->offset_y = calculate_new_box_offset_y(box, box->cursor);
                }break;
                case OS_KEY_LEFT: {
                    if (holding_shift) {
                        if(holding_ctrl){
                            box->cursor = text_box_move_cursor_left_word(box, box->cursor);
                        }
                        else{
                            box->cursor = text_box_move_cursor_left(box, box->cursor);
                        }
                        box->selection = set_selection_end(box->selection, box->cursor);
                    }
                    else {
                        if (!have_selection) {
                            if(holding_ctrl){
                                box->cursor = text_box_move_cursor_left_word(box, box->cursor);
                            }
                            else{
                                box->cursor = text_box_move_cursor_left(box, box->cursor);
                            }
                        }
                        box->is_selecting = false;
                        box->need_to_render = true;
                    }

                    box->offset_y = calculate_new_box_offset_y(box, box->cursor);
                }break;
                case OS_KEY_RIGHT: {
                    if (holding_shift) {
                        if(holding_ctrl){
                            box->cursor = text_box_move_cursor_right_word(box, box->cursor);
                        }
                        else{
                            box->cursor = text_box_move_cursor_right(box, box->cursor);
                        }
                        box->selection = set_selection_end(box->selection, box->cursor);
                    }
                    else {
                        if (!have_selection) {
                            if(holding_ctrl){
                                box->cursor = text_box_move_cursor_right_word(box, box->cursor);
                            }
                            else{
                                box->cursor = text_box_move_cursor_right(box, box->cursor);
                            }
                        }
                        box->is_selecting = false;
                        box->need_to_render = true;
                    }

                    box->offset_y = calculate_new_box_offset_y(box, box->cursor);
                }break;
                case OS_KEY_UP: {
                    box->cursor = text_box_move_cursor_up(box, box->cursor);
                    if (holding_shift) {
                        box->selection = set_selection_end(box->selection, box->cursor);
                    }
                    else {
                        box->is_selecting = false;
                    }

                    box->offset_y = calculate_new_box_offset_y(box, box->cursor);
                }break;
                case OS_KEY_DOWN: {
                    box->cursor = text_box_move_cursor_down(box, box->cursor);
                    if (holding_shift) {
                        box->selection = set_selection_end(box->selection, box->cursor);
                    }
                    else {
                        box->is_selecting = false;
                    }

                    box->offset_y = calculate_new_box_offset_y(box, box->cursor);
                }break;
                case OS_KEY_BACKSPACE: {
                    if (have_selection) {
                        box->cursor = text_box_delete_range(box, box->selection);
                    }
                    else{
                        box->cursor = text_box_delete_a_character(box, box->cursor);
                    }
                    if (holding_shift) {
                        box->selection = set_selection_to_cursor(box->cursor);
                    }
                    else {
                        box->is_selecting = false;
                    }
                    box->offset_y = calculate_new_box_offset_y(box, box->cursor);
                }break;

                case OS_KEY_F1: {
                    box->cursor = text_box_type(box, box->cursor, utf_sv_from_cstr(TEST_TEXT_ENGLISH));
                    box->offset_y = calculate_new_box_offset_y(box, box->cursor);
                }break;
                case OS_KEY_F2: {
                    box->cursor = text_box_type(box, box->cursor, utf_sv_from_cstr(TEST_TEXT_KOREAN));
                    box->offset_y = calculate_new_box_offset_y(box, box->cursor);
                }break;

                case OS_KEY_F3: {
					if(have_selection){
						UTFString *tmp = text_box_get_selection_str(box, box->selection);
						printf("--- selection str ---\n");
						utf_sv_fprintln(utf_sv_from_str(tmp), stdout);
						printf("--- selection str ---\n");
						utf_destroy(tmp);
					}
                }break;

                //handle ctrl v event
                case OS_KEY_v:
                case OS_KEY_V: {
                    if(holding_ctrl){
                        os_request_text_paste();
                    }
                }break;

                //handle ctrl c event
                case OS_KEY_c:
                case OS_KEY_C: {
                    if(holding_ctrl){
						if(have_selection){
							UTFString* tmp = text_box_get_selection_str(box, box->selection);
							os_set_clipboard_text(utf_sv_from_str(tmp));
							utf_destroy(tmp);
						}
						else{
							os_set_clipboard_text(utf_sv_from_cstr(u8""));
						}
                    }
                }break;
				default : { /*pass*/ } break;
            }
        }break;
        case OS_TEXT_PASTE_EVENT:
        {
            if (have_selection) {
                box->cursor = text_box_delete_range(box, box->selection);
            }
            box->cursor = text_box_type(box, box->cursor, event->text_paste_event.paste_sv);
            box->selection = set_selection_to_cursor(box->cursor);
            if (!holding_shift) {
                box->is_selecting = false;
            }
            box->offset_y = calculate_new_box_offset_y(box, box->cursor);
        }break;
		default : { /*pass*/ } break;
	}

	//update cursor pos
	int cursor_pos_x, cursor_pos_y;
	get_cursor_screen_pos(box, &cursor_pos_x, &cursor_pos_y);
	int font_height = TTF_FontHeight(box->font);
	os_set_ime_preedit_pos(cursor_pos_x, cursor_pos_y + font_height + box->offset_y);
}

void update_text_line(TextBox* box, TextLine* line)
{
	UTFString* copy = replace_missing_glyph_with_char(utf_sv_from_str(line->str), box->font, utf_sv_from_cstr(MISSING_GLYPH));

	UTFStringView sv = utf_sv_from_str(copy);
	int font_height = TTF_FontHeight(box->font);

	if (sv.count == 0) {
		line->size_y = font_height;
		line->size_x = box->w;
		line->wrapped_line_count = 1;
		line->wrapped_line_sizes[0] = 0;
		utf_destroy(copy);
		return;
	}

	line->size_x = box->w;

	size_t size_offset = 0;
	line->wrapped_line_count = 0;
	line->size_y = 0;

	while (true) {
		size_t measured_count = 0;
		bool fits = sv_fits(sv, box->font, box->w, &measured_count, NULL);

		//this is for the special case where font is so large that
		//it couldn't fit even a single character...
		if (sv.count > 0 && measured_count == 0) {
			break;
		}

		line->wrapped_line_sizes[size_offset++] = measured_count;
		line->wrapped_line_count++;
		line->size_y += font_height;
		sv = utf_sv_trim_left(sv, measured_count);
		if (fits) {
			break;
		}
	}

	utf_destroy(copy);

}

TextBox* text_box_create(
	const char* text,
	size_t w, size_t h,
	TTF_Font* font,
	SDL_Color bg_color, SDL_Color text_color, SDL_Color selection_bg, SDL_Color selection_fg, SDL_Color cursor_color,
	PreeditPosSetter pos_setter
	)
{
	TextBox* box = malloc(sizeof(TextBox));
	box->w = w;
	box->h = h;

	box->font = font;

	box->offset_y = 0;

	box->cursor.char_offset = 0;
	box->cursor.line_number = 0;
	box->cursor.place_after_last_char_before_wrapping = false;

	box->render_surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
	if (box->render_surface == NULL) {
		fprintf(stderr, "%s:%d:ERROR : Failed to create a render_surface for text box\n", __FILE__, __LINE__);
		return NULL;
	}

	box->first_line = create_lines_from_cstr(text);

	//calculate line pixel width and height
	for (TextLine* line = box->first_line; line != NULL; line = line->next) {
		update_text_line(box, line);
	}

	box->selection.start_char = 0;
	box->selection.end_char = 0;

	box->selection.start_line_number = 0;
	box->selection.end_line_number = 0;

	box->is_selecting = false;

	box->bg_color = bg_color;
	box->text_color = text_color;
	box->selection_bg = selection_bg;
	box->selection_fg = selection_fg;
	box->cursor_color = cursor_color;

	box->need_to_render = true;

	box->composite_str = utf_from_cstr(u8"");

	box->preedit_pos_setter = pos_setter;

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

	if(box->composite_str){
        utf_destroy(box->composite_str);
	}

	if (box->render_surface)
		SDL_FreeSurface(box->render_surface);

	free(box);
}

TextCursor text_box_type(TextBox* box, TextCursor cursor, UTFStringView sv)
{
	TextLine* cursor_line = get_line_from_line_number(box, cursor.line_number);
	TextCursor new_cursor_pos = cursor;

	int new_line_index = utf_sv_find(sv, utf_sv_from_cstr(u8"\n"));
	bool has_new_line = new_line_index >= 0;

	if (!has_new_line) {
		size_t char_offset = cursor.char_offset;
		utf_insert_sv(cursor_line->str, char_offset, sv);
		update_text_line(box, cursor_line);
		new_cursor_pos.char_offset = char_offset + sv.count;
	}
	else {
		size_t afer_new_line_start = new_line_index + 1;
		//check if new_line_index is actuall crlf
		//and if it is move it left
		if (new_line_index >= 1) {
			if (utf_sv_cmp(utf_sv_sub_sv(sv, new_line_index - 1, new_line_index), utf_sv_from_cstr(u8"\r"))) {
				new_line_index--;
				afer_new_line_start = new_line_index + 2;
			}
		}

		//divide typed lines to before new line and after new line
		UTFStringView before_new_line = utf_sv_sub_sv(sv, 0, new_line_index);
		UTFStringView after_new_line = utf_sv_sub_sv(sv, afer_new_line_start, sv.count);

		//create new sub string from original string that starts from insertion point
		size_t insertion_point = cursor.char_offset;
		UTFString* after_insertion = utf_sub_str(cursor_line->str, insertion_point, cursor_line->str->count);

		//trim current line to where insertion happens
		utf_erase_right(cursor_line->str, cursor_line->str->count - insertion_point);
		//append before new line to current line
		utf_append_sv(cursor_line->str, before_new_line);

		size_t cursor_char_pos = 0;

		//create new text lines from after new line
		TextLine* new_lines;
		TextLine* new_lines_last;

		if (after_new_line.count == 0) {
			//if after_new_line is empty then new_lines is just after insertion
			cursor_char_pos = 0;
			
			//new lines follows inserted cursor line's new line ending
			//NOTE : Maybe in this case it should follow os's convention
			new_lines = text_line_create(after_insertion, 0, cursor_line->ends_with_lf, cursor_line->ends_with_crlf);
			new_lines_last = new_lines;
		}
		else {
			//else we create new lines from after_new_line and append after_insertion at the end
			new_lines = create_lines_from_sv(after_new_line);
			//text_line_push_back(new_lines, text_line_create(after_insertion, 0));
			new_lines_last = text_line_last(new_lines);
			cursor_char_pos = new_lines_last->str->count;
			utf_append_cstr(new_lines_last->str, after_insertion->data);
			utf_destroy(after_insertion);

			//new lines follows inserted cursor line's new line ending
			new_lines_last->ends_with_crlf = cursor_line->ends_with_crlf;
			new_lines_last->ends_with_lf = cursor_line->ends_with_lf;
		}
		
		//update new lines
		for (TextLine* line = new_lines; line != NULL; line = line->next) {
			update_text_line(box, line);
		}
		//insert new lines after current line
		text_line_insert_right(cursor_line, new_lines);
		text_line_set_number_right(cursor_line, cursor_line->line_number);
		//update current line
		update_text_line(box, cursor_line);

		//calulate cursor pos
		new_cursor_pos.line_number = new_lines_last->line_number;
		new_cursor_pos.char_offset = cursor_char_pos;
	}

	int font_height = TTF_FontHeight(box->font);

	int cursor_x = 0;
	int cursor_y = 0;
	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	if (cursor_y + box->offset_y > box->h - font_height) {
		box->offset_y = box->h - cursor_y - font_height;
	}

	box->need_to_render = true;

	return new_cursor_pos;
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
	UTFString *tmp = utf_from_sv(sv);

	if (!shaded) {
		//surface = TTF_RenderUTF8_LCD(box->font, sv.data, text_color, box->bg_color);
		surface = TTF_RenderUTF8_Shaded(box->font, tmp->data, text_color, box->bg_color);
	}
	else {
		//surface = TTF_RenderUTF8_LCD(box->font, sv.data, fg_color, bg_color);
		surface = TTF_RenderUTF8_Shaded(box->font, tmp->data, fg_color, bg_color);
	}

	utf_destroy(tmp);

	if (!surface) {
		fprintf(stderr, "%s:%d:ERROR : Failed to create a surface for text box\n", __FILE__, __LINE__);
		fprintf(stderr, "%s\n", SDL_GetError());
		return false;
	}

	SDL_Rect line_rect = { .x = pos_x, .y = pos_y, .w = surface->w, .h = surface->h };

	//SDL_RenderCopy(box->renderer, texture, NULL, &line_rect);
	SDL_BlitSurface(surface, NULL, box->render_surface, &line_rect);

	if (rendered_size_x) {
		*rendered_size_x = surface->w;
	}
	if (rendered_size_y) {
		*rendered_size_y = surface->h;
	}

	SDL_FreeSurface(surface);

	return true;
}

Selection normalize_selection(Selection selection)
{
	Selection to_return = selection;

	if (selection.start_line_number == selection.end_line_number) {
		if (selection.start_char > selection.end_char) {
			to_return.end_char = selection.start_char;
			to_return.start_char = selection.end_char;
		}
	}
	else {
		if (selection.start_line_number > selection.end_line_number) {
			to_return.start_line_number = selection.end_line_number;
			to_return.start_char = selection.end_char;

			to_return.end_line_number = selection.start_line_number;
			to_return.end_char = selection.start_char;
		}
	}

	return to_return;
}


void text_box_render(TextBox* box) {
	if (!box->need_to_render) {
		return;
	}
	//Clear render surface with bg color
	SDL_FillRect(box->render_surface, NULL,
		SDL_MapRGBA(box->render_surface->format, box->bg_color.r, box->bg_color.g, box->bg_color.b, box->bg_color.a)) ;

	int font_height = TTF_FontHeight(box->font);

	bool no_selection = !box->is_selecting;
	no_selection = no_selection ||
		(box->selection.start_line_number == box->selection.end_line_number &&
		box->selection.start_char == box->selection.end_char);

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
					line->line_number > selection.start_line_number &&
					line->line_number < selection.end_line_number
					);
				partially_inside_selection = (
					line->line_number == selection.start_line_number ||
					line->line_number == selection.end_line_number
					);
				outside_selecton = !completely_inside_selection && !partially_inside_selection;
			}

			UTFString* copy = replace_missing_glyph_with_char(utf_sv_from_str(line->str), box->font, utf_sv_from_cstr(MISSING_GLYPH));

			if (outside_selecton || completely_inside_selection) {
				UTFStringView sv = utf_sv_from_str(copy);

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
				UTFStringView sv = utf_sv_from_str(copy);

				size_t char_offset = 0;

				for (size_t i = 0; i < line->wrapped_line_count; i++) {
					if (pixel_offset_y > box->h) {
						goto text_render_end;
					}
					size_t line_start = char_offset;
					size_t line_end = line->wrapped_line_sizes[i] + char_offset;

					UTFStringView line_sv = utf_sv_sub_sv(sv, line_start, line_end);

					bool left_shaded = selection.end_char <= line_end && selection.end_char >= line_start;
					left_shaded = left_shaded && (selection.end_line_number == line->line_number);

					bool right_shaded = selection.start_char <= line_end && selection.start_char >= line_start;
					right_shaded = right_shaded && (selection.start_line_number == line->line_number);

					bool un_shaded = line->line_number == selection.start_line_number && line_end < selection.start_char && line_start < selection.start_char;
					un_shaded = un_shaded || (line->line_number == selection.end_line_number && line_start > selection.end_char && line_end > selection.end_char);


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

			utf_destroy(copy);

		}
	}
text_render_end: ;

	/////////////////////////////
	// Render Cursor
	/////////////////////////////
	int cursor_x = 0;
	int cursor_y = 0;

	get_cursor_screen_pos(box, &cursor_x, &cursor_y);
	SDL_Rect cursor_rect = { .x = cursor_x, .y = cursor_y + 2 + box->offset_y, .w = 2, .h = font_height - 2 };

	SDL_FillRect(box->render_surface, &cursor_rect,
        SDL_MapRGBA(box->render_surface->format, box->cursor_color.r, box->cursor_color.g, box->cursor_color.b, box->cursor_color.a));

renderexit:

	box->need_to_render = false;
}

TextCursor text_box_move_cursor_left(TextBox* box, TextCursor cursor)
{
	TextCursor new_cursor_pos = cursor;

	new_cursor_pos.place_after_last_char_before_wrapping = false;
	if (cursor.char_offset > 0) {
		new_cursor_pos.char_offset--;
	}
	else {
		TextLine* cursor_line = get_line_from_line_number(box, cursor.line_number);
		TextLine* prev_line = cursor_line->prev;
		if (prev_line) {
			new_cursor_pos.char_offset = prev_line->str->count;
			new_cursor_pos.line_number--;
		}
	}

	box->need_to_render = true;

	return new_cursor_pos;
}

TextCursor text_box_move_cursor_right(TextBox* box, TextCursor cursor)
{
	TextCursor new_cursor_pos = cursor;

	new_cursor_pos.place_after_last_char_before_wrapping = false;
	TextLine* cursor_line = get_line_from_line_number(box, cursor.line_number);

	if (cursor.char_offset < cursor_line->str->count) {
		new_cursor_pos.char_offset++;
	}
	else {
		TextLine* next_line = cursor_line->next;
		if (next_line) {
			new_cursor_pos.char_offset = 0;
			new_cursor_pos.line_number++;
		}
	}

	box->need_to_render = true;

	return new_cursor_pos;
}

size_t get_char_offset_from_line_and_char_coord(TextLine* line, size_t char_x, size_t char_y) {
	size_t char_offset = 0;
	for (size_t i = 0; i < min(line->wrapped_line_count, char_y); i++) {
		char_offset += line->wrapped_line_sizes[i];
	}
	size_t cursor_line_index = min(line->wrapped_line_count-1, char_y);
	size_t cursor_line_size = min((int)char_x, line->wrapped_line_sizes[cursor_line_index]);
	char_offset += cursor_line_size;
	return char_offset;
}

TextCursor text_box_move_cursor_up(TextBox* box, TextCursor cursor)
{
	TextCursor new_cursor_pos = cursor;
	if (cursor.char_offset == 0 && cursor.line_number == 0) {
		return new_cursor_pos;
	}
	//box->cursor.place_after_last_char_before_wrapping = true;

	TextLine* cursor_line = get_line_from_line_number(box, cursor.line_number);
	size_t offset_x;
	size_t offset_y;
	get_char_coord_from_cursor(box, cursor, &offset_x, &offset_y);

	if (offset_y == 0) {
		TextLine* prev_line = cursor_line->prev;
		if (prev_line) {
			new_cursor_pos.char_offset = get_char_offset_from_line_and_char_coord(
				prev_line,
				offset_x,
				prev_line->wrapped_line_count - 1
			);
			new_cursor_pos.line_number--;
		}
	}
	else {
		if (cursor_line->wrapped_line_sizes[offset_y] == (int)offset_x) {
			new_cursor_pos.place_after_last_char_before_wrapping = true;
		}
		else {
			new_cursor_pos.place_after_last_char_before_wrapping = false;
		}
		new_cursor_pos.char_offset = get_char_offset_from_line_and_char_coord(
			cursor_line,
			offset_x,
			offset_y-1
		);
	}

	box->need_to_render = true;

	return new_cursor_pos;
}

TextCursor text_box_move_cursor_down(TextBox* box, TextCursor cursor)
{
	TextCursor new_cursor_pos = cursor;

	TextLine* cursor_line = get_line_from_line_number(box, cursor.line_number);

	size_t offset_x;
	size_t offset_y;
	get_char_coord_from_cursor(box, cursor, &offset_x, &offset_y);

	if (offset_x == 0) {
		new_cursor_pos.place_after_last_char_before_wrapping = false;
	}

	if (offset_y + 1 < cursor_line->wrapped_line_count) {

		new_cursor_pos.char_offset = get_char_offset_from_line_and_char_coord(
			cursor_line,
			offset_x,
			offset_y + 1
		);
	}
	else if(cursor_line->next != NULL) {
		new_cursor_pos.char_offset = get_char_offset_from_line_and_char_coord(
			cursor_line->next,
			offset_x,
			0
		);
		new_cursor_pos.line_number++;
	}

	box->need_to_render = true;

	return new_cursor_pos;
}

TextCursor text_box_move_cursor_left_word(TextBox* box, TextCursor cursor)
{
    //first check if we can go left
    if(cursor.line_number <= 0 && cursor.char_offset <= 0){
        return cursor;
    }

    box->need_to_render = true;

    while(true){
        cursor = text_box_move_cursor_left(box, cursor);
        //if we reached the beginnig of the string return
        if(cursor.char_offset <= 0){
            return cursor;
        }

        TextLine* cursor_line = get_line_from_line_number(box, cursor.line_number);

        //if we reached the end of the string return
        if(cursor.char_offset == cursor_line->str->count){
            return cursor;
        }

        //if there is a space at left return
        UTFStringView sv = utf_sv_from_str(cursor_line->str);
        UTFStringView at_left = utf_sv_sub_sv(sv, cursor.char_offset - 1 , cursor.char_offset);
        UTFStringView at_right = utf_sv_sub_sv(sv, cursor.char_offset, cursor.char_offset+1);
        if( utf_sv_cmp(at_left, utf_sv_from_cstr(u8" ")) && !utf_sv_cmp(at_right, utf_sv_from_cstr(u8" ")) )
        {
            return cursor;
        }
    }
}
TextCursor text_box_move_cursor_right_word(TextBox* box, TextCursor cursor)
{
    TextLine* cursor_line = get_line_from_line_number(box, cursor.line_number);

    //first check if we can go right
    if(cursor.char_offset >= cursor_line->str->count && cursor_line->next == NULL){
        return cursor;
    }

    box->need_to_render = true;

    while(true){
        cursor = text_box_move_cursor_right(box, cursor);

        //if we reached the beginnig of the string return
        if(cursor.char_offset <= 0){
            return cursor;
        }

        TextLine* cursor_line = get_line_from_line_number(box, cursor.line_number);

        //if we reached the end of the string return
        if(cursor.char_offset == cursor_line->str->count){
            return cursor;
        }

        //if there is a space at left return
        UTFStringView sv = utf_sv_from_str(cursor_line->str);
        UTFStringView at_left = utf_sv_sub_sv(sv, cursor.char_offset - 1 , cursor.char_offset);
        UTFStringView at_right = utf_sv_sub_sv(sv, cursor.char_offset, cursor.char_offset+1);
        if( utf_sv_cmp(at_left, utf_sv_from_cstr(u8" ")) && !utf_sv_cmp(at_right, utf_sv_from_cstr(u8" ")) )
        {
            return cursor;
        }
    }
}

TextCursor text_box_delete_a_character(TextBox* box, TextCursor cursor)
{
	TextCursor new_cursor_pos = cursor;

	if (cursor.line_number == 0 && cursor.char_offset == 0) {
		return new_cursor_pos;
	}

	size_t char_offset = cursor.char_offset;
	if (char_offset <= 0) {
		if (cursor.line_number == 0) {
			return new_cursor_pos;
		}
		TextLine* cursor_line = get_line_from_line_number(box, cursor.line_number);
		if (cursor_line->prev == NULL) {
			return new_cursor_pos;
		}
		TextLine* prev_line = cursor_line->prev;
		size_t line_count = prev_line->str->count;
		utf_append_str(prev_line->str, cursor_line->str);

		prev_line->next = cursor_line->next;
		if (cursor_line->next) {
			cursor_line->next->prev = prev_line;
		}
		text_line_destroy(cursor_line);
		new_cursor_pos.line_number--;

		text_line_set_number_right(prev_line, prev_line->line_number);
		update_text_line(box, prev_line);

		new_cursor_pos.char_offset = line_count;
	}
	else {
		TextLine* cursor_line = get_line_from_line_number(box, cursor.line_number);
		utf_erase_range(cursor_line->str, char_offset - 1, char_offset);
		update_text_line(box, cursor_line);
		new_cursor_pos.char_offset = char_offset - 1;
	}

	box->need_to_render = true;

	return new_cursor_pos;
}

TextCursor text_box_delete_range(TextBox* box, Selection selection)
{
	TextCursor new_cursor_pos = box->cursor;

	selection = normalize_selection(selection);

	TextLine* start_line = get_line_from_line_number(box, selection.start_line_number);
	TextLine* end_line = get_line_from_line_number(box, selection.end_line_number);

	if (selection.start_line_number == selection.end_line_number) {
		utf_erase_range(start_line->str, selection.start_char, selection.end_char);
		new_cursor_pos.char_offset = selection.start_char;
		update_text_line(box, start_line);
		box->need_to_render = true;
	}
	else {
		//first get texts after selection end char
		UTFString *start_str = start_line->str;
		UTFString *end_str = end_line->str;
		UTFStringView after_selection_end_char = utf_sv_sub_str(end_str, selection.end_char, end_str->count);

		//erase start_str after selection start char
		utf_erase_range(start_str, selection.start_char, start_str->count);
		//append after_selection_end_char
		utf_append_sv(start_str, after_selection_end_char);

		//free lines between start and end
		assert(start_line->next != NULL);
		for (TextLine* line = start_line->next; line != end_line;) {
			TextLine* tmp = line->next;
			text_line_destroy(line);
			line = tmp;
		}
		start_line->next = end_line->next;
		if (end_line->next) {
			end_line->next->prev = start_line;
		}
		text_line_destroy(end_line);

		update_text_line(box, start_line);
		new_cursor_pos.line_number = start_line->line_number;
		new_cursor_pos.char_offset = selection.start_char;

		text_line_set_number_right(start_line, start_line->line_number);
	}
	box->need_to_render = true;

	return new_cursor_pos;
}

UTFString* text_box_get_selection_str(TextBox* box, Selection selection)
{
	selection = normalize_selection(selection);

	//if selection has the same text line at beginning and at the end
	//just create str from sub sv	
	if(selection.start_line_number == selection.end_line_number){
		TextLine* line = get_line_from_line_number(box, selection.start_line_number);
		UTFString* str = utf_from_sv(utf_sv_sub_str(line->str, selection.start_char, selection.end_char)); 
		return str;
	}

	TextLine* start_line = get_line_from_line_number(box, selection.start_line_number); 
	TextLine* end_line = get_line_from_line_number(box, selection.end_line_number); 
	
	UTFString* str = utf_from_sv(utf_sv_sub_str(start_line->str, selection.start_char, start_line->str->count));

	//TODO : Implement some sort of mechanic to differentiate between crlf and lf
	utf_append_cstr(str, u8"\n");

	for(TextLine* line = start_line->next; line != NULL && line != end_line; line = line->next){
		utf_append_str(str, line->str);
		//TODO : Implement some sort of mechanic to differentiate between crlf and lf
		utf_append_cstr(str, u8"\n");
	}

	utf_append_sv(str, utf_sv_sub_str(end_line->str, 0, selection.end_char));  

	return str;
}

void text_box_resize(TextBox* box, int w, int h)
{
	SDL_Surface *new_render_surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, box->render_surface->format->format);

	if (!new_render_surface) {
		fprintf(stderr, "%s:%d:ERROR : Failed to resize text box!!! : %s", __FILE__, __LINE__, SDL_GetError());
		return;
	}
	SDL_FreeSurface(box->render_surface);
	box->render_surface = new_render_surface;

	box->w = w;
	box->h = h;

	for (TextLine* line = box->first_line; line != NULL; line = line->next) {
		update_text_line(box, line);
	}

	box->offset_y = calculate_new_box_offset_y(box, box->cursor);
	box->need_to_render = true;
}
