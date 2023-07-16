#include "songmessage.h"

#include "gui/msgbox.h"
#include "gui/bevel.h"
#include "gui/bevdefs.h"
#include "gui/dialog.h"
#include "gfx/gfx.h"
#include "gui/view.h"
#include "gui/mouse.h"
#include "gui/toolutil.h"
#include "mybevdefs.h"
#include <string.h>

#define SCROLLBAR 10
#define TOP_LEFT 0
#define TOP_RIGHT 0
#define MARGIN 8
#define SCREENMARGIN 8 //#define SCREENMARGIN 32
#define TITLE 14
#define FIELD 14
#define CLOSE_BUTTON 12
#define ELEMWIDTH msg_data.elemwidth
#define LIST_WIDTH msg_data.list_width
#define BUTTONS 16

extern Mused mused;
extern GfxDomain *domain;

typedef struct
{
	int selected_line;
	const char *title;
	SliderParam scrollbar;
	char **lines;
	int n_lines;
	int list_position;
	int horiz_position;
	int quit;
	GfxSurface *gfx;
	int elemwidth, list_width;
	int chars_per_line;

	Uint32* line_start_points; //needed to correctly determine where to edit source string
	Uint32* line_lengths;

	bool editing;
} MessageData;

MessageData msg_data;

static void message_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);
static void title_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);
static void window_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);
static void buttons_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);

static void close_msg_editor(void* unused1, void* unused2, void* unused3);
static void copy_msg_editor(void* unused1, void* unused2, void* unused3);
static void paste_msg_editor(void* unused1, void* unused2, void* unused3);
static void clear_msg_editor(void* unused1, void* unused2, void* unused3);

static const View messagebox_view[] =
{
	{{ SCREENMARGIN, SCREENMARGIN, -SCREENMARGIN, -SCREENMARGIN }, window_view, &msg_data, -1},
	{{ MARGIN+SCREENMARGIN, SCREENMARGIN+MARGIN, -MARGIN-SCREENMARGIN, TITLE - 2 }, title_view, &msg_data, -1},
	{{ -SCROLLBAR-MARGIN-SCREENMARGIN, SCREENMARGIN+MARGIN + TITLE, SCROLLBAR, -MARGIN-SCREENMARGIN-BUTTONS }, slider, &msg_data.scrollbar, -1},
	{{ SCREENMARGIN+MARGIN, SCREENMARGIN+MARGIN + TITLE, -SCROLLBAR-MARGIN-1-SCREENMARGIN, -MARGIN-SCREENMARGIN-BUTTONS }, message_view, &msg_data, -1},
	{{ SCREENMARGIN+MARGIN, -SCREENMARGIN-MARGIN-BUTTONS+2, -MARGIN-SCREENMARGIN, BUTTONS-2 }, buttons_view, &msg_data, -1},
	{{0, 0, 0, 0}, NULL}
};

static int checkbox_simple(GfxDomain *dest, const SDL_Event *event, const SDL_Rect *area, GfxSurface *gfx, const Font * font, int offset, int offset_pressed, int decal, const char* _label, Uint32 *flags, Uint32 mask)
{
	SDL_Rect tick, label;
	copy_rect(&tick, area);
	copy_rect(&label, area);
	tick.h = tick.w = 8;
	label.w -= tick.w + 4;
	label.x += tick.w + 4;
	label.y += 1;
	label.h -= 1;
	int pressed = button_event(dest, event, &tick, gfx, offset, offset_pressed, (*flags & mask) ? decal : -1, NULL, 0, 0, 0);
	font_write(font, dest, &label, _label);
	
	return pressed;
}

static void generic_flags_simple(const SDL_Event *e, const SDL_Rect *_area, const char *label, Uint32 *_flags, Uint32 mask)
{
	SDL_Rect area;
	copy_rect(&area, _area);
	area.y += 1;

	int hit = check_event(e, _area, NULL, NULL, NULL, NULL);

	if (checkbox_simple(domain, e, &area, mused.slider_bevel, &mused.smallfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, DECAL_TICK, label, _flags, mask))
	{

	}
	
	if (hit)
	{
		// so that the gap between the box and label works too
		Uint32 flags = *_flags;
		flags ^= mask;
		*_flags = flags;
	}
}

static void buttons_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	SDL_Rect button;
	
	copy_rect(&button, area);
	
	button.w = strlen("OK") * mused.largefont.w + 12;
	
	button.x = area->x + area->w - button.w;
	button_text_event(dest_surface, event, &button, msg_data.gfx, &mused.largefont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "OK", close_msg_editor, 0, 0, 0);

	button.x -= button.w / 2;

	button.w = strlen("Paste") * mused.largefont.w + 12;
	button.x -= button.w + 1;
	button_text_event(dest_surface, event, &button, msg_data.gfx, &mused.largefont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "Paste", paste_msg_editor, 0, 0, 0);

	button.w = strlen("Copy") * mused.largefont.w + 12;
	button.x -= button.w + 1;
	button_text_event(dest_surface, event, &button, msg_data.gfx, &mused.largefont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "Copy", copy_msg_editor, 0, 0, 0);

	button.w = strlen("Clear") * mused.largefont.w + 12;
	button.x -= button.w + 1;
	button_text_event(dest_surface, event, &button, msg_data.gfx, &mused.largefont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "Clear", clear_msg_editor, 0, 0, 0);

	button.w = strlen("SHOW WHEN SONG LOADS") * mused.smallfont.w + 12;
	button.x -= button.w + 10;
	button.y += 2;
	generic_flags_simple(event, &button, "SHOW WHEN SONG LOADS", &mused.song.flags, MUS_SHOW_MESSAGE);
}

static void window_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	bevel(dest_surface, area, msg_data.gfx, BEV_MENU);
}


static void title_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	const char* title = msg_data.title;
	SDL_Rect titlearea, button;
	copy_rect(&titlearea, area);
	titlearea.w -= CLOSE_BUTTON - 4;
	copy_rect(&button, area);
	adjust_rect(&button, titlearea.h - CLOSE_BUTTON);
	button.w = CLOSE_BUTTON;
	button.x = area->w + area->x - CLOSE_BUTTON;
	font_write(&mused.largefont, dest_surface, &titlearea, title);
	if (button_event(dest_surface, event, &button, msg_data.gfx, BEV_BUTTON, BEV_BUTTON_ACTIVE, DECAL_CLOSE, NULL, MAKEPTR(1), 0, 0) & 1)
		msg_data.quit = 1;
}


static void message_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	SDL_Rect content, pos;
	copy_rect(&content, area);
	adjust_rect(&content, 1);
	copy_rect(&pos, &content);
	pos.h = mused.largefont.h + 1;
	bevel(dest_surface, area, msg_data.gfx, BEV_FIELD);
	
	//gfx_domain_set_clip(dest_surface, &content);
	//gfx_domain_set_clip(dest_surface, NULL);
	
	for (int i = msg_data.list_position; i < msg_data.n_lines && pos.y < content.h + content.y; ++i)
	{
		if (msg_data.selected_line == i)
		{
			bevel(dest_surface, &pos, msg_data.gfx, BEV_SELECTED_ROW);
		}

		if(msg_data.lines[i])
		{
			font_write(&mused.largefont, dest_surface, &pos, msg_data.lines[i]);
		}

		if (msg_data.selected_line == i)
		{
			SDL_Rect cursor;
			copy_rect(&cursor, &pos);

			cursor.h += 4;
			cursor.y -= 2;

			cursor.w = mused.largefont.w + 2 + 2;
			cursor.x = pos.x + mused.largefont.w * msg_data.horiz_position;

			cursor.x -= 2;

			bevel(dest_surface, &cursor, msg_data.gfx, msg_data.editing ? BEV_EDIT_CURSOR : BEV_CURSOR);
		}
		
		if (pos.y + pos.h <= content.h + content.y) slider_set_params(&msg_data.scrollbar, 0, msg_data.n_lines - 1, msg_data.list_position, i, &msg_data.list_position, 1, SLIDER_VERTICAL, msg_data.gfx);
		
		update_rect(&content, &pos);
	}
	
	gfx_domain_set_clip(dest_surface, NULL);
}

static void deinit_lines()
{
	if(msg_data.lines)
	{
		for (int i = 0; i < msg_data.n_lines; ++i)
		{
			if (msg_data.lines[i]) free(msg_data.lines[i]);
		}

		free(msg_data.lines);
	}

	msg_data.lines = NULL;
	msg_data.n_lines = 0;

	if(msg_data.line_start_points)
	{
		free(msg_data.line_start_points);
	}

	if(msg_data.line_lengths)
	{
		free(msg_data.line_lengths);
	}
}

static void init_lines()
{
	deinit_lines();

	char* song_msg = mused.song.song_message;

	Uint32 current_char = 0;

	Uint32 space_char = 0;

	Uint32 end_of_line_char = 0;

	Uint32 msg_len = strlen(song_msg);

	msg_data.n_lines = 0;

	msg_data.lines = (char**)calloc(1, sizeof(char*) * (msg_data.n_lines + 1));
	msg_data.line_start_points = (Uint32*)calloc(1, sizeof(Uint32) * (msg_data.n_lines + 1));
	msg_data.line_lengths = (Uint32*)calloc(1, sizeof(Uint32) * (msg_data.n_lines + 1));

	while(current_char < msg_len)
	{
		bool next_line = false;

		while(!(next_line) && current_char < msg_len)
		{
			char curr_char = song_msg[current_char];
			
			if(curr_char == ' ')
			{
				space_char = current_char; //so we can wrap string to new line where space is
			}

			if(curr_char == '\n')
			{
				next_line = true;

				if(current_char < msg_len - 1)
				{
					if(song_msg[current_char + 1] == '\r')
					{
						current_char++; //skip '\r' symbol if present
					}
				}

				goto end;
			}

			if(current_char - end_of_line_char >= msg_data.chars_per_line)
			{
				next_line = true;

				if(space_char - end_of_line_char < msg_data.chars_per_line) //so very long characters sequence without spaces is also forced to wrap (otherwise klystrack crashes)
				{
					current_char = space_char;
				}
			}

			end:;

			current_char++;
		}

		if(song_msg[end_of_line_char] == ' ')
		{
			end_of_line_char++; //so new line does not start with space char
		}

		Uint32 line_len = current_char - end_of_line_char;

		msg_data.n_lines++;
		msg_data.lines = (char**)realloc(msg_data.lines, sizeof(char*) * msg_data.n_lines);
		msg_data.line_start_points = (Uint32*)realloc(msg_data.line_start_points, sizeof(Uint32) * msg_data.n_lines);
		msg_data.line_lengths = (Uint32*)realloc(msg_data.line_lengths, sizeof(Uint32) * msg_data.n_lines);

		msg_data.lines[msg_data.n_lines - 1] = (char*)calloc(1, sizeof(char) * (line_len + 1));
		memcpy(msg_data.lines[msg_data.n_lines - 1], &song_msg[end_of_line_char], line_len * sizeof(char));

		msg_data.line_start_points[msg_data.n_lines - 1] = end_of_line_char;
		msg_data.line_lengths[msg_data.n_lines - 1] = line_len;

		for(int k = 0; k < 2; k++) //two times to get rid of '\n\r' sequence
		{
			for(int i = 0; i < line_len; i++)
			{
				if(msg_data.lines[msg_data.n_lines - 1][i] == '\n' || msg_data.lines[msg_data.n_lines - 1][i] == '\r')
				{
					//msg_data.lines[msg_data.n_lines - 1][i] = ' ';
					if(line_len > 1)
					{
						for(int j = i; j < line_len - 1; j++)
						{
							msg_data.lines[msg_data.n_lines - 1][j] = msg_data.lines[msg_data.n_lines - 1][j + 1];
						}

						line_len--;
						msg_data.line_lengths[msg_data.n_lines - 1]--;
					}

					else
					{
						msg_data.lines[msg_data.n_lines - 1][i] = ' ';
					}
				}
			}
		}

		msg_data.lines[msg_data.n_lines - 1][line_len] = '\0';

		//debug("Line: \"%s\", length: %d", msg_data.lines[msg_data.n_lines - 1], msg_data.line_lengths[msg_data.n_lines - 1]);

		end_of_line_char = current_char - 1;
	}

	if(msg_data.n_lines == 0)
	{
		msg_data.selected_line = 0;
	}

	else
	{
		if(msg_data.selected_line > msg_data.n_lines)
		{
			msg_data.selected_line = msg_data.n_lines - 1;
		}
	}

	if(msg_data.horiz_position > msg_data.line_lengths[msg_data.selected_line])
	{
		msg_data.horiz_position = msg_data.line_lengths[msg_data.selected_line];
	}
}

const char* song_message_header = "Song message";

static Uint32 get_current_message_string_position()
{
	Uint32 position = msg_data.line_start_points[msg_data.selected_line] + msg_data.horiz_position;

	if(mused.song.song_message[msg_data.line_start_points[msg_data.selected_line]] == '\n')
	{
		position++;
	}

	return position;
}

static void add_char(char ch)
{
	mused.song.song_message_length = strlen(mused.song.song_message) + 1;

	mused.song.song_message = realloc(mused.song.song_message, mused.song.song_message_length + 2);

	memmove(&mused.song.song_message[get_current_message_string_position() + 1], &mused.song.song_message[get_current_message_string_position()], mused.song.song_message_length - get_current_message_string_position());

	mused.song.song_message[get_current_message_string_position()] = ch;

	mused.song.song_message_length = strlen(mused.song.song_message) + 1;
}

void song_message_view(GfxDomain *domain, GfxSurface *gfx, const Font *largefont, const Font *smallfont)
{
	set_repeat_timer(NULL);

	if(mused.song.song_message == NULL)
	{
		mused.song.song_message = (char*)calloc(1, sizeof(char) * 2);
		strcpy(mused.song.song_message, "");
	}

	memset(&msg_data, 0, sizeof(msg_data));
	
	msg_data.title = (char*)calloc(1, sizeof(char) * strlen(song_message_header));
	strcpy(msg_data.title, song_message_header);

	msg_data.gfx = mused.slider_bevel;
	msg_data.elemwidth = domain->screen_w - SCREENMARGIN * 2 - MARGIN * 2 - 16 - 2;
	msg_data.list_width = domain->screen_w - SCREENMARGIN * 2 - MARGIN * 2 - SCROLLBAR - 2;

	msg_data.list_position = 0;

	msg_data.chars_per_line = msg_data.list_width / mused.largefont.w; //2 more for null termination and different widths on different screens thing

	msg_data.editing = false;
	
	init_lines();
	
	slider_set_params(&msg_data.scrollbar, 0, msg_data.n_lines - 1, msg_data.list_position, 0, &msg_data.list_position, 1, SLIDER_VERTICAL, msg_data.gfx);

	while (!msg_data.quit)
	{
		SDL_Event e = { 0 };
		int got_event = 0;

		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				case SDL_QUIT:
				
				set_repeat_timer(NULL);
				SDL_PushEvent(&e);
				
				deinit_lines();
				SDL_StopTextInput();
				return;
				
				break;
				
				case SDL_WINDOWEVENT:
				{
					switch (e.window.event) 
					{
						case SDL_WINDOWEVENT_RESIZED:
						{
							debug("SDL_WINDOWEVENT_RESIZED %dx%d", e.window.data1, e.window.data2);

							domain->screen_w = my_max(320, e.window.data1 / domain->scale);
							domain->screen_h = my_max(240, e.window.data2 / domain->scale);

							if (!(mused.flags & FULLSCREEN))
							{
								mused.window_w = domain->screen_w * domain->scale;
								mused.window_h = domain->screen_h * domain->scale;
							}

							msg_data.elemwidth = domain->screen_w - SCREENMARGIN * 2 - MARGIN * 2 - 16 - 2;
							msg_data.list_width = domain->screen_w - SCREENMARGIN * 2 - MARGIN * 2 - SCROLLBAR - 2;

							msg_data.list_position = 0;

							msg_data.chars_per_line = msg_data.list_width / mused.largefont.w;
							
							init_lines();

							gfx_domain_update(domain, false);
						}
						break;
					}
					break;
				}

				case SDL_TEXTINPUT:
				{
					add_char(e.text.text[0]);

					msg_data.horiz_position++;

					init_lines();

					if(msg_data.horiz_position > msg_data.line_lengths[msg_data.selected_line] && strcmp(msg_data.lines[msg_data.selected_line], " ") != 0)
					{
						if(msg_data.selected_line < msg_data.n_lines - 1)
						{
							msg_data.selected_line++;
							msg_data.horiz_position = msg_data.line_lengths[msg_data.selected_line]; //so the cursor still is at the end of the word
						}
					}
				}
				break;

				case SDL_TEXTEDITING:
				break;
				
				case SDL_KEYDOWN:
				{
					switch (e.key.keysym.sym)
					{
						case SDLK_F11:
						
						set_repeat_timer(NULL);
						deinit_lines();
						SDL_StopTextInput();
						return;
						
						break;
						
						case SDLK_DOWN:
						slider_move_position(&msg_data.selected_line, &msg_data.list_position, &msg_data.scrollbar, 1);

						if(msg_data.horiz_position >= msg_data.line_lengths[msg_data.selected_line])
						{
							msg_data.horiz_position = msg_data.line_lengths[msg_data.selected_line] - 1;
						}
						break;
						
						case SDLK_UP:
						slider_move_position(&msg_data.selected_line, &msg_data.list_position, &msg_data.scrollbar, -1);

						if(msg_data.horiz_position >= msg_data.line_lengths[msg_data.selected_line])
						{
							msg_data.horiz_position = msg_data.line_lengths[msg_data.selected_line] - 1;
						}
						break;

						case SDLK_RIGHT:
						{
							msg_data.horiz_position++;

							if(msg_data.horiz_position > msg_data.line_lengths[msg_data.selected_line])
							{
								slider_move_position(&msg_data.selected_line, &msg_data.list_position, &msg_data.scrollbar, 1);
								msg_data.horiz_position = 0;
							}
						}
						break;
						
						case SDLK_LEFT:
						{
							msg_data.horiz_position--;

							if(msg_data.horiz_position < 0)
							{
								slider_move_position(&msg_data.selected_line, &msg_data.list_position, &msg_data.scrollbar, -1);
								msg_data.horiz_position = msg_data.line_lengths[msg_data.selected_line] - 1;
							}
						}
						break;
						
						case SDLK_PAGEUP:
						case SDLK_PAGEDOWN:
						{
							int items = msg_data.scrollbar.visible_last - msg_data.scrollbar.visible_first;
							
							if (e.key.keysym.sym == SDLK_PAGEUP)
								items = -items;
							
							slider_move_position(&msg_data.selected_line, &msg_data.list_position, &msg_data.scrollbar, items);
						}
						break;

						case SDLK_ESCAPE:
						{
							if(msg_data.editing)
							{
								SDL_StopTextInput();
								msg_data.editing = false;
							}

							else
							{
								SDL_StartTextInput();
								msg_data.editing = true;
							}
						}
						break;

						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						{
							if(msg_data.editing)
							{
								debug("get_current_message_string_position() %d mused.song.song_message_length %d", get_current_message_string_position(), mused.song.song_message_length);

								add_char('\n');

								init_lines();

								debug("a get_current_message_string_position() %d mused.song.song_message_length %d", get_current_message_string_position(), mused.song.song_message_length);

								if(get_current_message_string_position() == mused.song.song_message_length - 2)
								{
									add_char('\n');

									//msg_data.selected_line--;

									init_lines();
								}

								msg_data.horiz_position++;

								if(msg_data.horiz_position >= msg_data.line_lengths[msg_data.selected_line])
								{
									msg_data.horiz_position = 0;
									msg_data.selected_line++;
								}
							}
						}
						break;

						case SDLK_BACKSPACE:
						case SDLK_DELETE:
						{
							if(msg_data.editing)
							{
								if(e.key.keysym.sym == SDLK_BACKSPACE)
								{
									if (msg_data.horiz_position > 0) --msg_data.horiz_position;

									else
									{
										slider_move_position(&msg_data.selected_line, &msg_data.list_position, &msg_data.scrollbar, -1);
										msg_data.horiz_position = msg_data.line_lengths[msg_data.selected_line] - 1;
									}
								}

								if(e.key.keysym.sym == SDLK_DELETE)
								{
									if (msg_data.horiz_position > msg_data.line_lengths[msg_data.selected_line]) --msg_data.horiz_position;

									if(msg_data.horiz_position < 0)
									{
										slider_move_position(&msg_data.selected_line, &msg_data.list_position, &msg_data.scrollbar, -1);
										msg_data.horiz_position = msg_data.line_lengths[msg_data.selected_line] - 1;
									}
								}

								mused.song.song_message_length = strlen(mused.song.song_message) + 1;

								mused.song.song_message = realloc(mused.song.song_message, mused.song.song_message_length);

								memmove(&mused.song.song_message[get_current_message_string_position()], &mused.song.song_message[get_current_message_string_position() + 1], mused.song.song_message_length - get_current_message_string_position());
								mused.song.song_message[mused.song.song_message_length - 2] = '\0';

								mused.song.song_message_length = strlen(mused.song.song_message) + 1;

								init_lines();
							}
						}
						break;
						
						default: break;
					}
				}
				break;
			
				case SDL_USEREVENT:
					e.type = SDL_MOUSEBUTTONDOWN;
				break;
				
				case SDL_MOUSEMOTION:
					if (domain)
					{
						e.motion.xrel /= domain->scale;
						e.motion.yrel /= domain->scale;
						e.button.x /= domain->scale;
						e.button.y /= domain->scale;
					}
				break;
				
				case SDL_MOUSEBUTTONDOWN:
					if (domain)
					{
						e.button.x /= domain->scale;
						e.button.y /= domain->scale;
					}
				break;
				
				case SDL_MOUSEBUTTONUP:
				{
					if (e.button.button == SDL_BUTTON_LEFT)
						mouse_released(&e);
				}
				break;
			}
			
			if (e.type == SDL_MOUSEWHEEL)
			{
				if (e.wheel.y > 0)
				{
					msg_data.list_position -= 2;
					*(msg_data.scrollbar.position) -= 2;
				}
				
				else
				{
					msg_data.list_position += 2;
					*(msg_data.scrollbar.position) += 2;
				}
				
				msg_data.list_position = my_max(0, my_min((msg_data.n_lines - 1) - ((domain->window_h - (33 + 30)) / 18), msg_data.list_position));
				*(msg_data.scrollbar.position) = my_max(0, my_min((msg_data.n_lines - 1) - ((domain->window_h - (33 + 30)) / 18), *(msg_data.scrollbar.position)));
			}
			
			if (e.type != SDL_MOUSEMOTION || (e.motion.state)) ++got_event;
			
			// Process mouse click events immediately, and batch mouse motion events
			// (process the last one) to fix lag with high poll rate mice on Linux.
			//fix from here https://github.com/kometbomb/klystrack/pull/300
			if (should_process_mouse(&e))
				break;
		}
		
		if (got_event || gfx_domain_is_next_frame(domain))
		{
			draw_view(domain, messagebox_view, &e);
			gfx_domain_flip(domain);
		}
		else
			SDL_Delay(5);
	}
	
	deinit_lines();
	SDL_StopTextInput();

	mused.frames_since_menu_close = 0;
}

static void close_msg_editor(void* unused1, void* unused2, void* unused3)
{
	msg_data.quit = 1;
}

static void copy_msg_editor(void* unused1, void* unused2, void* unused3)
{
	SDL_SetClipboardText(mused.song.song_message);
}

static void paste_msg_editor(void* unused1, void* unused2, void* unused3)
{
	char* temp = SDL_GetClipboardText();

	if(strcmp(temp, "") == 0)
	{
		msgbox(domain, mused.slider_bevel, &mused.largefont, "Clipboard is empty!", MB_OK);
		
		goto end;
	}

	if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite message with clipboard text?"))
	{
		free(mused.song.song_message);
		mused.song.song_message = calloc(1, sizeof(char) * (strlen(temp) + 2));

		strcpy(mused.song.song_message, temp);

		mused.song.song_message_length = strlen(mused.song.song_message) + 1;

		init_lines();
	}

	end:;

	SDL_free(temp);
}

static void clear_msg_editor(void* unused1, void* unused2, void* unused3)
{
	if(confirm(domain, mused.slider_bevel, &mused.largefont, "Clear song message?"))
	{
		free(mused.song.song_message);
		mused.song.song_message = calloc(1, sizeof(char) * 2);

		strcpy(mused.song.song_message, "");

		mused.song.song_message_length = strlen(mused.song.song_message) + 1;

		slider_set_params(&msg_data.scrollbar, 0, msg_data.n_lines - 1, msg_data.list_position, 0, &msg_data.list_position, 1, SLIDER_VERTICAL, msg_data.gfx);

		init_lines();
	}

	if(msg_data.editing)
	{
		SDL_StopTextInput();
		SDL_StartTextInput();
	}

	else
	{
		SDL_StartTextInput();
		SDL_StopTextInput();
	}
}