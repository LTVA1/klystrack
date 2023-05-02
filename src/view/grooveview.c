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
#define DIALOG_WIDTH 480
#define DIALOG_HEIGHT 320
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
	
}

static void ok_action(void *unused0, void *unused1, void *unused2)
{
	parse_groove_string();
	data.quit = 1;
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
			const SDL_Rect *r = console_write_args(mused.console, "%c", mused.editpos == i ? '�' : text[i]);
			
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


static void parameters_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	SDL_Rect button;
	
	copy_rect(&button, area);
	
	
	
	//data.subsong += generic_field(event, &button, -1, -1, "SUBSONG", "%02d", MAKEPTR(data.subsong), 2);
	button.y += button.h;
	
	//data.hub->n_tracks += generic_field(event, &button, -1, -1, "TRACKS", "%d", MAKEPTR(data.hub->n_tracks), 1);
	button.y += button.h;
	
	//data.hub->addr.patternptrlo += generic_field(event, &button, -1, -1, "PAT LO", "%04X", MAKEPTR(data.hub->addr.patternptrlo), 4);
	button.y += button.h;
	
	//data.hub->addr.patternptrhi += generic_field(event, &button, -1, -1, "PAT HI", "%04X", MAKEPTR(data.hub->addr.patternptrhi), 4);
	button.y += button.h;
	
	//data.hub->addr.songtab += generic_field(event, &button, -1, -1, "SONGS", "%04X", MAKEPTR(data.hub->addr.songtab), 4);
	button.y += button.h;
	
	//data.hub->addr.instruments += generic_field(event, &button, -1, -1, "INSTRUMENTS", "%04X", MAKEPTR(data.hub->addr.instruments), 4);
	button.y += button.h;
	
	button.w = DIALOG_WIDTH - MARGIN * 2;
	button.y = area->y + area->h - 10 - BUTTONS - 10 - 3;
	button.h = 10;
	
	text_field(event, &button, sizeof(mused.groove_string), mused.groove_string);
}


static void buttons_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	SDL_Rect button;
	
	copy_rect(&button, area);
	
	button.w = strlen("OK") * data.largefont->w + 24;
	button.x = area->w + area->x - button.w;
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
				SDL_PushEvent(&e);
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
}