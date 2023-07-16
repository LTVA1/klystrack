/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2023 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "grooveview.h"

#include "mused.h"
#include "view.h"
#include "gui/msgbox.h"
#include "gui/bevel.h"
#include "gui/bevdefs.h"
#include "gui/dialog.h"
#include "gfx/gfx.h"
#include "gui/view.h"
#include "gui/mouse.h"
#include "gui/toolutil.h"
#include <string.h>

#define TOP_LEFT 0
#define TOP_RIGHT 0
#define MARGIN 8
#define SCREENMARGIN 64
#define DIALOG_WIDTH 320
#define DIALOG_HEIGHT 160
#define TITLE 14
#define FIELD 14
#define CLOSE_BUTTON 12
#define ELEMWIDTH data.elemwidth
#define LIST_WIDTH data.list_width
#define BUTTONS 16

static struct
{
	const char *title;
	int quit;
	const Font *largefont, *smallfont;
	GfxSurface *gfx;
	
	Uint8 speed_nom;
	Uint8 speed_denom;
} data;

extern const KeyShortcut shortcuts[];

extern Mused mused;

static void title_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);
static void window_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);
static void buttons_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);
static void parameters_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);

static const View groove_view_array[] =
{
	{{ SCREENMARGIN, SCREENMARGIN, DIALOG_WIDTH, DIALOG_HEIGHT-MARGIN }, window_view, &data, -1},
	{{ MARGIN+SCREENMARGIN, SCREENMARGIN+MARGIN, DIALOG_WIDTH-MARGIN*2, TITLE - 2 }, title_view, &data, -1},
	{{ SCREENMARGIN+MARGIN, SCREENMARGIN+MARGIN+TITLE+2, DIALOG_WIDTH-MARGIN, DIALOG_HEIGHT-BUTTONS-2-MARGIN }, parameters_view, &data, -1},
	{{ SCREENMARGIN+MARGIN, DIALOG_HEIGHT-BUTTONS+2+SCREENMARGIN-MARGIN-8, DIALOG_WIDTH-MARGIN*2, BUTTONS-2 }, buttons_view, &data, -1},
	{{0, 0, 0, 0}, NULL}
};


static void parse_groove_string()
{
	char* temp_string = (char*)calloc(1, sizeof(mused.groove_string) + 1);
	strcpy(temp_string, mused.groove_string);
	
	const char delimiters[] = " .,!?";
	
	char* current_number;
	
	Uint8 groove_index = 0;
	
	int passes = 0;
	
	while(current_number != NULL)
	{
		current_number = strtok(passes > 0 ? NULL : temp_string, delimiters);
		passes++;
		
		if(current_number)
		{
			Sint64 number = atoi(current_number);
			
			if(number > 0 && number <= 0xff)
			{
				mused.song.grooves[mused.current_groove][groove_index] = number;
				groove_index++;
			}
		}
	}
	
	if(groove_index > 0)
	{
		for(int i = groove_index; i < MUS_MAX_GROOVE_LENGTH; i++)
		{
			mused.song.grooves[mused.current_groove][i] = 0;
		}
	}
	
	mused.song.groove_length[mused.current_groove] = groove_index;
	
	free(temp_string);
}

static void update_groove_string()
{
	char* number = calloc(1, 6);
	
	int current_string_position = 0;
	
	memset(mused.groove_string, 0, sizeof(mused.groove_string));
	
	for(int i = 0; i < mused.song.groove_length[mused.current_groove]; i++)
	{
		memset(number, 0, 6);
		
		number = my_itoa(mused.song.grooves[mused.current_groove][i], number);
		
		Uint8 size = 1 + (mused.song.grooves[mused.current_groove][i] > 10 ? 1 : 0) + (mused.song.grooves[mused.current_groove][i] > 100 ? 1 : 0);
		
		number[size] = ' ';
		
		memcpy(&mused.groove_string[current_string_position], number, ((i == mused.song.groove_length[mused.current_groove] - 1) ? size : (size + 1)));
		
		current_string_position += size + 1;
	}
	
	free(number);
}

static void copy_action(void *unused0, void *unused1, void *unused2)
{
	SDL_SetClipboardText(mused.groove_string);
}

static void paste_action(void *unused0, void *unused1, void *unused2)
{
	char* temp = SDL_GetClipboardText();
	
	if(temp)
	{
		if(strlen(temp) < sizeof(mused.groove_string))
		{
			strcpy(mused.groove_string, temp);
		}
		
		SDL_free(temp);
		
		mused.editpos = strlen(mused.groove_string);
	}
}

static void ok_action(void *unused0, void *unused1, void *unused2)
{
	parse_groove_string();
	update_groove_string();
	
	mused.current_groove_position = 0;
}

static void move_up_action(void *unused0, void *unused1, void *unused2)
{
	if(mused.current_groove_position > 0 && mused.song.groove_length[mused.current_groove] > 1)
	{
		Uint8 current_num = mused.song.grooves[mused.current_groove][mused.current_groove_position];
		Uint8 prev_num = mused.song.grooves[mused.current_groove][mused.current_groove_position - 1];
		
		mused.song.grooves[mused.current_groove][mused.current_groove_position] = prev_num;
		mused.song.grooves[mused.current_groove][mused.current_groove_position - 1] = current_num;
		
		mused.current_groove_position--;
		
		update_groove_string();
	}
}

static void move_down_action(void *unused0, void *unused1, void *unused2)
{
	if(mused.current_groove_position < mused.song.groove_length[mused.current_groove] - 1 && mused.song.groove_length[mused.current_groove] > 1)
	{
		Uint8 current_num = mused.song.grooves[mused.current_groove][mused.current_groove_position];
		Uint8 next_num = mused.song.grooves[mused.current_groove][mused.current_groove_position + 1];
		
		mused.song.grooves[mused.current_groove][mused.current_groove_position + 1] = current_num;
		mused.song.grooves[mused.current_groove][mused.current_groove_position] = next_num;
		
		mused.current_groove_position++;
		
		update_groove_string();
	}
}

static void generate_groove_action(void *unused0, void *unused1, void *unused2)
{
	if(data.speed_nom != 0 && data.speed_denom != 0) //shamelessly stolen from Dn-FamiTracker source code
	{
		if(data.speed_denom < 1 || data.speed_denom > MUS_MAX_GROOVE_LENGTH || data.speed_nom < data.speed_denom || data.speed_nom > data.speed_denom * 255) return;
		
		memset(&mused.song.grooves[mused.current_groove][0], 0, sizeof(mused.song.grooves[mused.current_groove][0]) * MUS_MAX_GROOVE_LENGTH);
		
		mused.song.groove_length[mused.current_groove] = data.speed_denom;
		
		for(int i = 0; i < (int)data.speed_denom * data.speed_nom; i += data.speed_nom)
		{
			mused.song.grooves[mused.current_groove][data.speed_denom - i / data.speed_nom - 1] = (i + data.speed_nom) / data.speed_denom - i / data.speed_denom;
		}
		
		update_groove_string();
		
		mused.current_groove_position = 0;
	}
}

static void delete_groove_action(void *unused0, void *unused1, void *unused2)
{
	memset(&mused.song.grooves[mused.current_groove][0], 0, sizeof(mused.song.grooves[mused.current_groove][0]) * MUS_MAX_GROOVE_LENGTH);
	mused.song.groove_length[mused.current_groove] = 0;
	
	update_groove_string();
	
	mused.current_groove_position = 0;
}

static void text_field(const SDL_Event *e, const SDL_Rect *area, int length, char *text)
{
	console_set_color(mused.console, colors[COLOR_MAIN_TEXT]);
	console_set_clip(mused.console, area);
	console_clear(mused.console);

	bevelex(domain, area, mused.slider_bevel, BEV_FIELD, BEV_F_STRETCH_ALL);

	SDL_Rect field;
	copy_rect(&field, area);
	adjust_rect(&field, 1);

	console_set_clip(mused.console, &field);

	int got_pos = 0;

	if (mused.edit_buffer == text && mused.focus == EDITBUFFER)
	{
		int i = my_max(0, mused.editpos - field.w / mused.console->font.w + 1), c = 0;
		
		for (; text[i] && c < my_min(length, field.w / mused.console->font.w); ++i, ++c)
		{
			const SDL_Rect *r = console_write_args(mused.console, "%c", mused.editpos == i ? 'Ѕ' : text[i]);
			
			if (check_event(e, r, NULL, NULL, NULL, NULL))
			{
				mused.editpos = i;
				got_pos = 1;
			}
		}

		if (mused.editpos == i && c <= length)
		{
			console_write(mused.console, "�");
		}
	}
	
	else
	{
		char temp[1000];
		strncpy(temp, text, my_min(sizeof(temp), length));

		temp[my_max(0, my_min(sizeof(temp), field.w / mused.console->font.w))] = '\0';

		console_write_args(mused.console, "%s", temp);
	}

	int c = 1;

	if (!got_pos && (c = check_event(e, &field, NULL, 0, 0, 0)))
	{
		if (mused.focus == EDITBUFFER && mused.edit_buffer == text)
			mused.editpos = strlen(text);
		else
			set_edit_buffer(text, length);
	}
	
	if (!c && mused.focus == EDITBUFFER && e->type == SDL_MOUSEBUTTONDOWN) mused.focus = 0;//change_mode(mused.prev_mode);
}

void select_groove(void *idx, void *unused2, void *unused3)
{
	if(CASTPTR(int, idx) >= 0 && CASTPTR(int, idx) < MUS_MAX_GROOVES)
	{
		mused.current_groove = CASTPTR(int, idx);
		mused.current_groove_position = 0;
		update_groove_string();
	}
}

void select_groove_position(void *idx, void *unused2, void *unused3)
{
	if(CASTPTR(int, idx) >= 0 && CASTPTR(int, idx) < mused.song.groove_length[mused.current_groove])
	{
		mused.current_groove_position = CASTPTR(int, idx);
	}
}

void groove_list(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 3);
	console_set_clip(mused.console, &area);

	int y = area.y;

	int start = mused.groove_list_position;

	for (int i = start; i < MUS_MAX_GROOVES && y < area.h + area.y; ++i, y += mused.console->font.h)
	{
		SDL_Rect row = { area.x - 1, y - 1, area.w + 2, mused.console->font.h + 1};

		if (i == mused.current_groove)
		{
			bevel(domain, &row, mused.slider_bevel, BEV_SELECTED_PATTERN_ROW);
			console_set_color(mused.console, colors[COLOR_INSTRUMENT_SELECTED]);
		}
		
		else
		{
			console_set_color(mused.console, colors[COLOR_INSTRUMENT_NORMAL]);
		}

		console_write_args(mused.console, "%02X %c\n", i, mused.song.groove_length[i] == 0 ? ' ' : '#');

		check_event(event, &row, select_groove, MAKEPTR(i), 0, 0);

		slider_set_params(&mused.groove_list_slider_param, 0, MUS_MAX_GROOVES - 1, start, i, (int*)&mused.groove_list_position, 1, SLIDER_VERTICAL, mused.slider_bevel);
	}
}

void current_groove_list(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 3);
	console_set_clip(mused.console, &area);

	int y = area.y;

	int start = mused.current_groove_list_position;

	for (int i = start; i < mused.song.groove_length[mused.current_groove] && y < area.h + area.y; ++i, y += mused.console->font.h)
	{
		SDL_Rect row = { area.x - 1, y - 1, area.w + 2, mused.console->font.h + 1};

		if (i == mused.current_groove_position)
		{
			bevel(domain, &row, mused.slider_bevel, BEV_SELECTED_PATTERN_ROW);
			console_set_color(mused.console, colors[COLOR_INSTRUMENT_SELECTED]);
		}
		
		else
		{
			console_set_color(mused.console, colors[COLOR_INSTRUMENT_NORMAL]);
		}

		console_write_args(mused.console, "%02X: %d\n", i, mused.song.grooves[mused.current_groove][i]);

		check_event(event, &row, select_groove_position, MAKEPTR(i), 0, 0);

		slider_set_params(&mused.groove_editor_slider_param, 0, mused.song.groove_length[mused.current_groove] - 1, start, i, (int*)&mused.current_groove_list_position, 1, SLIDER_VERTICAL, mused.slider_bevel);
	}
}


static void parameters_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	SDL_Rect list;
	copy_rect(&list, area);
	
	list.w = data.largefont->w * 5 + 4;
	list.h -= 40;
	
	groove_list(dest_surface, &list, event, param);
	
	list.x += list.w;
	list.w = 10;
	
	slider(dest_surface, &list, event, &mused.groove_list_slider_param);
	
	list.w = data.largefont->w * 8 + 4;
	list.x += 30;
	
	current_groove_list(dest_surface, &list, event, param);
	
	list.x += list.w;
	list.w = 10;
	
	slider(dest_surface, &list, event, &mused.groove_editor_slider_param);
	
	SDL_Rect button;
	
	copy_rect(&button, area);
	
	button.w = DIALOG_WIDTH - MARGIN * 2 - 2;
	button.y = area->y + area->h - 10 - BUTTONS - 10 - 3;
	button.h = 10;
	
	text_field(event, &button, sizeof(mused.groove_string), mused.groove_string);
	
	button.x = list.x + 20;
	button.y = list.y;
	button.h = 12;
	
	button.w = strlen("OK") * data.largefont->w + 32;
	button_text_event(dest_surface, event, &button, data.gfx, data.largefont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "UP", move_up_action, 0, 0, 0);
	
	button.x += button.w + 20;
	button.w += data.largefont->w * 2;
	button_text_event(dest_surface, event, &button, data.gfx, data.largefont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "DELETE", delete_groove_action, 0, 0, 0);
	button.w -= data.largefont->w * 2;
	button.x -= button.w + 20;
	
	button.y += 12;
	button_text_event(dest_surface, event, &button, data.gfx, data.largefont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "DOWN", move_down_action, 0, 0, 0);
	
	int temp = button.x;
	
	button.y += 15;
	button.w = 90;
	button.h = 10;
	
	data.speed_nom += generic_field(event, &button, -1, -1, "SPEED", "%3d", MAKEPTR(data.speed_nom), 3);
	
	char string[200];
	
	button.x += 90 + 2;
	button.w = data.largefont->w * 3 + 4 * 2;
	
	snprintf(string, sizeof(string), "/");
	font_write(data.largefont, dest_surface, &button, string);
	
	button.x += data.largefont->w + 12;
	
	data.speed_denom += generic_field(event, &button, -1, -1, "", "%3d", MAKEPTR(data.speed_denom), 3);
	
	button.x -= 90 + 2 + data.largefont->w + 12;
	button.w = 90;
	button.y += 12;
	button.h = 12;
	
	button_text_event(dest_surface, event, &button, data.gfx, data.largefont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "GENERATE", generate_groove_action, 0, 0, 0);
	
	button.x = temp;
	
	button.y = area->y + area->h - 10 - BUTTONS - 10 - 3 - 12;
	button.w = DIALOG_WIDTH - MARGIN * 2 - 2;
	
	float speed = 0.0;
	
	for(int i = 0; i < mused.song.groove_length[mused.current_groove]; i++)
	{
		speed += (float)mused.song.grooves[mused.current_groove][i];
	}
	
	if(speed > 0.0 && mused.song.groove_length[mused.current_groove] > 0)
	{
		speed /= mused.song.groove_length[mused.current_groove];
	}
	
	snprintf(string, sizeof(string), "Speed: %.3f", speed);
	font_write(data.largefont, dest_surface, &button, string);
}


static void buttons_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	SDL_Rect button;
	
	copy_rect(&button, area);
	
	button.w = strlen("OK") * data.largefont->w + 32;
	button.x = area->w + area->x - button.w * 3 - 10 - 3;
	button_text_event(dest_surface, event, &button, data.gfx, data.largefont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "COPY", copy_action, 0, 0, 0);
	button.x += button.w + 1;
	button_text_event(dest_surface, event, &button, data.gfx, data.largefont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "PASTE", paste_action, 0, 0, 0);
	button.x += button.w + 10;
	button_text_event(dest_surface, event, &button, data.gfx, data.largefont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "OK", ok_action, 0, 0, 0);
	button.x += button.w + 1;
}


void window_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	bevel(dest_surface, area, data.gfx, BEV_MENU);
}


void title_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	const char* title = data.title;
	SDL_Rect titlearea, button;
	copy_rect(&titlearea, area);
	titlearea.w -= CLOSE_BUTTON - 4;
	copy_rect(&button, area);
	adjust_rect(&button, titlearea.h - CLOSE_BUTTON);
	button.w = CLOSE_BUTTON;
	button.x = area->w + area->x - CLOSE_BUTTON;
	font_write(data.largefont, dest_surface, &titlearea, title);
	
	if (button_event(dest_surface, event, &button, data.gfx, BEV_BUTTON, BEV_BUTTON_ACTIVE, DECAL_CLOSE, NULL, MAKEPTR(1), 0, 0) & 1)
	{
		data.quit = 1;
	}
}


void groove_view()
{
	set_repeat_timer(NULL);
	
	memset(&data, 0, sizeof(data));
	data.title = "Groove settings";
	data.largefont = &mused.largefont;
	data.smallfont = &mused.smallfont;
	data.gfx = mused.slider_bevel;
	
	data.speed_nom = 12;
	data.speed_denom = 2;
	
	while (!data.quit)
	{
		SDL_Event e = { 0 };
		int got_event = 0;
		
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				case SDL_QUIT:
				
				set_repeat_timer(NULL);
				//SDL_PushEvent(&e);
				return;
				
				break;
				
				case SDL_WINDOWEVENT:
				{
					switch (e.window.event) 
					{
						case SDL_WINDOWEVENT_RESIZED:
						{
							domain->screen_w = my_max(320, e.window.data1 / domain->scale);
							domain->screen_h = my_max(240, e.window.data2 / domain->scale);
							
							if (!(mused.flags & FULLSCREEN))
							{
								mused.window_w = domain->screen_w * domain->scale;
								mused.window_h = domain->screen_h * domain->scale;
							}
							
							gfx_domain_update(domain, false);
						}
						break;
					}
					break;
				}
				
				case SDL_TEXTEDITING:
				case SDL_TEXTINPUT:
					switch (mused.focus)
					{
						case EDITBUFFER:
							edit_text(&e);
						break;
					}
					break;
				
				case SDL_KEYDOWN:
				{
					if (e.key.keysym.sym != 0)
					{
						switch (mused.focus)
						{
							case EDITBUFFER:
							edit_text(&e);
							break;
						}
					}
					
					switch (e.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						
						set_repeat_timer(NULL);
						return;
						
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
			
			if (e.type != SDL_MOUSEMOTION || (e.motion.state)) ++got_event;
			
			// Process mouse click events immediately, and batch mouse motion events
			// (process the last one) to fix lag with high poll rate mice on Linux.
			//fix from here https://github.com/kometbomb/klystrack/pull/300
			if (should_process_mouse(&e))
				break;
		}
		
		if (got_event || gfx_domain_is_next_frame(domain))
		{
			draw_view(domain, groove_view_array, &e);
			gfx_domain_flip(domain);
		}
		else
			SDL_Delay(5);
	}

	bool quit = false;

	while (!quit)
	{
		SDL_Event e = { 0 };
		int got_event = 0;
		
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				case SDL_QUIT:
				
				set_repeat_timer(NULL);
				//SDL_PushEvent(&e);
				return;
			
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
					{
						mouse_released(&e);
					}

					quit = true;
				}
				break;
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
			draw_view(domain, groove_view_array, &e);
			gfx_domain_flip(domain);
		}
		else
			SDL_Delay(5);
	}
}