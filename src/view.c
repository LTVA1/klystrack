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

#include "view.h"
#include "event.h"
#include "mused.h"
#include "action.h"
#include "diskop.h"
#include "gui/mouse.h"
#include "gui/dialog.h"
#include "gui/bevel.h"
#include "theme.h"
#include "mybevdefs.h"
#include "snd/freqs.h"
#include "view/visu.h"
#include "view/sequence.h"
#include "view/grooveview.h"
#include "view/songmessage.h"
#include <stdbool.h>
#include "edit.h"
#include "mymsg.h"
#include "command.h"
#include <string.h>

#include "view/oscilloscope.h" //wasn't there

#include "import/mml_string.h" //wasn't there
#include "import/wavetable_string.h" //wasn't there

extern Mused mused;

extern int event_hit;

void open_4op(void *unused1, void *unused2, void *unused3);
void open_prog(void *unused1, void *unused2, void *unused3);
void open_env(void *unused1, void *unused2, void *unused3);
void open_local_samples(void *unused1, void *unused2, void *unused3);

float percent_to_dB(float percent)
{
	return 10 * log10(percent);
}

bool is_selected_param(int focus, int p)
{
	if (focus == mused.focus)
	switch (focus)
	{
		case EDITINSTRUMENT:
			return p == mused.selected_param;
			break;
			
		case EDIT4OP:
			return p == mused.fourop_selected_param;
			break;

		case EDITFX:
			return p == mused.edit_reverb_param;
			break;

		case EDITWAVETABLE:
			return p == mused.wavetable_param;
			break;
			
		case EDITLOCALSAMPLE:
			return p == mused.local_sample_param;
			break;

		case EDITSONGINFO:
			return p == mused.songinfo_param;
			break;
	}

	return false;
}


void my_separator(const SDL_Rect *parent, SDL_Rect *rect)
{
	separator(domain, parent, rect, mused.slider_bevel, BEV_SEPARATOR);
}

// note: since we need to handle the focus this piece of code is duplicated from gui/view.c

void my_draw_view(const View* views, const SDL_Event *_event, GfxDomain *domain, int m)
{
	gfx_rect(domain, NULL, colors[COLOR_OF_BACKGROUND]);

	SDL_Event event;
	memcpy(&event, _event, sizeof(event));

	int orig_focus = mused.focus;

	for (int i = 0; views[i].handler; ++i)
	{
		const View *view = &views[i];
		SDL_Rect area;
		area.x = view->position.x >= 0 ? view->position.x : domain->screen_w + view->position.x;
		area.y = view->position.y >= 0 ? view->position.y : domain->screen_h + view->position.y;
		area.w = *(Sint16*)&view->position.w > 0 ? *(Sint16*)&view->position.w : domain->screen_w + *(Sint16*)&view->position.w - view->position.x;
		area.h = *(Sint16*)&view->position.h > 0 ? *(Sint16*)&view->position.h : domain->screen_h + *(Sint16*)&view->position.h - view->position.y;

		memcpy(&mused.console->clip, &area, sizeof(view->position));
		int iter = 0;
		
		do
		{
			event_hit = 0;
			view->handler(domain, &area, &event, view->param);
			
			if (event_hit)
			{
				event.type = MSG_EVENTHIT;
				
				if (view->focus != -1 && mused.focus != view->focus && orig_focus != EDITBUFFER && mused.focus != EDITBUFFER)
				{
					if((mused.show_four_op_menu && (m == 3 || m == 4) && i > 9) || (!(mused.show_four_op_menu) && (m != 3 || m != 4) && (i <= 9 || (i == 13 && !(mused.show_four_op_menu)))))
					//if((mused.show_four_op_menu) || (!(mused.show_four_op_menu) && (m != 3 || m != 4) && i != 11))
					{
						if((mused.show_point_envelope_editor && (m == 3 || m == 4 || m == EDITPROG4OP) && i == 6) || (i == 13 && mused.show_four_op_menu) || (i == 7 && mused.show_point_envelope_editor && (m == EDITPROG || m == EDITINSTRUMENT)) 
							|| (mused.show_4op_point_envelope_editor && (i == 4 || i == 3 || i == 10 || i == 11) && (m == 3 || m == 4 || m == EDITPROG4OP)))
						{
							
						}
						
						else //so we don't focus on program editor when point envelope editor is open
						{
							if(mused.mode != EDITLOCALSAMPLE && view->focus != EDITINSTRUMENT)
							{
								if((view->focus == EDITENVELOPE && !(mused.show_point_envelope_editor)) || (view->focus == EDITENVELOPE4OP && !(mused.show_4op_point_envelope_editor)))
								{

								}

								else
								{
									mused.focus = view->focus;
								}
							}
							
							//debug("152 new focus %d", mused.focus);
						}
						
						if(!(mused.selection.drag_selection_program_4op))
						{
							//clear_selection(0, 0, 0);
						}
					}
				}
				
				++iter;
			}
		}
		while (event_hit && iter <= 1);

		if (!event_hit && event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT
			&& event.button.x >= area.x && event.button.x < area.x + area.w
			&& event.button.y >= area.y && event.button.y < area.y + area.h)
		{
			if (view->focus != -1 && mused.focus != view->focus)
			{
				if((mused.show_four_op_menu && (m == 3 || m == 4) && i > 9) || (!(mused.show_four_op_menu) && (m != 3 || m != 4) && (i == 13 && !(mused.show_four_op_menu))))
				//if((mused.show_four_op_menu) || (!(mused.show_four_op_menu) && (m != 3 || m != 4) && i != 11))
				{
					if (orig_focus == EDITBUFFER)
					{
						if((mused.show_point_envelope_editor && (m == 3 || m == 4 || m == EDITPROG4OP) && i == 6) || (i == 13 && mused.show_four_op_menu) || (i == 7 && mused.show_point_envelope_editor && (m == EDITPROG || m == EDITINSTRUMENT)) 
							|| (mused.show_4op_point_envelope_editor && (i == 4 || i == 3 || i == 10 || i == 11) && (m == 3 || m == 4 || m == EDITPROG4OP)))
						{
							
						}
						
						else
						{
							if((view->focus == EDITENVELOPE && !(mused.show_point_envelope_editor)) || (view->focus == EDITENVELOPE4OP && !(mused.show_4op_point_envelope_editor)))
							{

							}

							else
							{
								change_mode(view->focus);
							}
							
							
							//debug("187 new mode %d", mused.mode);
						}
					}
					
					if((mused.show_point_envelope_editor && (m == 3 || m == 4 || m == EDITPROG4OP) && i == 6) || (i == 13 && mused.show_four_op_menu) || (i == 7 && mused.show_point_envelope_editor && (m == EDITPROG || m == EDITINSTRUMENT)) 
							|| (mused.show_4op_point_envelope_editor && (i == 4 || i == 3 || i == 10 || i == 11) && (m == 3 || m == 4 || m == EDITPROG4OP)))
					{
						
					}
					
					else
					{
						if(mused.mode != EDITLOCALSAMPLE && view->focus != EDITINSTRUMENT)
						{
							if((view->focus == EDITENVELOPE && !(mused.show_point_envelope_editor)) || (view->focus == EDITENVELOPE4OP && !(mused.show_4op_point_envelope_editor)))
							{

							}

							else
							{
								mused.focus = view->focus;
							}
						}
						
						//debug("200 new focus %d i = %d m = %d", mused.focus, i, m);
					}
					
					if(!(mused.selection.drag_selection_program_4op))
					{
						//clear_selection(0, 0, 0);
					}
				}
			}
		}
	}
	
	//debug("final focus %d", mused.focus);

	mused.cursor.w = (mused.cursor_target.w + mused.cursor.w * 2) / 3;
	mused.cursor.h = (mused.cursor_target.h + mused.cursor.h * 2) / 3;
	mused.cursor.x = (mused.cursor_target.x + mused.cursor.x * 2) / 3;
	mused.cursor.y = (mused.cursor_target.y + mused.cursor.y * 2) / 3;

	if (mused.cursor.w < mused.cursor_target.w) ++mused.cursor.w;
	if (mused.cursor.w > mused.cursor_target.w) --mused.cursor.w;

	if (mused.cursor.h < mused.cursor_target.h) ++mused.cursor.h;
	if (mused.cursor.h > mused.cursor_target.h) --mused.cursor.h;

	if (mused.cursor.x < mused.cursor_target.x) ++mused.cursor.x;
	if (mused.cursor.x > mused.cursor_target.x) --mused.cursor.x;

	if (mused.cursor.y < mused.cursor_target.y) ++mused.cursor.y;
	if (mused.cursor.y > mused.cursor_target.y) --mused.cursor.y;

	if (mused.cursor.w > 0) bevelex(domain, &mused.cursor, mused.slider_bevel, (mused.flags & EDIT_MODE) ? BEV_EDIT_CURSOR : BEV_CURSOR, BEV_F_STRETCH_ALL|BEV_F_DISABLE_CENTER);
}

static const char* notename_array[] =
{
	"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
};

static const char* notename_negative[] =
{
	"c_", "c+", "d_", "d+", "e_", "f_", "f+", "g_", "g+", "a_", "a+", "b_"
};

char* notename_default(int note) //for importing OpenMPT/Furnace formatted text data
{
	note = my_min(my_max(0, note), FREQ_TAB_SIZE - 1);
	static char buffer[4];

	if(note < C_ZERO)
	{
		sprintf(buffer, "%s%d", notename_negative[note % 12], (C_ZERO - note + 11) / 12);
	}
	
	else
	{
		sprintf(buffer, "%s%d", notename_array[note % 12], (note - C_ZERO) / 12);
	}

	return buffer;
}

char* notename(int note)
{
	note = my_min(my_max(0, note), FREQ_TAB_SIZE - 1);
	static char buffer[4];
	
	static const char * notename_flats[] =
	{
		"C-", "Db", "D-", "Eb", "E-", "F-", "Gb", "G-", "Ab", "A-", "Bb", "B-"
	};
	
	static const char * notename_negative_flats[] =
	{
		"c_", "d\xc", "d_", "e\xc", "e_", "f_", "g\xc", "g_", "a\xc", "a_", "b\xc", "b_"
	};
	
	if(note < C_ZERO)
	{
		sprintf(buffer, "%s%d", (mused.flags2 & SHOW_FLATS_INSTEAD_OF_SHARPS) ? notename_negative_flats[note % 12] : notename_negative[note % 12], (C_ZERO - note + 11) / 12);
	}
	
	else
	{
		sprintf(buffer, "%s%d", (mused.flags2 & SHOW_FLATS_INSTEAD_OF_SHARPS) ? notename_flats[note % 12] : notename_array[note % 12], (note - C_ZERO) / 12);
	}
	
	return buffer;
}


void label(const char *_label, const SDL_Rect *area)
{
	SDL_Rect r;

	copy_rect(&r, area);

	r.y = r.y + area->h / 2 - mused.smallfont.h / 2;

	font_write(&mused.smallfont, domain, &r, _label);
}


void set_cursor(const SDL_Rect *location)
{
	if (location == NULL)
	{
		mused.cursor_target.w = 0;
		mused.cursor.w = 0;
		return;
	}

	if (mused.flags & ANIMATE_CURSOR)
	{
		copy_rect(&mused.cursor_target, location);

		if (mused.cursor.w == 0 || mused.cursor.h == 0)
			copy_rect(&mused.cursor, location);
	}
	
	else
	{
		copy_rect(&mused.cursor_target, location);
		copy_rect(&mused.cursor, location);
	}
}


bool check_mouse_hit(const SDL_Event *e, const SDL_Rect *area, int focus, int param)
{
	if (param < 0) return false;
	if (check_event(e, area, NULL, NULL, NULL, NULL))
	{
		switch (focus)
		{
			case EDITINSTRUMENT:
				mused.selected_param = param;
				break;
				
			case EDITENVELOPE:
				mused.env_selected_param = param;
				break;
				
			case EDITENVELOPE4OP:
				mused.fourop_env_selected_param = param;
				break;
			
			case EDIT4OP:
				mused.fourop_selected_param = param;
				break;

			case EDITFX:
				mused.edit_reverb_param = param;
				break;

			case EDITWAVETABLE:
				mused.wavetable_param = param;
				break;
				
			case EDITLOCALSAMPLE:
				mused.local_sample_param = param;
				break;

			case EDITSONGINFO:
				mused.songinfo_param = param;
				break;
		}

		mused.focus = focus;

		return true;
	}

	return false;
}


int generic_field(const SDL_Event *e, const SDL_Rect *area, int focus, int param, const char *_label, const char *format, void *value, int width)
{
	label(_label, area);

	SDL_Rect field, spinner_area;
	copy_rect(&field, area);

	field.w = width * mused.console->font.w + 2;
	field.x = area->x + area->w - field.w;

	copy_rect(&spinner_area, &field);

	spinner_area.x += field.w;
	spinner_area.w = 16;
	field.x -= spinner_area.w;
	spinner_area.x -= spinner_area.w;

	bevelex(domain, &field, mused.slider_bevel, BEV_FIELD, BEV_F_STRETCH_ALL);

	adjust_rect(&field, 1);

	font_write_args(&mused.largefont, domain, &field, format, value);

	int r = spinner(domain, e, &spinner_area, mused.slider_bevel, (Uint32)area->x << 16 | area->y);

	check_mouse_hit(e, area, focus, param);

	if (is_selected_param(focus, param))
	{
		SDL_Rect r;
		copy_rect(&r, area);
		adjust_rect(&r, -2);

		set_cursor(&r);
	}

	return r * (SDL_GetModState() & KMOD_SHIFT ? 16 : 1);
}

int generic_field_simple(const SDL_Event *e, const SDL_Rect *area, const char *_label, const char *format, void *value, int width)
{
	label(_label, area);

	SDL_Rect field, spinner_area;
	copy_rect(&field, area);

	field.w = width * mused.console->font.w + 2;
	field.x = area->x + area->w - field.w;

	copy_rect(&spinner_area, &field);

	spinner_area.x += field.w;
	spinner_area.w = 16;
	field.x -= spinner_area.w;
	spinner_area.x -= spinner_area.w;

	bevelex(domain, &field, mused.slider_bevel, BEV_FIELD, BEV_F_STRETCH_ALL);

	adjust_rect(&field, 1);

	font_write_args(&mused.largefont, domain, &field, format, value);

	int r = spinner(domain, e, &spinner_area, mused.slider_bevel, (Uint32)area->x << 16 | area->y);

	return r * (SDL_GetModState() & KMOD_SHIFT ? 16 : 1);
}


void generic_flags(const SDL_Event *e, const SDL_Rect *_area, int focus, int p, const char *label, Uint32 *_flags, Uint32 mask)
{
	SDL_Rect area;
	copy_rect(&area, _area);
	area.y += 1;

	int hit = check_mouse_hit(e, _area, focus, p);
	Uint32 flags = *_flags;

		if (checkbox(domain, e, &area, mused.slider_bevel, &mused.smallfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, DECAL_TICK, label, &flags, mask))
		{

		}
		
		else if (hit)
		{
			if(mused.mode == focus)
			{
			// so that the gap between the box and label works too
			flags ^= mask;
			}
		}
	

	if (*_flags != flags)
	{
		switch (focus)
		{
			case EDITINSTRUMENT: case EDIT4OP: case EDITENVELOPE: case EDITENVELOPE4OP: snapshot(S_T_INSTRUMENT); break;
			case EDITFX: snapshot(S_T_FX); break;
			case EDITSONGINFO: snapshot(S_T_SONGINFO); break;
		}
		*_flags = flags;
	}

	if (is_selected_param(focus, p))
	{
		SDL_Rect r;
		copy_rect(&r, &area);
		adjust_rect(&r, -2);
		r.h -= 2;
		r.w -= 2;
		set_cursor(&r);
	}
}


int generic_button(const SDL_Event *e, const SDL_Rect *area, int focus, int param, const char *_label, void (*action)(void*,void*,void*), void *p1, void *p2, void *p3)
{
	if (is_selected_param(focus, param))
	{
		SDL_Rect r;
		copy_rect(&r, area);
		adjust_rect(&r, -2);
		r.h -= 2;
		r.w -= 2;
		set_cursor(&r);
	}

	return button_text_event(domain, e, area, mused.slider_bevel, &mused.smallfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, _label, action, p1, p2, p3);
}


void songinfo1_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain,&area, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);

	area.w = my_min(320, area.w);

	adjust_rect(&area, 2);
	SDL_Rect r;
	copy_rect(&r, &area);
	r.w = 100; //100 - 8

	if (area.w > r.w)
	{
		r.w = area.w / (int)(area.w / r.w) - 5;
	}
	else
	{
		r.w = area.w;
	}

	r.h = 10;
	console_set_clip(mused.console, &area);

	int d;

	d = generic_field(event, &r, EDITSONGINFO, SI_LENGTH, "LENGTH", "%04X", MAKEPTR(mused.song.song_length), 4);
	songinfo_add_param(d);
	update_rect(&area, &r);

	d = generic_field(event, &r, EDITSONGINFO, SI_LOOP, "LOOP","%04X", MAKEPTR(mused.song.loop_point), 4);
	songinfo_add_param(d);
	update_rect(&area, &r);

	d = generic_field(event, &r, EDITSONGINFO, SI_STEP, "STEP","%04X", MAKEPTR(mused.sequenceview_steps), 4);
	songinfo_add_param(d);
	update_rect(&area, &r);
}

void open_groove_editor(void* unused_1, void* unused_2, void* unused_3)
{
	mused.open_groove_settings = true;
}


void songinfo2_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain,&area, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);

	area.w = my_min(320, area.w);

	adjust_rect(&area, 2);
	SDL_Rect r;
	copy_rect(&r, &area);
	r.w = 100; //100 - 8

	if (area.w > r.w)
	{
		r.w = area.w / (int)(area.w / r.w) - 5;
	}
	else
	{
		r.w = area.w;
	}

	r.h = 10;
	console_set_clip(mused.console, &area);

	int d, tmp = r.w;

	r.w -= 34; //26

	d = generic_field(event, &r, EDITSONGINFO, SI_SPEED1, "SPD","%02X", MAKEPTR(mused.song.song_speed), 2);
	songinfo_add_param(d);

	r.x += r.w;
	r.w = 34; //26

	d = generic_field(event, &r, EDITSONGINFO, SI_SPEED2, "","%02X", MAKEPTR(mused.song.song_speed2), 2);
	songinfo_add_param(d);
	update_rect(&area, &r);

	r.w = 58;
	generic_flags(event, &r, EDITSONGINFO, SI_ENABLE_GROOVE, "GROOVE", &mused.song.flags, MUS_USE_GROOVE);

	r.w = 40;
	r.x += 58;
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "OPEN", open_groove_editor, NULL, NULL, NULL);

	r.w = tmp;
	update_rect(&area, &r);

	d = generic_field(event, &r, EDITSONGINFO, SI_RATE, "RATE", "%5d", MAKEPTR(mused.song.song_rate), 5);
	songinfo_add_param(d);
	update_rect(&area, &r);
	
	tmp = r.w;

	r.w -= 34; //26

	d = generic_field(event, &r, EDITSONGINFO, SI_TIME1, "TIME","%02X", MAKEPTR(mused.time_signature >> 8), 2);
	songinfo_add_param(d);

	r.x += r.w;
	r.w = 34; //26

	d = generic_field(event, &r, EDITSONGINFO, SI_TIME2, "","%02X", MAKEPTR( mused.time_signature & 0xff), 2);
	songinfo_add_param(d);
	update_rect(&area, &r);

	r.w = tmp;
}


void songinfo3_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain,&area, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);

	area.w = my_min(320, area.w);

	adjust_rect(&area, 2);
	SDL_Rect r;
	copy_rect(&r, &area);
	r.w = 100; //100 - 8

	if (area.w > r.w)
	{
		r.w = area.w / (int)(area.w / r.w) - 5;
	}
	else
	{
		r.w = area.w;
	}

	r.h = 10;
	console_set_clip(mused.console, &area);

	int d;

	d = generic_field(event, &r, EDITSONGINFO, SI_OCTAVE, "OCTAVE", mused.octave == 0 ? "%02X" : (mused.octave < 0 ? "-%1d" : "%02X"), MAKEPTR(mused.octave < 0 ? abs(mused.octave) : mused.octave), 2);
	songinfo_add_param(d);
	update_rect(&area, &r);

	d = generic_field(event, &r, EDITSONGINFO, SI_CHANNELS, "CHANNELS", "%02X", MAKEPTR(mused.song.num_channels), 2);
	songinfo_add_param(d);
	update_rect(&area, &r);
}


void playstop_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain,&area, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);

	area.w = my_min(PLAYSTOP_INFO_W * 2, area.w);

	adjust_rect(&area, 2);
	area.x++;
	area.w -= 2;
	SDL_Rect r;
	copy_rect(&r, &area);
	r.w = area.w;

	r.h = 10;
	console_set_clip(mused.console, &area);

	SDL_Rect button;
	copy_rect(&button, &r);
	button.w = my_max(button.w / 2, PLAYSTOP_INFO_W/2 - 4);
	button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, (mused.flags & SONG_PLAYING) ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "PLAY", play, NULL, NULL, NULL);
	button.x -= ELEMENT_MARGIN;
	update_rect(&area, &button);
	button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "STOP", stop, NULL, NULL, NULL);

	r.y = button.y + button.h;

	int d;

	if (mused.mus.cyd->flags & CYD_CLIPPING)
		d = generic_field(event, &r, EDITSONGINFO, SI_MASTERVOL, "\2 VOL","%02X", MAKEPTR(mused.song.master_volume), 2);
	else
		d = generic_field(event, &r, EDITSONGINFO, SI_MASTERVOL, "\1 VOL","%02X", MAKEPTR(mused.song.master_volume), 2);
	songinfo_add_param(d);
	update_rect(&area, &r);
}

char* my_itoa(int num, char *str)
{
	if(str == NULL)
	{
		return NULL;
	}
	sprintf(str, "%d", num);
	return str;
}


void info_line(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	area.w -= (N_VIEWS - 1 - 1) * dest->h;
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain,&area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 3);
	console_set_clip(mused.console, &area);
	console_set_color(mused.console, colors[COLOR_STATUSBAR_TEXT]);

	console_clear(mused.console);

	char text[200] = "";
	
	static char prev_text[200];

	if (mused.info_message[0] != '\0')
		strncpy(text, mused.info_message, sizeof(text) - 1);
	else
	{
		switch (mused.focus)
		{
			case EDITENVELOPE:
			{
				static const char * param_desc[] =
				{
					"Use custom volume envelope",
					"Volume envelope fadeout (sort of release rate)",
					"Volume envelope horizontal axis display scale",
					"Enable volume envelope sustain",
					"Volume envelope sustain point",
					"Enable volume envelope loop",
					"Volume envelope loop begin point",
					"Volume envelope loop end point",
					
					"Use custom panning envelope",
					"Panning envelope fadeout (sort of release rate)",
					"Panning envelope horizontal axis display scale",
					"Enable panning envelope sustain",
					"Panning envelope sustain point",
					"Enable panning envelope loop",
					"Panning envelope loop begin point",
					"Panning envelope loop end point",
				};
				
				strcpy(text, param_desc[mused.env_selected_param]);
				
				break;
			}
			
			case EDITENVELOPE4OP:
			{
				static const char * param_desc[] =
				{
					"Use custom volume envelope for current FM operator",
					"Volume envelope fadeout (sort of release rate)",
					"Volume envelope horizontal axis display scale",
					"Enable volume envelope sustain",
					"Volume envelope sustain point",
					"Enable volume envelope loop",
					"Volume envelope loop begin point",
					"Volume envelope loop end point",
				};
				
				strcpy(text, param_desc[mused.fourop_env_selected_param]);
				
				break;
			}
			
			case EDITPROG:
			case EDITPROG4OP:
			{
				Uint16 inst;
				
				if(mused.show_four_op_menu)
				{
					if(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]])
					{
						inst = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step];
						get_command_desc(text, sizeof(text) - 1, inst);
						
						strcpy(prev_text, text); //so in 4-op editor when you undo and go to prev macro there isn't 1 frame of empty info line
					}
					
					else
					{
						strcpy(text, prev_text);
					}
				}
				
				else
				{
					if(mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program])
					{
						inst = mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][mused.current_program_step];
						get_command_desc(text, sizeof(text) - 1, inst);
						
						strcpy(prev_text, text);
					}
					
					else
					{
						strcpy(text, prev_text);
					}
				}
			}
			break;

			case EDITWAVETABLE:
			{
				static const char * param_desc[] =
				{
					"Wavetable item",
					"Item name",
					"Sample rate",
					"Base note",
					"Base note finetune",
					"Interpolate",
					"Interpolation type", //wasn't there
					"Enable looping",
					"Loop begin",
					"Ping-pong looping",
					"Loop end",
					"Number of oscillators",
					"Oscillator type",
					"Frequency multiplier",
					"Phase shift",
					"Phase exponent",
					"Oscillator volume", //wasn't there
					"Absolute",
					"Negative",
					"Wave length",
					"Randomize & generate",
					"Generate",
					"Randomize",
					"Toolbox"
				};
				
				strcpy(text, param_desc[mused.wavetable_param]);
			}
			break;
			
			case EDITLOCALSAMPLE:
			{
				static const char * param_desc[] =
				{
					"Enable local samples",
					"Local/global sample",
					
					"Sample number",
					
					"Sample name",
					"Sample rate",
					"Base note",
					"Base note finetune",
					"Interpolate",
					"Interpolation type", //wasn't there
					"Enable looping",
					"Loop begin",
					"Ping-pong looping",
					"Loop end",
					
					"Connect specific samples to specific notes",
				};
				
				strcpy(text, param_desc[mused.local_sample_param]);
			}
			break;

			case EDITINSTRUMENT:
			{
				static const char * param_desc[] =
				{
					"Select instrument",
					"Edit instrument name",
					"Base note",
					"Finetune",
					"Lock to base note",
					"Drum (short burst of noise in the beginning)",
					"Sync oscillator on keydown",
					"Reverse vibrato bit",
					"Reverse tremolo bit",
					"Set PW on keydown",
					"Set cutoff on keydown",
					"Slide speed",
					"Pulse wave",
					"Pulse width",
					"Sawtooth wave",
					"Triangle wave",
					"Noise",
					"Metallic noise (shortens noise cycle)",
					"LFSR enable",
					"LFSR type",
					"Quarter frequency",
					"Sine wave", //wasn't there
					"Sine wave phase shift", //wasn't there
					"Lock noise pitch", //wasn't there
					"Constant noise note", //wasn't there
					"Enable 1-bit noise (as on NES/Gameboy)", //wasn't there
					"Wavetable",
					"Wavetable entry",
					"Override volume envelope for wavetable",
					"Lock wave to base note",
					"Oscillators mix mode", //wasn't there
					"Volume",
					"Relative volume commands",
					"Envelope attack",
					"Envelope decay",
					"Envelope sustain",
					"Envelope release",
					"Enable exponential volume", //wasn't there
					"Enable exponential attack", //wasn't there
					"Enable exponential decay", //wasn't there
					"Enable exponential release", //wasn't there
					"Enable volume key scaling", //wasn't there
					"Volume key scaling level", //wasn't there
					"Enable envelope key scaling", //wasn't there
					"Envelope key scaling level", //wasn't there
					"Buzz (AY/YM volume envelope)",
					"Buzz semitone",
					"Buzz shape",
					"Buzz toggle AY8930 mode", //wasn't there
					"Buzz fine",
					"Sync channel",
					"Sync master channel",
					"Ring modulation",
					"Ring modulation source",
					"Enable filter",
					"Filter type",
					"Filter cutoff frequency",
					"Filter resonance",
					"Filter slope",
					
					"Current instrument program", //wasn't there
					"Selected program period",
					
					"Send signal to FX chain",
					"FX bus",
					"Vibrato speed",
					"Vibrato depth",
					"Vibrato shape",
					"Vibrato delay",
					"Pulse width modulation speed",
					"Pulse width modulation depth",
					"Pulse width modulation shape",
					"Pulse width modulation delay", //wasn't there
					"Tremolo speed", //wasn't there
					"Tremolo depth", //wasn't there
					"Tremolo shape", //wasn't there
					"Tremolo delay", //wasn't there
					
					"Don't restart program on keydown",
					"Enable multi oscillator (play 2- or 3-note chords by using 00XY command in pattern)",
					"Save vibrato, PWM and tremolo settings", //wasn't there
					"FM enable",
					"FM modulation",
					"Enable FM modulator volume key scaling", //wasn't there
					"FM modulator volume key scaling level", //wasn't there
					"Enable FM modulator envelope key scaling", //wasn't there
					"FM modulator envelope key scaling level", //wasn't there
					"FM feedback",
					"FM modulator frequency divider",
					"FM modulator frequency multiplier",
					"FM modulator base note", //wasn't there
					"FM modulator finetune", //wasn't there
					"FM frequency multiplier mapping", //wasn't there
					"FM attack",
					"FM decay",
					"FM sustain",
					"FM release",
					"Enable FM modulator exponential volume", //wasn't there
					"Enable FM modulator exponential attack", //wasn't there
					"Enable FM modulator exponential decay", //wasn't there
					"Enable FM modulator exponential release", //wasn't there
					"FM env start (0=beginning, 1F=end of attack phase)",
					"FM use wavetable",
					"FM wavetable entry",
					"FM modulator vibrato speed", //wasn't there
					"FM modulator vibrato depth", //wasn't there
					"FM modulator vibrato shape", //wasn't there
					"FM modulator vibrato delay", //wasn't there
					"FM modulator tremolo speed", //wasn't there
					"FM modulator tremolo depth", //wasn't there
					"FM modulator tremolo shape", //wasn't there
					"FM modulator tremolo delay", //wasn't there
					"FM enable additive synth mode", //wasn't there
					"Save FM modulator vibrato and tremolo settings", //wasn't there
					"FM enable 4-operator mode (independent from these FM settings)", //wasn't there

					"Enable phase reset timer", //wasn't there
					"Phase reset timer note", //wasn't there
					"Phase reset timer finetune", //wasn't there
					"Make phase reset timer note dependent on instrument note", //wasn't there
				};
				
				static const char * mixmodes[] =
				{
					"bitwise AND",
					"sum of oscillators' signals",
					"bitwise OR",
					"C64 8580 SID combined waves bitwise ANDed with other signals",
					"bitwise exclusive OR",
					//"multiplication of oscillators' signals",
				};
				
				static const char * filter_types[] =
				{
					"lowpass filter",
					"highpass filter",
					"bandpass filter",
					"lowpass + highpass (signal sum)",
					"highpass + bypass (signal sum)",
					"lowpass + bypass (signal sum)",
					"lowpass + highpass + bypass (signal sum)"
				};

				if (mused.selected_param == P_FXBUS)
					snprintf(text, sizeof(text) - 1, "%s (%s)", param_desc[mused.selected_param], mused.song.fx[mused.song.instrument[mused.current_instrument].fx_bus].name);
				else if (mused.selected_param == P_WAVE_ENTRY)
					snprintf(text, sizeof(text) - 1, "%s (%s)", param_desc[mused.selected_param], mused.song.wavetable_names[mused.song.instrument[mused.current_instrument].wavetable_entry]);
				else if (mused.selected_param == P_FM_WAVE_ENTRY)
					snprintf(text, sizeof(text) - 1, "%s (%s)", param_desc[mused.selected_param], mused.song.wavetable_names[mused.song.instrument[mused.current_instrument].fm_wave]);
				else if (mused.selected_param == P_VOLUME)
					snprintf(text, sizeof(text) - 1, "%s (%+.1f dB)", param_desc[mused.selected_param], percent_to_dB((float)mused.song.instrument[mused.current_instrument].volume / MAX_VOLUME));
				else if (mused.selected_param == P_ATTACK)
					snprintf(text, sizeof(text) - 1, "%s (%.1f ms)", param_desc[mused.selected_param], envelope_length(mused.song.instrument[mused.current_instrument].adsr.a));
				else if (mused.selected_param == P_DECAY)
					snprintf(text, sizeof(text) - 1, "%s (%.1f ms)", param_desc[mused.selected_param], envelope_length(mused.song.instrument[mused.current_instrument].adsr.d));
				else if (mused.selected_param == P_RELEASE)
					snprintf(text, sizeof(text) - 1, "%s (%.1f ms)", param_desc[mused.selected_param], envelope_length(mused.song.instrument[mused.current_instrument].adsr.r));
				else if (mused.selected_param == P_FM_ATTACK)
					snprintf(text, sizeof(text) - 1, "%s (%.1f ms)", param_desc[mused.selected_param], envelope_length(mused.song.instrument[mused.current_instrument].fm_adsr.a));
				else if (mused.selected_param == P_FM_DECAY)
					snprintf(text, sizeof(text) - 1, "%s (%.1f ms)", param_desc[mused.selected_param], envelope_length(mused.song.instrument[mused.current_instrument].fm_adsr.d));
				else if (mused.selected_param == P_FM_RELEASE)
					snprintf(text, sizeof(text) - 1, "%s (%.1f ms)", param_desc[mused.selected_param], envelope_length(mused.song.instrument[mused.current_instrument].fm_adsr.r));
				
				else if (mused.selected_param == P_CUTOFF) //wasn't there
					snprintf(text, sizeof(text) - 1, "%s (%d Hz)", param_desc[mused.selected_param], (mused.song.instrument[mused.current_instrument].cutoff * 20000) / 4096 + 5);
				else if (mused.selected_param == P_FLTTYPE) //wasn't there
					snprintf(text, sizeof(text) - 1, "%s (%s)", param_desc[mused.selected_param], filter_types[mused.song.instrument[mused.current_instrument].flttype]);
				else if (mused.selected_param == P_OSCMIXMODE) //wasn't there
					snprintf(text, sizeof(text) - 1, "%s (%s)", param_desc[mused.selected_param], mixmodes[mused.song.instrument[mused.current_instrument].mixmode]);
				
				else
					strcpy(text, param_desc[mused.selected_param]);
			}

			break;
			
			case EDIT4OP:
			{
				static const char * param_desc[] =
				{
					"Enable 3CH_EXP mode (independent freq. per op.)",
					"4-op system algorithm",
					
					"4-op master volume",
					"Use main instrument program to control operators",
					"4-op output bypasses main instrument filter",
					
					"base note",
					"finetune",
					"lock to base note",
					
					"frequency divider",
					"frequency multiplier",
					"detune",
					"coarse detune (OPM chip only, DT2)",
					
					"feedback",
					"enable SSG-EG",
					"SSG-EG mode",
					
					"drum (short burst of noise in the beginning)",
					"sync oscillator on keydown",
					"reverse vibrato bit",
					"reverse tremolo bit",
					"set PW on keydown",
					"set cutoff on keydown",
					"slide speed",
					"pulse wave",
					"pulse width",
					"sawtooth wave",
					"triangle wave",
					"noise",
					"metallic noise (shortens noise cycle)",
					"quarter frequency",
					"sine wave", //wasn't there
					"sine wave phase shift", //wasn't there
					"lock noise pitch", //wasn't there
					"constant noise note", //wasn't there
					"enable 1-bit noise (as on NES/Gameboy)", //wasn't there
					"wavetable",
					"wavetable entry",
					"override volume envelope for wavetable",
					"lock wave to base note",
					"oscillators mix mode", //wasn't there
					"volume",
					"relative volume commands",
					"envelope attack",
					"envelope decay",
					"envelope sustain level",
					"envelope sustain rate",
					"envelope release",
					"enable exponential volume", //wasn't there
					"enable exponential attack", //wasn't there
					"enable exponential decay", //wasn't there
					"enable exponential release", //wasn't there
					"enable volume key scaling", //wasn't there
					"volume key scaling level", //wasn't there
					"enable envelope key scaling", //wasn't there
					"envelope key scaling level", //wasn't there
					"sync channel",
					"sync master channel",
					"ring modulation",
					"ring modulation source",
					"enable filter",
					"filter type",
					"filter cutoff frequency",
					"filter resonance",
					"filter slope",
					
					"current program",
					"selected program period",
					
					"vibrato speed",
					"vibrato depth",
					"vibrato shape",
					"vibrato delay",
					"pulse width modulation speed",
					"pulse width modulation depth",
					"pulse width modulation shape",
					"pulse width modulation delay", //wasn't there
					"tremolo speed", //wasn't there
					"tremolo depth", //wasn't there
					"tremolo shape", //wasn't there
					"tremolo delay", //wasn't there
					
					"don't restart program on keydown",
					"save vibrato, PWM and tremolo settings", //wasn't there
					"trigger delay (in ticks)",
					"envelope offset (0=beginning, FF=end of attack phase)",
					"enable CSM timer (phase reset, envelope reset to release)",
					"CSM timer note",
					"CSM timer finetune",
					"make CSM timer note dependent on FM op note",

					"enable phase reset timer", //wasn't there
					"phase reset timer note", //wasn't there
					"phase reset timer finetune", //wasn't there
					"make phase reset timer note dependent on instrument note", //wasn't there
				};
				
				static const char * mixmodes[] =
				{
					"bitwise AND",
					"sum of oscillators' signals",
					"bitwise OR",
					"C64 8580 SID combined waves bitwise ANDed with other signals",
					"bitwise exclusive OR",
					//"multiplication of oscillators' signals",
				};
				
				static const char * filter_types[] =
				{
					"lowpass filter",
					"highpass filter",
					"bandpass filter",
					"lowpass + highpass (signal sum)",
					"highpass + bypass (signal sum)",
					"lowpass + bypass (signal sum)",
					"lowpass + highpass + bypass (signal sum)"
				};
				
				static const char * ssg_eg_types[] = { "\\\\\\\\", "\\___", "\\/\\/", "\\\xfd\xfd\xfd", "////", "/\xfd\xfd\xfd", "/\\/\\", "/___" };
				
				//\xfd_/\\
				
				Uint32 p = 0x7F800000; //a hack to display infinity

				if (mused.fourop_selected_param == FOUROP_WAVE_ENTRY)
					snprintf(text, sizeof(text) - 1, "Operator %d %s (%s)", mused.selected_operator, param_desc[mused.fourop_selected_param], mused.song.wavetable_names[mused.song.instrument[mused.current_instrument].wavetable_entry]);
				else if (mused.fourop_selected_param == FOUROP_VOLUME)
					snprintf(text, sizeof(text) - 1, "Operator %d %s (%+.1f dB)", mused.selected_operator, param_desc[mused.fourop_selected_param], percent_to_dB((float)mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume / MAX_VOLUME));
				else if (mused.fourop_selected_param == FOUROP_ATTACK)
					snprintf(text, sizeof(text) - 1, "Operator %d %s (%.1f ms)", mused.selected_operator, param_desc[mused.fourop_selected_param], envelope_length(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].adsr.a));
				else if (mused.fourop_selected_param == FOUROP_DECAY)
					snprintf(text, sizeof(text) - 1, "Operator %d %s (%.1f ms)", mused.selected_operator, param_desc[mused.fourop_selected_param], envelope_length(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].adsr.d));
				
				else if (mused.fourop_selected_param == FOUROP_SUSTAIN_RATE)
					snprintf(text, sizeof(text) - 1, "Operator %d %s (%.1f ms)", mused.selected_operator, param_desc[mused.fourop_selected_param], mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].adsr.sr == 0 ? *(float *)&p : envelope_length((0xFF - mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].adsr.sr)));
				
				else if (mused.fourop_selected_param == FOUROP_RELEASE)
					snprintf(text, sizeof(text) - 1, "Operator %d %s (%.1f ms)", mused.selected_operator, param_desc[mused.fourop_selected_param], envelope_length(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].adsr.r));
				
				else if (mused.fourop_selected_param == FOUROP_CUTOFF) //wasn't there
					snprintf(text, sizeof(text) - 1, "Operator %d %s (%d Hz)", mused.selected_operator, param_desc[mused.fourop_selected_param], (mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].cutoff * 20000) / 4096 + 5);
				else if (mused.fourop_selected_param == FOUROP_FLTTYPE) //wasn't there
					snprintf(text, sizeof(text) - 1, "Operator %d %s (%s)", mused.selected_operator, param_desc[mused.fourop_selected_param], filter_types[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].flttype]);
				else if (mused.fourop_selected_param == FOUROP_OSCMIXMODE) //wasn't there
					snprintf(text, sizeof(text) - 1, "Operator %d %s (%s)", mused.selected_operator, param_desc[mused.fourop_selected_param], mixmodes[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].mixmode]);
				else if (mused.fourop_selected_param == FOUROP_SSG_EG_TYPE) //wasn't there
					snprintf(text, sizeof(text) - 1, "Operator %d %s (%s)", mused.selected_operator, param_desc[mused.fourop_selected_param], ssg_eg_types[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].ssg_eg_type]);
				else if (mused.fourop_selected_param == FOUROP_ALG || mused.fourop_selected_param == FOUROP_3CH_EXP_MODE || mused.fourop_selected_param == FOUROP_USE_MAIN_INST_PROG || mused.fourop_selected_param == FOUROP_MASTER_VOL || mused.fourop_selected_param == FOUROP_BYPASS_MAIN_INST_FILTER) //wasn't there
					snprintf(text, sizeof(text) - 1, "%s", param_desc[mused.fourop_selected_param]);
				
				else
				{
					snprintf(text, sizeof(text) - 1, "Operator %d %s", mused.selected_operator, param_desc[mused.fourop_selected_param]);
				}
			}

			break;

			case EDITFX:
			{
				static const char * param_desc[] =
				{
					"Enable multiplex",
					"Multiplex period",
					"Pitch inaccuracy",
					"FX bus",
					"FX bus name",
					"Enable bitcrusher",
					"Drop bits (reduces bit depth)",
					"Downsample (reduces sample rate)",
					"Dither",
					"Crush gain",
					"Enable stereo chorus",
					"Min. delay",
					"Max. delay",
					"Phase separation",
					"Modulation frequency",
					"Enable reverb",
					"Room size",
					"Reverb volume",
					"Decay",
					"Snap taps to ticks",
					"Tap enabled",
					"Selected tap",
					"Tap delay",
					"Tap gain",
					"Amount of taps",
					"Tap panning",
				};

				strcpy(text, param_desc[mused.edit_reverb_param]);
			} break;

			case EDITPATTERN:
				if (get_current_pattern())
				{
					if (mused.current_patternx >= PED_COMMAND1)
					{
						Uint16 inst = mused.song.pattern[current_pattern()].step[current_patternstep()].command[(mused.current_patternx - PED_COMMAND1) / 4];

						if (inst != 0)
							get_command_desc(text, sizeof(text), inst);
						else
						{
							char buf[4];
							strcpy(text, "Command "); //was strcpy(text, "Command");
							
							strcat(text, my_itoa((mused.current_patternx - PED_COMMAND1) / 4 + 1, buf));
						}
					}
					
					else if (mused.current_patternx == PED_VOLUME1 || mused.current_patternx == PED_VOLUME2)
					{
						Uint16 vol = mused.song.pattern[current_pattern()].step[current_patternstep()].volume;

						if (vol != MUS_NOTE_NO_VOLUME && vol > MAX_VOLUME)
						{
							switch (vol & 0xf0)
							{
								case MUS_NOTE_VOLUME_FADE_UP:
									strcpy(text, "Fade volume up");
									break;

								case MUS_NOTE_VOLUME_FADE_DN:
									strcpy(text, "Fade volume down");
									break;
								
								case MUS_NOTE_VOLUME_FADE_UP_FINE:
									strcpy(text, "Fine fade volume up");
									break;

								case MUS_NOTE_VOLUME_FADE_DN_FINE:
									strcpy(text, "Fine fade volume down");
									break;

								case MUS_NOTE_VOLUME_PAN_LEFT:
									strcpy(text, "Pan left");
									break;

								case MUS_NOTE_VOLUME_PAN_RIGHT:
									strcpy(text, "Pan right");
									break;

								case MUS_NOTE_VOLUME_SET_PAN:
									strcpy(text, "Set panning");
									break;
							}
						}
						
						else if (vol == MUS_NOTE_NO_VOLUME)
							strcpy(text, "Volume");
						else
							sprintf(text, "Volume (%+.1f dB)", percent_to_dB((float)vol / MAX_VOLUME));
					}
					
					else if (mused.current_patternx == PED_NOTE)
					{
						Uint8 note = mused.song.pattern[current_pattern()].step[current_patternstep()].note;

						switch(note)
						{
							case MUS_NOTE_RELEASE:
							{
								sprintf(text, "Trigger release"); break;
							}
							
							case MUS_NOTE_CUT:
							{
								sprintf(text, "Note cut"); break;
							}
							
							case MUS_NOTE_MACRO_RELEASE:
							{
								sprintf(text, "Trigger instrument macro(s) release"); break;
							}
							
							case MUS_NOTE_RELEASE_WITHOUT_MACRO:
							{
								sprintf(text, "Trigger only instrument note (without macro(s)) release"); break;
							}
							
							default:
							{
								sprintf(text, "Note"); break;
							}
						}
					}
					
					else
					{
						static const char *pattern_txt[] =
						{
							//"Note", "Instrument", "Instrument", "", "", "Legato", "Slide", "Vibrato", "Tremolo"
							"", "Instrument", "Instrument", "", "", "Legato", "Slide", "Vibrato", "Tremolo"
						};

						strcpy(text, pattern_txt[mused.current_patternx]);
					}
				}
			break;
		}
	}

	console_write(mused.console, text);
	
	int m = mused.mode >= VIRTUAL_MODE ? mused.prev_mode : mused.mode;
	
	if((m == EDITCLASSIC || m == EDITPATTERN || m == EDITSEQUENCE || m == EDITSONGINFO) && (mused.flags2 & SHOW_BPM))
	{
		float bpm = 0;
		
		if(mused.mus.flags & MUS_ENGINE_USE_GROOVE)
		{
			float av_speed = 1.0;
			
			if(mused.song.groove_length[mused.mus.groove_number] > 0)
			{
				for(int i = 0; i < mused.song.groove_length[mused.mus.groove_number]; i++)
				{
					av_speed += (float)mused.song.grooves[mused.mus.groove_number][i];
				}
				
				av_speed /= (float)mused.song.groove_length[mused.mus.groove_number];
			}
			
			bpm = (float)3600 / (av_speed / (float)mused.song.song_rate * (float)(mused.time_signature & 0xff) * (float)60);
		}
		
		else
		{
			bpm = (float)3600 / ((float)(mused.song.song_speed + mused.song.song_speed2) / (float)2 / (float)mused.song.song_rate * (float)(mused.time_signature & 0xff) * (float)60);
		}
		
		int offset = 8 * 9 - 3 + (bpm >= 10.0 ? 8 : 0) +  (bpm >= 100.0 ? 8 : 0) + (bpm >= 1000.0 ? 8 : 0) + (bpm >= 10000.0 ? 8 : 0) + (bpm >= 100000.0 ? 8 : 0);
		
		SDL_Rect bpm_value = { area.w - offset, area.y, offset, area.h };
		
		console_set_clip(mused.console, &bpm_value);
		console_clear(mused.console);
		console_set_clip(mused.console, &bpm_value);
		console_set_color(mused.console, colors[COLOR_STATUSBAR_TEXT]);
		console_clear(mused.console);
		
		char bpm_string[30];
		
		snprintf(bpm_string, 30, " %0.2f BPM", bpm);
		
		console_write(mused.console, bpm_string);
	}

	SDL_Rect button = { dest->x + area.w + 6, dest->y, dest->h, dest->h };
	
	button_event(domain, event, &button, mused.slider_bevel,
		(mused.mode != EDITPATTERN) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		(mused.mode != EDITPATTERN) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		DECAL_MODE_PATTERN + EDITPATTERN + (mused.mode == EDITPATTERN ? DECAL_MODE_PATTERN_SELECTED - DECAL_MODE_PATTERN : 0), (mused.mode != EDITPATTERN) ? change_mode_action : NULL, (mused.mode != EDITPATTERN) ? MAKEPTR(EDITPATTERN) : 0, 0, 0);
	
	button.x += button.w;
	
	button_event(domain, event, &button, mused.slider_bevel,
		(mused.mode != EDITSEQUENCE) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		(mused.mode != EDITSEQUENCE) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		DECAL_MODE_PATTERN + EDITSEQUENCE + (mused.mode == EDITSEQUENCE ? DECAL_MODE_PATTERN_SELECTED - DECAL_MODE_PATTERN : 0), (mused.mode != EDITSEQUENCE) ? change_mode_action : NULL, (mused.mode != EDITSEQUENCE) ? MAKEPTR(EDITSEQUENCE) : 0, 0, 0);
	
	button.x += button.w;
	
	button_event(domain, event, &button, mused.slider_bevel,
		(mused.mode != EDITCLASSIC) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		(mused.mode != EDITCLASSIC) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		DECAL_MODE_PATTERN + EDITCLASSIC + (mused.mode == EDITCLASSIC ? DECAL_MODE_PATTERN_SELECTED - DECAL_MODE_PATTERN : 0), (mused.mode != EDITCLASSIC) ? change_mode_action : NULL, (mused.mode != EDITCLASSIC) ? MAKEPTR(EDITCLASSIC) : 0, 0, 0);
	
	button.x += button.w;
	
	button_event(domain, event, &button, mused.slider_bevel,
		(mused.mode != EDITINSTRUMENT && mused.mode != EDIT4OP) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		(mused.mode != EDITINSTRUMENT && mused.mode != EDIT4OP) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		DECAL_MODE_PATTERN + EDITINSTRUMENT + ((mused.mode == EDITINSTRUMENT || mused.mode == EDIT4OP) ? DECAL_MODE_PATTERN_SELECTED - DECAL_MODE_PATTERN : 0), (mused.mode != EDITINSTRUMENT) ? change_mode_action : NULL, (mused.mode != EDITINSTRUMENT) ? MAKEPTR(EDITINSTRUMENT) : 0, 0, 0);
	
	button.x += button.w;
	
	button_event(domain, event, &button, mused.slider_bevel,
		(mused.mode != EDITFX) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		(mused.mode != EDITFX) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		DECAL_MODE_PATTERN + EDITFX - 1 + (mused.mode == EDITFX ? DECAL_MODE_PATTERN_SELECTED - DECAL_MODE_PATTERN : 0), (mused.mode != EDITFX) ? change_mode_action : NULL, (mused.mode != EDITFX) ? MAKEPTR(EDITFX) : 0, 0, 0);
	
	button.x += button.w;
	
	button_event(domain, event, &button, mused.slider_bevel,
		(mused.mode != EDITWAVETABLE) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		(mused.mode != EDITWAVETABLE) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		DECAL_MODE_PATTERN + EDITWAVETABLE - 1 + (mused.mode == EDITWAVETABLE ? DECAL_MODE_PATTERN_SELECTED - DECAL_MODE_PATTERN : 0), (mused.mode != EDITWAVETABLE) ? change_mode_action : NULL, (mused.mode != EDITWAVETABLE) ? MAKEPTR(EDITWAVETABLE) : 0, 0, 0);
	
	button.x += button.w;
	
	if(mused.open_groove_settings)
	{
		groove_view();
		mused.open_groove_settings = false;
	}
	
	if(mused.import_wavetable_string)
	{
		import_wavetable_string(&mused.song.instrument[mused.current_instrument]);
		
		mused.import_wavetable_string = false;
	}

	if(mused.open_song_message)
	{
		//open_songmessage(0, 0, 0);
		song_message_view(domain, mused.slider_bevel, &mused.largefont, &mused.smallfont);
		mused.open_song_message = false;
	}
}

Uint32 char_to_hex(char c)
{
	const char numbers[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	
	for(int i = 0; i < 16; ++i)
	{
		if(c == numbers[i])
		{
			return i;
		}
	}
	
	return 0;
}

static void write_command(const SDL_Event *event, const char *text, int cmd_idx, int cur_idx, bool highlight_cursor)
{
	int i = 0;
	
	Uint32 init_color = mused.console->current_color;

	for (const char *c = text; *c; ++c, ++i)
	{
		const SDL_Rect *r;
		
		if(mused.flags2 & HIGHLIGHT_COMMANDS)
		{
			Uint16 command = (char_to_hex(text[0]) << 12) + (char_to_hex(text[1]) << 8) + (char_to_hex(text[2]) << 4) + char_to_hex(text[3]);
			
			if(((~(get_instruction_mask(command))) & (0xf << ((3 - i) * 4))) && is_valid_command(command) && text[0] != '.' && (command & 0xff00) != MUS_FX_LABEL && (command & 0xff00) != MUS_FX_RELEASE_POINT && command != MUS_FX_END)
			{
				Uint32 color = init_color;
				Uint32 highlight_color = ((color & 0xff) * 1 / 2) + (((((color >> 8) & 0xff) * 1 / 2) & 0xff) << 8) + 0xff0000;//my_min(0xff0000, (((((color >> 16) & 0xff) * 7 / 4) & 0xff) << 16));
				
				if(cmd_idx == cur_idx && mused.focus == (mused.show_four_op_menu ? EDITPROG4OP : EDITPROG))
				{
					highlight_color = 0x00ee00;
				}
				
				if(highlight_cursor)
				{
					highlight_color = 0x00dddd;
				}
				
				console_set_color(mused.console, highlight_color);
			}
			
			else
			{
				console_set_color(mused.console, init_color);
			}
		}
		
		if(mused.focus != EDITPROG)
		{
			mused.jump_in_program = true;
		}
		
		if(mused.focus != EDITPROG4OP)
		{
			mused.jump_in_program_4op = true;
		}
		
		if(mused.jump_in_program && mused.jump_in_program_4op)
		{
			check_event(event, r = console_write_args(mused.console, "%c", *c), select_program_step, MAKEPTR(cmd_idx), MAKEPTR(i), 0);
		}
		
		else
		{
			check_event(event, r = console_write_args(mused.console, "%c", *c), NULL, NULL, NULL, NULL);
		}

		if (mused.focus == (mused.show_four_op_menu ? EDITPROG4OP : EDITPROG) && mused.editpos == i && cmd_idx == cur_idx)
		{
			SDL_Rect cur;
			copy_rect(&cur, r);
			adjust_rect(&cur, -2);
			set_cursor(&cur);
		}
	}
	
	console_set_color(mused.console, init_color);
}


void program_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	//debug("program_view");
	
	if(mused.show_point_envelope_editor) return;
	
	if(!(mused.show_four_op_menu) || ((mused.focus != EDIT4OP) && (mused.focus != EDITPROG4OP)))
	{
		//debug("program_view");
		
		SDL_Rect area, clip;
		copy_rect(&area, dest);
		
		if(mused.show_fx_lfo_settings)
		{
			area.y -= 60;
			area.h += 60;
		}

		if(mused.show_timer_lfo_settings)
		{
			area.y -= 130;
			area.h += 130;
		}
		
		console_set_clip(mused.console, &area);
		console_clear(mused.console);
		bevelex(domain,&area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
		adjust_rect(&area, 2);
		copy_rect(&clip, &area);
		adjust_rect(&area, 1);
		area.w = 4000;
		console_set_clip(mused.console, &area);
		
		MusInstrument *inst = &mused.song.instrument[mused.current_instrument];
		
		if(inst->program_unite_bits[mused.current_instrument_program] == NULL)
		{
			while(inst->program_unite_bits[mused.current_instrument_program] == NULL)
			{
				mused.current_instrument_program--;
			}
		}
		
		if(inst->program_unite_bits[mused.current_instrument_program] == NULL) return;
		if(inst->program[mused.current_instrument_program] == NULL) return;
		
		//separator("----program-----");
		
		int start = mused.program_position;
		
		int pos = 0, prev_pos = -1;
		int selection_begin = -1, selection_end = -1;
		
		for (int i = 0; i < start; ++i)
		{
			prev_pos = pos;
			if (!(inst->program_unite_bits[mused.current_instrument_program][i / 8] & (1 << (i & 7))) || (inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_JUMP || (inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_LABEL || (inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_LOOP) ++pos; //old command if (!(inst->program[i] & 0x8000) || (inst->program[i] & 0xf000) == 0xf000) ++pos;
		}

		gfx_domain_set_clip(domain, &clip);

		for (int i = start, s = 0, y = 0; i < MUS_PROG_LEN && y < area.h; ++i, ++s, y += mused.console->font.h)
		{
			SDL_Rect row = { area.x - 1, area.y + y - 1, area.w + 2, mused.console->font.h + 1};
			SDL_Rect row1 = { row.x + 8 * 6, row.y, 8 * 5, row.h};

			if (mused.current_program_step == i && mused.focus == EDITPROG)
			{
				bevel(domain, &row, mused.slider_bevel, BEV_SELECTED_PATTERN_ROW);
				console_set_color(mused.console, colors[COLOR_PROGRAM_SELECTED]);
			}
			
			else
				console_set_color(mused.console, pos & 1 ? colors[COLOR_PROGRAM_ODD] : colors[COLOR_PROGRAM_EVEN]);
			
			if (i == mused.selection.start)
			{
				selection_begin = row.y;
			}
			
			if (i == mused.selection.end - 1)
			{
				selection_end = row.y + row.h + 1;
			}
			
			char box[6], cur = ' ';
			
			bool pointing_at_command = false;
			
			for (int c = 0; c < CYD_MAX_CHANNELS; ++c)
			{
				if (mused.channel[c].instrument == inst && ((mused.cyd.channel[c].flags & CYD_CHN_ENABLE_GATE) || ((mused.cyd.channel[c].flags & CYD_CHN_ENABLE_FM) && (mused.cyd.channel[c].fm.adsr.envelope != 0) && !(mused.cyd.channel[c].flags & CYD_CHN_ENABLE_GATE))) && (mused.channel[c].program_flags & (1 << mused.current_instrument_program)) && mused.channel[c].program_tick[mused.current_instrument_program] == i)
				{
					cur = '½'; //where arrow pointing at current instrument program step is drawn
					pointing_at_command = true;
				}
			}
			
			if (inst->program[mused.current_instrument_program][i] == MUS_FX_NOP)
			{
				strcpy(box, "....");
			}
			
			else
			{
				sprintf(box, "%04X", inst->program[mused.current_instrument_program][i]); //old command sprintf(box, "%04X", ((inst->program[i] & 0xf000) != 0xf000) ? (inst->program[i] & 0x7fff) : inst->program[i]);
			}
			
			Uint32 temp_color = mused.console->current_color;
			Uint32 highlight_color;
			
			if((inst->program[mused.current_instrument_program][mused.current_program_step] & 0xff00) == MUS_FX_JUMP && inst->program[mused.current_instrument_program][mused.current_program_step] != MUS_FX_NOP && (mused.flags2 & HIGHLIGHT_COMMANDS) && (inst->program[mused.current_instrument_program][mused.current_program_step] & 0xff) == i && mused.focus == EDITPROG)
			{
				highlight_color = 0x00ee00;
				console_set_color(mused.console, highlight_color);
			}

			if (pos == prev_pos)
			{
				if(mused.flags2 & DRAG_SELECT_PROGRAM)
				{
					if(mused.jump_in_program)
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%02X%c   ", i, cur), select_program_step, MAKEPTR(i), 0, 0);
					}
					
					else
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%02X%c   ", i, cur), NULL, NULL, NULL, NULL);
					}
				}
				
				else
				{
					check_event(event, console_write_args(mused.console, "%02X%c   ", i, cur), select_program_step, MAKEPTR(i), 0, 0);
				}
				
				bool highlight_united = false;
				
				if(mused.flags2 & HIGHLIGHT_COMMANDS)
				{
					console_set_color(mused.console, temp_color);
					
					if((inst->program_unite_bits[mused.current_instrument_program][my_max(i - 1, 0) / 8] & (1 << (my_max(i - 1, 0) & 7))))
					{
						for(int q = i - 1; ((inst->program_unite_bits[mused.current_instrument_program][my_max(q, 0) / 8] & (1 << (my_max(q, 0) & 7)))) && (q >= 0); --q)
						{
							for (int c = 0; c < CYD_MAX_CHANNELS; ++c)
							{
								if (mused.channel[c].instrument == inst && ((mused.cyd.channel[c].flags & CYD_CHN_ENABLE_GATE) || ((mused.cyd.channel[c].flags & CYD_CHN_ENABLE_FM) && (mused.cyd.channel[c].fm.adsr.envelope != 0) && !(mused.cyd.channel[c].flags & CYD_CHN_ENABLE_GATE))) && (mused.channel[c].program_flags & (1 << mused.current_instrument_program)) && mused.channel[c].program_tick[mused.current_instrument_program] == q)
								{
									highlight_united = true;
									break;
								}
							}
						}
					}
				}
				
				write_command(event, box, i, mused.current_program_step, pointing_at_command || highlight_united);
				
				if(mused.flags2 & DRAG_SELECT_PROGRAM)
				{
					if(mused.jump_in_program)
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%c ", (!(inst->program_unite_bits[mused.current_instrument_program][i / 8] & (1 << (i & 7)))) ? '\xd' : '|'), select_program_step, MAKEPTR(i), 0, 0);
					}
					
					else
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%c ", (!(inst->program_unite_bits[mused.current_instrument_program][i / 8] & (1 << (i & 7)))) ? '\xd' : '|'), NULL, NULL, NULL, NULL);
					}
				}
				
				else
				{
					check_event(event, console_write_args(mused.console, "%c ", (!(inst->program_unite_bits[mused.current_instrument_program][i / 8] & (1 << (i & 7)))) ? '\xd' : '|'), select_program_step, MAKEPTR(i), 0, 0);
				}
			}
			
			else
			{
				if(mused.flags2 & DRAG_SELECT_PROGRAM)
				{
					if(mused.jump_in_program)
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%02X%c%02X ", i, cur, pos), select_program_step, MAKEPTR(i), 0, 0);
					}
					
					else
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%02X%c%02X ", i, cur, pos), NULL, NULL, NULL, NULL);
					}
				}
				
				else
				{
					check_event(event, console_write_args(mused.console, "%02X%c%02X ", i, cur, pos), select_program_step, MAKEPTR(i), 0, 0);
				}
				
				bool highlight_united = false;
				
				if(mused.flags2 & HIGHLIGHT_COMMANDS)
				{
					console_set_color(mused.console, temp_color);
					
					if(inst->program_unite_bits[mused.current_instrument_program][(i) / 8] & (1 << ((i) & 7)))
					{
						for(int q = i; (inst->program_unite_bits[mused.current_instrument_program][q / 8] & (1 << (q & 7))) && (q >= 0); --q)
						{
							for (int c = 0; c < CYD_MAX_CHANNELS; ++c)
							{
								if (mused.channel[c].instrument == inst && ((mused.cyd.channel[c].flags & CYD_CHN_ENABLE_GATE) || ((mused.cyd.channel[c].flags & CYD_CHN_ENABLE_FM) && (mused.cyd.channel[c].fm.adsr.envelope != 0) && !(mused.cyd.channel[c].flags & CYD_CHN_ENABLE_GATE))) && (mused.channel[c].program_flags & (1 << mused.current_instrument_program)) && mused.channel[c].program_tick[mused.current_instrument_program] == q)
								{
									highlight_united = true;
									break;
								}
							}
						}
					}
				}
				
				write_command(event, box, i, mused.current_program_step, pointing_at_command || highlight_united);
				
				if(mused.flags2 & DRAG_SELECT_PROGRAM)
				{
					if(mused.jump_in_program)
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%c ", ((inst->program_unite_bits[mused.current_instrument_program][i / 8] & (1 << (i & 7)))) ? '`' : ' '), //old command check_event(event, console_write_args(mused.console, "%c ", ((inst->program[i] & 0x8000) && (inst->program[i] & 0xf000) != 0xf000) ? '`' : ' '),
							select_program_step, MAKEPTR(i), 0, 0);
					}
					
					else
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%c ", ((inst->program_unite_bits[mused.current_instrument_program][i / 8] & (1 << (i & 7)))) ? '`' : ' '), NULL, NULL, NULL, NULL);
					}
				}
				
				else
				{
					check_event(event, console_write_args(mused.console, "%c ", ((inst->program_unite_bits[mused.current_instrument_program][i / 8] & (1 << (i & 7)))) ? '`' : ' '), //old command check_event(event, console_write_args(mused.console, "%c ", ((inst->program[i] & 0x8000) && (inst->program[i] & 0xf000) != 0xf000) ? '`' : ' '),
							select_program_step, MAKEPTR(i), 0, 0);
				}
			}
			
			if(mused.flags2 & DRAG_SELECT_PROGRAM)
			{
				if(mused.selection.drag_selection_program)
				{
					int mouse_x, mouse_y;
					
					Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
					UNUSED(buttons);
					
					gfx_convert_mouse_coordinates(domain, &mouse_x, &mouse_y);
					
					if ((mouse_x > row1.x) && (mouse_y > row1.y) && (mouse_x <= row1.x + row1.w) && (mouse_y < row1.y + row1.h - 1))
					{
						if(mused.selection.prev_start != i)
						{
							mused.selection.start = mused.selection.prev_start;
							mused.selection.end = i;
							
							if(mused.selection.start > mused.selection.end)
							{
								int temp = mused.selection.start;
								mused.selection.start = mused.selection.end;
								mused.selection.end = temp;
							}
						}
					}
					
					if(event->type == SDL_MOUSEBUTTONUP)
					{
						mused.selection.drag_selection_program = false;
						
						if(mused.selection.start != mused.selection.end)
						{
							mused.jump_in_program = false;
						}
					}
				}
				
				if(check_event(event, &row1, NULL, NULL, NULL, NULL))
				{
					if(!(mused.selection.drag_selection_program) && (mused.selection.start == mused.selection.end))
					{
						mused.selection.prev_start = mused.current_program_step;
						mused.selection.drag_selection_program = true;
					}
					
					if(!(mused.selection.drag_selection_program) && (mused.selection.start != mused.selection.end))
					{
						mused.selection.drag_selection_program = false;
						mused.jump_in_program = true;
						mused.selection.start = mused.selection.end = -1;
					}
				}
			}

			if (!is_valid_command(inst->program[mused.current_instrument_program][i]))
				console_write_args(mused.console, "???");
			else if ((inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_ARPEGGIO || (inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_ARPEGGIO_ABS)
			{
				if ((inst->program[mused.current_instrument_program][i] & 0xff) != 0xf0 && (inst->program[mused.current_instrument_program][i] & 0xff) != 0xf1)
					console_write_args(mused.console, "%s", notename(((inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_ARPEGGIO_ABS ? 0 : inst->base_note) + (inst->program[mused.current_instrument_program][i] & 0xff)));
				else
					console_write_args(mused.console, "EXT%x", inst->program[mused.current_instrument_program][i] & 0x0f);
			}
			
			else if ((inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_ARPEGGIO_DOWN)
			{
				console_write_args(mused.console, "%s", notename(((inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_ARPEGGIO_ABS ? 0 : inst->base_note) - (inst->program[mused.current_instrument_program][i] & 0xff)));
			}
			
			else if ((inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_SET_2ND_ARP_NOTE || (inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_SET_3RD_ARP_NOTE)
			{
				console_write_args(mused.console, "%s", notename(inst->base_note + (inst->program[mused.current_instrument_program][i] & 0xff)));
			}
			
			else if ((inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_SET_NOISE_CONSTANT_PITCH)
			{
				console_write_args(mused.console, "NOI %s", notename(inst->program[mused.current_instrument_program][i] & 0xff));
			}
			
			else if (inst->program[mused.current_instrument_program][i] != MUS_FX_NOP)
			{
				const InstructionDesc *d = get_instruction_desc(inst->program[mused.current_instrument_program][i]);
				
				if (d)
					console_write(mused.console, d->shortname ? d->shortname : d->name);
			}

			console_write_args(mused.console, "\n");

			if (row.y + row.h < area.y + area.h)
				slider_set_params(&mused.program_slider_param, 0, MUS_PROG_LEN - 1, start, i, &mused.program_position, 1, SLIDER_VERTICAL, mused.slider_bevel);

			prev_pos = pos;

			if (!(inst->program_unite_bits[mused.current_instrument_program][i / 8] & (1 << (i & 7))) || (inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_JUMP || (inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_LABEL || (inst->program[mused.current_instrument_program][i] & 0xff00) == MUS_FX_LOOP) ++pos;
		}

		if (mused.focus == EDITPROG && mused.selection.start != mused.selection.end
			&& !(mused.selection.start > mused.program_slider_param.visible_last || mused.selection.end <= mused.program_slider_param.visible_first) && mused.selection.start >= 0 && selection_begin >= 0 && selection_end >= 0)
		{
			if (selection_begin == -1) selection_begin = area.y - 8;
			if (selection_end == -1) selection_end = area.y + area.h + 8;

			if (selection_begin > selection_end) swap(selection_begin, selection_end);

			SDL_Rect selection = { area.x + 8 * 6 - 3, selection_begin - 1, 8 * 4 + 5 + 5, selection_end - selection_begin + 1 };
			adjust_rect(&selection, -3);
			bevel(domain, &selection, mused.slider_bevel, BEV_SELECTION);
		}

		gfx_domain_set_clip(domain, NULL);

		//if (mused.focus == EDITPROG)
			//check_mouse_wheel_event(event, dest, &mused.program_slider_param);
		
		if (event->type == SDL_MOUSEWHEEL && mused.focus == EDITPROG)
		{
			if (event->wheel.y > 0)
			{
				mused.program_position -= 2;
				*(mused.program_slider_param.position) -= 2;
			}
			
			else
			{
				mused.program_position += 2;
				*(mused.program_slider_param.position) += 2;
			}
			
			mused.program_position = my_max(0, my_min(MUS_PROG_LEN - area.h / 8, mused.program_position));
			*(mused.program_slider_param.position) = my_max(0, my_min(MUS_PROG_LEN - area.h / 8, *(mused.program_slider_param.position)));
		}
	}
}

void oscilloscope_view(GfxDomain *dest_surface, const SDL_Rect *dest1, const SDL_Event *event, void *param) //wasn't there
{
	SDL_Rect *dest = (SDL_Rect *)dest1;
	
	if(mused.show_four_op_menu)
	{
		dest->y -= 69; //nice
		dest->x -= 10;
	}
	
	if(!(mused.show_four_op_menu) && (mused.show_fx_lfo_settings))
	{
		dest->y -= 60;
	}

	if(!(mused.show_four_op_menu) && (mused.show_timer_lfo_settings))
	{
		dest->y -= 130;
	}
	
	if(mused.flags & SHOW_OSCILLOSCOPE_INST_EDITOR)
	{
		SDL_Rect area;
		copy_rect(&area, dest);
		bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
		adjust_rect(&area, 2);
		
		int *pointer = (int*)&mused.output_buffer_counter;
		
		update_oscillscope_view(dest_surface, &area, mused.output_buffer, OSC_SIZE, pointer, true, (bool)(mused.flags2 & SHOW_OSCILLOSCOPE_MIDLINES));
	}
	
	if(mused.import_mml_string)
	{
		import_mml_fm_instrument_string(&mused.song.instrument[mused.current_instrument]);
	}
}

static void inst_flags(const SDL_Event *e, const SDL_Rect *_area, int p, const char *label, Uint32 *flags, Uint32 mask)
{
	generic_flags(e, _area, EDITINSTRUMENT, p, label, flags, mask);
}


static void inst_text(const SDL_Event *e, const SDL_Rect *area, int p, const char *_label, const char *format, void *value, int width)
{
	//check_event(e, area, select_instrument_param, (void*)p, 0, 0);

	int d = generic_field(e, area, EDITINSTRUMENT, p, _label, format, value, width);
	
	if (d && (mused.mode == EDITINSTRUMENT || p == P_INSTRUMENT)) //p == P_INSTRUMENT because otherwise it does not work in classic editor (doesn't allow you to change current instrument)
	{
		if (p >= 0) mused.selected_param = p;
		if (p != P_INSTRUMENT) snapshot_cascade(S_T_INSTRUMENT, mused.current_instrument, p);
		if (d < 0) instrument_add_param(-1);
		else if (d > 0) instrument_add_param(1);
	}
}

static void four_op_flags(const SDL_Event *e, const SDL_Rect *_area, int p, const char *label, Uint32 *flags, Uint32 mask)
{
	generic_flags(e, _area, EDIT4OP, p, label, flags, mask);
}


static void four_op_text(const SDL_Event *e, const SDL_Rect *area, int p, const char *_label, const char *format, void *value, int width)
{
	int d = generic_field(e, area, EDIT4OP, p, _label, format, value, width);
	
	if (d && mused.mode == EDIT4OP)
	{
		if (p >= 0) mused.fourop_selected_param = p;
		if (p != P_INSTRUMENT) snapshot_cascade(S_T_INSTRUMENT, mused.current_instrument, p);
		if (d < 0) four_op_add_param(-1);
		else if (d > 0) four_op_add_param(1);
	}
}

void inst_field(const SDL_Event *e, const SDL_Rect *area, int p, int length, char *text) //instrument name/song name
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

	if (mused.edit_buffer == text && mused.focus == EDITBUFFER && mused.selected_param == p)
	{
		int i = my_max(0, mused.editpos - field.w / mused.console->font.w + 1), c = 0;
		
		for (; text[i] && c < my_min(length, field.w / mused.console->font.w); ++i, ++c)
		{
			const SDL_Rect *r = console_write_args(mused.console, "%c", mused.editpos == i ? '½' : text[i]);
			
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

	if (!got_pos && (c = check_event(e, &field, select_instrument_param, MAKEPTR(p), 0, 0)))
	{
		if (mused.focus == EDITBUFFER && mused.edit_buffer == text)
			mused.editpos = strlen(text);
		else
			set_edit_buffer(text, length);

		if (text == mused.song.title)
			snapshot(S_T_SONGINFO);
		else if (text == mused.song.instrument[mused.current_instrument].name)
			snapshot(S_T_INSTRUMENT);
		else if (text == mused.song.wavetable_names[mused.selected_wavetable])
			snapshot(S_T_WAVE_NAME);
	}

	if (!c && mused.focus == EDITBUFFER && e->type == SDL_MOUSEBUTTONDOWN) change_mode(mused.prev_mode);
}



void instrument_name_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	if(!(mused.show_four_op_menu) || ((mused.focus != EDIT4OP) && (mused.focus != EDITPROG4OP) && (mused.focus != EDITENVELOPE4OP)))
	{
		SDL_Rect farea, larea, tarea;
		copy_rect(&farea, dest);
		copy_rect(&larea, dest);
		copy_rect(&tarea, dest);

		farea.w = 2 * mused.console->font.w + 2 + 16;

		if (!param)
		{
			larea.w = 0;
		}
		
		else
		{
			larea.w = 32;
			label("INST", &larea);
		}

		tarea.w = dest->w - farea.w - larea.w;
		farea.x = larea.w + dest->x;
		tarea.x = farea.x + farea.w;

		inst_text(event, &farea, P_INSTRUMENT, "", "%02X", MAKEPTR(mused.current_instrument), 2);
		inst_field(event, &tarea, P_NAME, sizeof(mused.song.instrument[mused.current_instrument].name), mused.song.instrument[mused.current_instrument].name);

		if (is_selected_param(EDITINSTRUMENT, P_NAME) || (mused.selected_param == P_NAME && mused.mode == EDITINSTRUMENT && (mused.edit_buffer == mused.song.instrument[mused.current_instrument].name && mused.focus == EDITBUFFER)))
		{
			SDL_Rect r;
			copy_rect(&r, &tarea);
			adjust_rect(&r, -2);
			set_cursor(&r);
		}
	}
}

void instrument_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	if(!(mused.show_four_op_menu) || ((mused.focus != EDIT4OP) && (mused.focus != EDITPROG4OP) && (mused.focus != EDITENVELOPE4OP)))
	{
		MusInstrument *inst = &mused.song.instrument[mused.current_instrument];

		SDL_Rect r, frame;
		copy_rect(&frame, dest);
		bevelex(domain, &frame, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
		adjust_rect(&frame, 4);
		copy_rect(&r, &frame);
		r.w = r.w / 2 - 2;
		r.h = 10;
		r.y += r.h + 1;

		{
			SDL_Rect note;
			copy_rect(&note, &frame);

			note.w = frame.w / 2 + 2;
			note.h = 10;

			inst_text(event, &note, P_BASENOTE, "BASE", "%s", notename(inst->base_note), 3);
			note.x += note.w + 2;
			note.w = frame.w / 3;
			inst_text(event, &note, P_FINETUNE, "", "%+4d", MAKEPTR(inst->finetune), 4);
			note.x += note.w + 2;
			note.w = frame.w - note.x;

			inst_flags(event, &note, P_LOCKNOTE, "L", (Uint32*)&inst->flags, MUS_INST_LOCK_NOTE);
			inst_flags(event, &r, P_DRUM, "DRUM", (Uint32*)&inst->flags, MUS_INST_DRUM);
			update_rect(&frame, &r);
			inst_flags(event, &r, P_KEYSYNC, "KSYNC", &inst->cydflags, CYD_CHN_ENABLE_KEY_SYNC);
			update_rect(&frame, &r);
			inst_flags(event, &r, P_INVVIB, "VIB", (Uint32*)&inst->flags, MUS_INST_INVERT_VIBRATO_BIT);
			update_rect(&frame, &r);
			
			inst_flags(event, &r, P_INVTREM, "TREM", (Uint32*)&inst->flags, MUS_INST_INVERT_TREMOLO_BIT);
			update_rect(&frame, &r);
			
			inst_flags(event, &r, P_SETPW, "SET PW", (Uint32*)&inst->flags, MUS_INST_SET_PW);
			update_rect(&frame, &r);
			inst_flags(event, &r, P_SETCUTOFF, "SET CUT", (Uint32*)&inst->flags, MUS_INST_SET_CUTOFF);
			update_rect(&frame, &r);
			
			int tmp = r.w;
			r.w += 10;
			
			inst_text(event, &r, P_SLIDESPEED, "SLIDE", "%03X", MAKEPTR(inst->slide_speed), 3);
			update_rect(&frame, &r);
			
			r.w = tmp;
		}

		{
			int tmp = r.w;
			r.w = frame.w / 3 - 2 - 12;
			my_separator(&frame, &r);
			inst_flags(event, &r, P_PULSE, "PUL", &inst->cydflags, CYD_CHN_ENABLE_PULSE);
			update_rect(&frame, &r);
			r.w = frame.w / 2 - 2 - 25;
			inst_text(event, &r, P_PW, "", "%03X", MAKEPTR(inst->pw), 3);
			update_rect(&frame, &r);
			r.w = frame.w / 3 - 8;
			inst_flags(event, &r, P_SAW, "SAW", &inst->cydflags, CYD_CHN_ENABLE_SAW);
			update_rect(&frame, &r);
			r.w = frame.w / 3 - 8;
			inst_flags(event, &r, P_TRIANGLE, "TRI", &inst->cydflags, CYD_CHN_ENABLE_TRIANGLE);
			update_rect(&frame, &r);
			inst_flags(event, &r, P_NOISE, "NOI", &inst->cydflags, CYD_CHN_ENABLE_NOISE);
			update_rect(&frame, &r);
			r.w = frame.w / 3;
			inst_flags(event, &r, P_METAL, "METAL", &inst->cydflags, CYD_CHN_ENABLE_METAL);
			update_rect(&frame, &r);
			inst_flags(event, &r, P_LFSR, "POKEY", &inst->cydflags, CYD_CHN_ENABLE_LFSR);
			update_rect(&frame, &r);
			r.w = frame.w / 3 - 16;
			inst_text(event, &r, P_LFSRTYPE, "", "%X", MAKEPTR(inst->lfsr_type), 1);
			update_rect(&frame, &r);
			r.w = frame.w / 3 - 2;
			inst_flags(event, &r, P_1_4TH, "1/4TH", (Uint32*)&inst->flags, MUS_INST_QUARTER_FREQ);
			update_rect(&frame, &r);
			
			r.w = frame.w / 3 - 5;
			
			inst_flags(event, &r, P_SINE, "SINE", &inst->cydflags, CYD_CHN_ENABLE_SINE);
			update_rect(&frame, &r);
			
			r.w = frame.w * 2 / 3 - 1;
			
			inst_text(event, &r, P_SINE_PHASE_SHIFT, "PH. SHIFT", "%X", MAKEPTR(inst->sine_acc_shift), 1);
			update_rect(&frame, &r);
			
			r.w = frame.w / 2 + 26;
			
			inst_flags(event, &r, P_FIX_NOISE_PITCH, "LOCK NOI PIT", &inst->cydflags, CYD_CHN_ENABLE_FIXED_NOISE_PITCH);
			update_rect(&frame, &r);
			
			SDL_Rect npitch;
			copy_rect(&npitch, &frame);
			
			npitch.w = frame.w / 3 - 6;
			npitch.h = 10;
			npitch.x += frame.w / 2 + 28;
			npitch.y += 87 + 10;
			
			inst_text(event, &npitch, P_FIXED_NOISE_BASE_NOTE, "", "%s", notename(inst->noise_note), 3);
			//update_rect(&npitch, &r);
			
			r.w = frame.w;
			
			inst_flags(event, &r, P_1_BIT_NOISE, "ENABLE 1-BIT NOISE", &inst->cydflags, CYD_CHN_ENABLE_1_BIT_NOISE);
			update_rect(&frame, &r);
			
			r.w = tmp;
		}

		{
			my_separator(&frame, &r);
			int tmp = r.w;
			r.w = 42;

			inst_flags(event, &r, P_WAVE, "WAVE", &inst->cydflags, CYD_CHN_ENABLE_WAVE);
			r.x += 44;
			r.w = 32;
			inst_text(event, &r, P_WAVE_ENTRY, "", "%02X", MAKEPTR(inst->wavetable_entry), 2);
			update_rect(&frame, &r);
			r.w = 42;
			inst_flags(event, &r, P_WAVE_OVERRIDE_ENV, "OENV", &inst->cydflags, CYD_CHN_WAVE_OVERRIDE_ENV);
			r.x += r.w;
			r.w = 20;
			inst_flags(event, &r, P_WAVE_LOCK_NOTE, "L", (Uint32*)&inst->flags, MUS_INST_WAVE_LOCK_NOTE);
			update_rect(&frame, &r);

			r.w = tmp;
		}
		
		my_separator(&frame, &r); //wasn't there
		
		
		static const char *mixtypes[] = {"AND", "SUM", "bOR", "C64", "XOR"}; 

		r.w = frame.w;

		inst_text(event, &r, P_OSCMIXMODE, "OSC. MIX MODE", "%s", (char*)mixtypes[inst->mixmode], 3); //inst_text(event, &r, P_OSCMIXMODE, "OSC. MIX MODE", "%s", (char*)mixtypes[inst->flttype], 3);
		update_rect(&frame, &r); 
		
		r.w = frame.w / 2 - 2; //wasn't there end
		

		my_separator(&frame, &r);
		inst_text(event, &r, P_VOLUME, "VOL", "%02X", MAKEPTR(inst->volume), 2);
		update_rect(&frame, &r);
		inst_flags(event, &r, P_RELVOL, "RELATIVE", (Uint32*)&inst->flags, MUS_INST_RELATIVE_VOLUME);
		update_rect(&frame, &r);
		inst_text(event, &r, P_ATTACK, "ATK", "%02X", MAKEPTR(inst->adsr.a), 2);
		update_rect(&frame, &r);
		inst_text(event, &r, P_DECAY, "DEC", "%02X", MAKEPTR(inst->adsr.d), 2);
		update_rect(&frame, &r);
		inst_text(event, &r, P_SUSTAIN, "SUS", "%02X", MAKEPTR(inst->adsr.s), 2);
		update_rect(&frame, &r);
		inst_text(event, &r, P_RELEASE, "REL", "%02X", MAKEPTR(inst->adsr.r), 2);
		update_rect(&frame, &r);
		
		int temp = r.w;
		r.w = frame.w / 4 - 2;
		
		inst_flags(event, &r, P_EXP_VOL, "E.V", &inst->cydflags, CYD_CHN_ENABLE_EXPONENTIAL_VOLUME);
		update_rect(&frame, &r);
		inst_flags(event, &r, P_EXP_ATTACK, "ATK", &inst->cydflags, CYD_CHN_ENABLE_EXPONENTIAL_ATTACK);
		update_rect(&frame, &r);
		inst_flags(event, &r, P_EXP_DECAY, "DEC", &inst->cydflags, CYD_CHN_ENABLE_EXPONENTIAL_DECAY);
		update_rect(&frame, &r);
		inst_flags(event, &r, P_EXP_RELEASE, "REL", &inst->cydflags, CYD_CHN_ENABLE_EXPONENTIAL_RELEASE);
		update_rect(&frame, &r);
		
		//r.w = temp;
		
		r.w = 8 * 4;
		
		inst_flags(event, &r, P_VOL_KSL, "VKS", &inst->cydflags, CYD_CHN_ENABLE_VOLUME_KEY_SCALING);
		update_rect(&frame, &r);
		
		r.w = 8 * 2 + 19;
		
		inst_text(event, &r, P_VOL_KSL_LEVEL, "", "%02X", MAKEPTR(inst->vol_ksl_level), 2);
		update_rect(&frame, &r);
		
		r.w = 8 * 4;
		
		r.x += 1;
		
		inst_flags(event, &r, P_ENV_KSL, "EKS", &inst->cydflags, CYD_CHN_ENABLE_ENVELOPE_KEY_SCALING);
		update_rect(&frame, &r);
		
		r.w = 8 * 2 + 19;
		
		r.x -= 1;
		
		inst_text(event, &r, P_ENV_KSL_LEVEL, "", "%02X", MAKEPTR(inst->env_ksl_level), 2);
		update_rect(&frame, &r);
		
		r.w = temp;

		{
			my_separator(&frame, &r);
			int tmp = r.w;
			r.w = frame.w / 3 + 8;
			inst_flags(event, &r, P_BUZZ, "BUZZ", (Uint32*)&inst->flags, MUS_INST_YM_BUZZ);
			r.x += r.w + 2;
			r.w = frame.w - r.x + 4;
			inst_text(event, &r, P_BUZZ_SEMI, "DETUNE", "%+3d", MAKEPTR((inst->buzz_offset + 0x80) >> 8), 3);
			update_rect(&frame, &r);
			r.w = frame.w / 3 - 7; //r.w = frame.w / 3 - 8;
			inst_text(event, &r, P_BUZZ_SHAPE, "SH", "%c", MAKEPTR(inst->ym_env_shape + 0xf0), 1);
			r.x += r.w + 1;
			//r.w = frame.w - r.x + 4;
			inst_flags(event, &r, P_BUZZ_ENABLE_AY8930, "8930", &inst->cydflags, CYD_CHN_ENABLE_AY8930_BUZZ_MODE);
			update_rect(&frame, &r);
			
			r.w = frame.w / 3 + 11;
			
			inst_text(event, &r, P_BUZZ_FINE, "F", "%+4d", MAKEPTR((Sint8)(inst->buzz_offset & 0xff)), 4);
			update_rect(&frame, &r);
			r.w = tmp;
		}

		my_separator(&frame, &r);
		
		temp = r.w;
		r.w = temp - 34 - 1;
		
		inst_flags(event, &r, P_SYNC, "H.S", &inst->cydflags, CYD_CHN_ENABLE_SYNC);
		update_rect(&frame, &r);
		
		r.w = 34;
		r.x -= 3;
		
		inst_text(event, &r, P_SYNCSRC, "", "%02X", MAKEPTR(inst->sync_source), 2);
		update_rect(&frame, &r);
		
		r.w = temp - 34;
		
		inst_flags(event, &r, P_RINGMOD, "R.M", &inst->cydflags, CYD_CHN_ENABLE_RING_MODULATION);
		update_rect(&frame, &r);
		
		r.w = 34;
		r.x -= 3;
		
		r.y -= 10;
		r.x += temp + temp - 34 + 7;
		
		inst_text(event, &r, P_RINGMODSRC, "", "%02X", MAKEPTR(inst->ring_mod), 2);
		update_rect(&frame, &r);
		
		r.w = temp;

		static const char *flttype[] = { "LPF", "HPF", "BPF", "LHP", "HBP", "LBP", "ALL" }; //was `{ "LP", "HP", "BP" };`
		static const char *slope[] = { "12  dB/oct", "24  dB/oct", "48  dB/oct", "96  dB/oct", "192 dB/oct", "384 dB/oct" };

		my_separator(&frame, &r);
		inst_flags(event, &r, P_FILTER, "FILTER", &inst->cydflags, CYD_CHN_ENABLE_FILTER);
		update_rect(&frame, &r);
		inst_text(event, &r, P_FLTTYPE, "TYPE", "%s", (char*)flttype[inst->flttype], 3); //was `(char*)flttype[inst->flttype], 2);`
		update_rect(&frame, &r);
		inst_text(event, &r, P_CUTOFF, "CUT", "%03X", MAKEPTR(inst->cutoff), 3);
		update_rect(&frame, &r);
		inst_text(event, &r, P_RESONANCE, "RES", "%02X", MAKEPTR(inst->resonance), 2);
		update_rect(&frame, &r);
		
		r.w = frame.w;
		
		inst_text(event, &r, P_SLOPE, "SLOPE", "%s", (char*)slope[inst->slope], 10);
		update_rect(&frame, &r);
		
		my_separator(&frame, &r);
		
		//inst_text(event, &r, P_NUM_OF_MACROS, "PROGRAM NUMBER", "%02X", MAKEPTR(inst->num_macros), 2);
		inst_text(event, &r, P_NUM_OF_MACROS, "PROGRAM NUMBER", "%02X", MAKEPTR(mused.current_instrument_program), 2);
		update_rect(&frame, &r);
		
		inst_field(event, &r, P_NAME, MUS_MACRO_NAME_LEN, mused.song.instrument[mused.current_instrument].program_names[mused.current_instrument_program]);
		
		r.y += 10;
		
		inst_text(event, &r, P_PROGPERIOD, "PROGRAM PERIOD", "%02X", MAKEPTR(inst->prog_period[mused.current_instrument_program]), 2);
		update_rect(&frame, &r);
	}
}


void draw_op(SDL_Rect* op, int opx, int opy, int op_num, MusInstrument* inst)
{
	gfx_line(domain, opx + 1, opy + 2, opx - 2, opy + 2, ((0x60 * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x30 : 0)) << 16) + ((0x6E * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x37 : 0)) << 8) + 0x6A * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x35 : 0)); //feedback
	gfx_line(domain, opx - 2, opy + 2, opx - 2, opy - 2, ((0x60 * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x30 : 0)) << 16) + ((0x6E * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x37 : 0)) << 8) + 0x6A * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x35 : 0));
	gfx_line(domain, opx - 2, opy - 2, opx + 23, opy - 2, ((0x60 * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x30 : 0)) << 16) + ((0x6E * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x37 : 0)) << 8) + 0x6A * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x35 : 0));
	gfx_line(domain, opx + 23, opy - 2, opx + 23, opy + 2, ((0x60 * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x30 : 0)) << 16) + ((0x6E * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x37 : 0)) << 8) + 0x6A * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x35 : 0));
	gfx_line(domain, opx + 23, opy + 2, opx + 20, opy + 2, ((0x60 * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x30 : 0)) << 16) + ((0x6E * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x37 : 0)) << 8) + 0x6A * inst->ops[op_num].feedback / 15 + (inst->ops[op_num].feedback > 0 ? 0x35 : 0));
	
	op->x = opx;
	op->y = opy;
}

void open_4op_prog(void *unused1, void *unused2, void *unused3)
{
	mused.show_4op_point_envelope_editor = false;
	
	mused.fourop_vol_env_point = -1;
	mused.fourop_vol_env_scale = 1;
	mused.fourop_vol_env_horiz_scroll = 0;
	
	mused.fourop_point_env_editor_scroll = 0;
	
	mused.focus = EDITPROG4OP;
}

void open_4op_env(void *unused1, void *unused2, void *unused3)
{
	mused.show_4op_point_envelope_editor = true;
	
	mused.fourop_current_volume_envelope_point = 0;
	mused.fourop_env_selected_param = 0;
	
	mused.fourop_vol_env_point = -1;
	mused.fourop_vol_env_scale = 1;
	mused.fourop_vol_env_horiz_scroll = 0;
	
	mused.fourop_point_env_editor_scroll = 0;
	
	mused.focus = EDITENVELOPE4OP;
}

void four_op_menu_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param) //4-op FM menu, filebox-like
{
	if(mused.show_four_op_menu)
	{
		MusInstrument *inst = &mused.song.instrument[mused.current_instrument];

		SDL_Rect frame;
		copy_rect(&frame, dest);
		
		#define BEV_SIZE 16
		#define BORDER 4
		#define SIZE_MINUS_BORDER (BEV_SIZE - BORDER)
		
		SDL_Rect dest1 = { BORDER + dest->x, BORDER + dest->y, dest->w - 2 * BORDER, dest->h - 2 * BORDER };
		
		gfx_rect(dest_surface, &dest1, 0xBCBCBC);
		
		/* Sides */
		for (int y = BORDER; y < dest->h - BORDER; y += BEV_SIZE / 2)
		{	
			{
				SDL_Rect src = { BEV_MENU * BEV_SIZE, BORDER, BORDER, my_min(BEV_SIZE / 2, dest->h - BORDER - y) };
				SDL_Rect dest1 = { dest->x, y + dest->y, BORDER, my_min(BEV_SIZE / 2, dest->h - BORDER - y) };
				my_BlitSurface(mused.slider_bevel, &src, dest_surface, &dest1);
			}
			
			{
				SDL_Rect src = { SIZE_MINUS_BORDER + BEV_MENU * BEV_SIZE, BORDER, BORDER, my_min(BEV_SIZE / 2, dest->h - BORDER - y) };
				SDL_Rect dest1 = { dest->x + dest->w - BORDER, y + dest->y, BORDER, my_min(BEV_SIZE / 2, dest->h - BORDER - y) };
				my_BlitSurface(mused.slider_bevel, &src, dest_surface, &dest1);
			}
		}
		
		for (int x = BORDER; x < dest->w - BORDER; x += BEV_SIZE / 2)
		{	
			{
				SDL_Rect src = { BORDER + BEV_MENU * BEV_SIZE, 0, my_min(BEV_SIZE / 2, dest->w - BORDER - x), BORDER };
				SDL_Rect dest1 = { dest->x + x, dest->y, my_min(BEV_SIZE / 2, dest->w - BORDER - x), BORDER };
				my_BlitSurface(mused.slider_bevel, &src, dest_surface, &dest1);
			}
			
			{
				SDL_Rect src = { BORDER + BEV_MENU * BEV_SIZE, SIZE_MINUS_BORDER, my_min(BEV_SIZE / 2, dest->w - BORDER - x), BORDER };
				SDL_Rect dest1 = { x + dest->x, dest->y + dest->h - BORDER, my_min(BEV_SIZE / 2, dest->w - BORDER - x), BORDER };
				my_BlitSurface(mused.slider_bevel, &src, dest_surface, &dest1);
			}
		}
		
		/* Corners */
		{
			SDL_Rect src = { BEV_MENU * BEV_SIZE, 0, BORDER, BORDER };
			SDL_Rect dest1 = { dest->x, dest->y, BORDER, BORDER };
			my_BlitSurface(mused.slider_bevel, &src, dest_surface, &dest1);
		}
		
		{
			SDL_Rect src = { SIZE_MINUS_BORDER + BEV_MENU * BEV_SIZE, 0, BORDER, BORDER };
			SDL_Rect dest1 = { dest->x + dest->w - BORDER, dest->y, BORDER, BORDER };
			my_BlitSurface(mused.slider_bevel, &src, dest_surface, &dest1);
		}
		
		{
			SDL_Rect src = { SIZE_MINUS_BORDER + BEV_MENU * BEV_SIZE, SIZE_MINUS_BORDER, BORDER, BORDER };
			SDL_Rect dest1 = { dest->x + dest->w - BORDER, dest->y + dest->h - BORDER, BORDER, BORDER };
			my_BlitSurface(mused.slider_bevel, &src, dest_surface, &dest1);
		}
		
		{
			SDL_Rect src = { BEV_MENU * BEV_SIZE, SIZE_MINUS_BORDER, BORDER, BORDER };
			SDL_Rect dest1 = { dest->x, dest->y + dest->h - BORDER, BORDER, BORDER };
			my_BlitSurface(mused.slider_bevel, &src, dest_surface, &dest1);
		}
		
		frame.h -= 16;
		frame.y += 16;
		
		const char* title = "4-OP FM settings";
		SDL_Rect titlearea, button;
		copy_rect(&titlearea, dest);
		
		adjust_rect(&titlearea, 10);
		
		copy_rect(&button, dest);
		adjust_rect(&button, titlearea.h);
		button.w = 12;
		
		button.h = 12;
		
		button.x = dest->w + dest->x - 20;
		button.y -= dest->h - 28;
		
		font_write(&mused.largefont, dest_surface, &titlearea, title);
		
		if (button_event(dest_surface, event, &button, mused.slider_bevel, BEV_BUTTON, BEV_BUTTON_ACTIVE, DECAL_CLOSE, NULL, MAKEPTR(1), 0, 0) & 1) //button to exit filebox
		{
			snapshot(S_T_MODE);
			
			mused.show_four_op_menu = false;
			mused.show_4op_point_envelope_editor = false;
			
			change_mode(EDITINSTRUMENT);
			
			snapshot(S_T_MODE);
		}
		
		adjust_rect(&frame, 8);
		
		int frame_x = frame.x;
		int frame_y = frame.y;
		int frame_w = frame.w;
		//int frame_h = frame.h;
		
		gfx_rect(dest_surface, &frame, 0x0);
		
		SDL_Rect top_view, view, view2, op_alg_view, program_view, slider;
		UNUSED(slider);
		UNUSED(program_view);
			
		{
			SDL_Rect r;
			
			top_view.x = frame.x;
			top_view.y = frame.y;
			top_view.h = TOP_VIEW_H;
			top_view.w = frame.w;
			
			adjust_rect(&top_view, 2);
			bevelex(domain, &top_view, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
			
			adjust_rect(&top_view, 4);
			
			copy_rect(&r, &top_view);
			r.w = 160;
			r.h = 10;
			r.y -= 2;
			
			{
				four_op_flags(event, &r, FOUROP_3CH_EXP_MODE, "ENABLE 3CH EXP MODE", &inst->fm_flags, CYD_FM_ENABLE_3CH_EXP_MODE);
				update_rect(&top_view, &r);
				
				r.w = 100;
				
				four_op_text(event, &r, FOUROP_ALG, "ALGORITHM", "%02d", MAKEPTR(inst->alg), 2);
				update_rect(&top_view, &r);
				
				r.w = 60;
				
				four_op_text(event, &r, FOUROP_MASTER_VOL, "VOL", "%02X", MAKEPTR(inst->fm_4op_vol), 2);
				update_rect(&top_view, &r);
				
				r.w = 125;
				
				four_op_flags(event, &r, FOUROP_USE_MAIN_INST_PROG, "USE INST. PROG.", &inst->fm_flags, CYD_FM_FOUROP_USE_MAIN_INST_PROG);
				update_rect(&top_view, &r);
				
				r.w = 125;
				
				four_op_flags(event, &r, FOUROP_BYPASS_MAIN_INST_FILTER, "BYP.INST.FILT.", &inst->fm_flags, CYD_FM_FOUROP_BYPASS_MAIN_INST_FILTER);
				update_rect(&top_view, &r);
			}
		}
		
		//===========================================================
		
		{
			SDL_Rect r;
			
			view.x = frame.x;
			view.y = frame.y + TOP_VIEW_H - 4;
			view.h = frame.h - TOP_VIEW_H + 5;
			view.w = VIEW_WIDTH;
			
			adjust_rect(&view, 2);
			
			copy_rect(&frame, &view);
			
			bevelex(domain, &view, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
			
			adjust_rect(&view, 4);
			
			copy_rect(&r, &view);
			r.w = r.w / 2 - 2;
			r.h = 10;
			r.y += r.h + 1;

			{
				if(inst->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE)
				{
					SDL_Rect note;
					copy_rect(&note, &view);

					note.w = note.w / 2 - 6;
					note.h = 10;

					four_op_text(event, &note, FOUROP_BASENOTE, "BASE", "%s", notename(inst->ops[mused.selected_operator - 1].base_note), 3);
					note.x += note.w + 2;
					note.w = view.w / 3;
					four_op_text(event, &note, FOUROP_FINETUNE, "", "%+4d", MAKEPTR(inst->ops[mused.selected_operator - 1].finetune), 4);
					note.x += note.w + 2;
					note.w = view.w - note.x;

					four_op_flags(event, &note, FOUROP_LOCKNOTE, "L", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_LOCK_NOTE);
				}
				
				else //mult and detune
				{
					SDL_Rect note;
					copy_rect(&note, &view);

					note.w = note.w / 3 - 2 - 15;
					note.h = 10;
					
					four_op_text(event, &note, FOUROP_HARMONIC_CARRIER, "M", "%01X", MAKEPTR(inst->ops[mused.selected_operator - 1].harmonic >> 4), 1);
					
					note.w -= 10;
					note.x += note.w + 12;
					
					four_op_text(event, &note, FOUROP_HARMONIC_MODULATOR, "", "%01X", MAKEPTR(inst->ops[mused.selected_operator - 1].harmonic & 15), 1);
					note.x += note.w + 2;
					note.w += 12 + 6;
					//note.w += 6;
					four_op_text(event, &note, FOUROP_DETUNE, "D", "%+1d", MAKEPTR(inst->ops[mused.selected_operator - 1].detune), 2);
					note.x += note.w + 2;
					note.w += 6 + 1;
					four_op_text(event, &note, FOUROP_COARSE_DETUNE, "DT2", "%01d", MAKEPTR(inst->ops[mused.selected_operator - 1].coarse_detune), 1);
				}
				
				int temp = r.w;
				
				r.w -= 12;
				r.y -= 1;
				
				four_op_text(event, &r, FOUROP_FEEDBACK, "FBACK", "%01X", MAKEPTR(inst->ops[mused.selected_operator - 1].feedback), 1);
				update_rect(&view, &r);
				
				r.w -= 7;
				
				four_op_flags(event, &r, FOUROP_ENABLE_SSG_EG, "SSG-EG", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_SSG_EG);
				r.x += 63;
				
				r.w = 25;
				
				four_op_text(event, &r, FOUROP_SSG_EG_TYPE, "", "%01X", MAKEPTR(inst->ops[mused.selected_operator - 1].ssg_eg_type), 1);
				update_rect(&view, &r);
				
				r.w = temp;
				
				four_op_flags(event, &r, FOUROP_DRUM, "DRUM", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_DRUM);
				update_rect(&view, &r);
				four_op_flags(event, &r, FOUROP_KEYSYNC, "KSYNC", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_KEY_SYNC);
				update_rect(&view, &r);
				four_op_flags(event, &r, FOUROP_INVVIB, "VIB", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_INVERT_VIBRATO_BIT);
				update_rect(&view, &r);
				
				four_op_flags(event, &r, FOUROP_INVTREM, "TREM", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_INVERT_TREMOLO_BIT);
				update_rect(&view, &r);
				
				four_op_flags(event, &r, FOUROP_SETPW, "SET PW", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_SET_PW);
				update_rect(&view, &r);
				four_op_flags(event, &r, FOUROP_SETCUTOFF, "SET CUT", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_SET_CUTOFF);
				update_rect(&view, &r);
				
				int tmp = r.w;
				r.w += 10;

				four_op_text(event, &r, FOUROP_SLIDESPEED, "SLIDE", "%03X", MAKEPTR(inst->ops[mused.selected_operator - 1].slide_speed), 3);
				update_rect(&view, &r);
				
				r.w = tmp;
			}
			
			{
				int tmp = r.w;
				r.w = view.w / 3 - 2 - 12;
				my_separator(&view, &r);
				four_op_flags(event, &r, FOUROP_PULSE, "PUL", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_PULSE);
				update_rect(&view, &r);
				r.w = view.w / 2 - 2 - 26;
				four_op_text(event, &r, FOUROP_PW, "", "%03X", MAKEPTR(inst->ops[mused.selected_operator - 1].pw), 3);
				update_rect(&view, &r);
				r.w = view.w / 3 - 8;
				four_op_flags(event, &r, FOUROP_SAW, "SAW", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_SAW);
				update_rect(&view, &r);
				r.w = view.w / 3 - 8;
				four_op_flags(event, &r, FOUROP_TRIANGLE, "TRI", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_TRIANGLE);
				update_rect(&view, &r);
				four_op_flags(event, &r, FOUROP_NOISE, "NOI", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_NOISE);
				update_rect(&view, &r);
				r.w = view.w / 3;
				four_op_flags(event, &r, FOUROP_METAL, "METAL", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_METAL);
				update_rect(&view, &r);
				
				r.w = view.w / 3 - 5;
			
				four_op_flags(event, &r, FOUROP_SINE, "SINE", &inst->ops[mused.selected_operator - 1].cydflags, CYD_CHN_ENABLE_SINE);
				update_rect(&view, &r);
				
				r.w = view.w * 2 / 3 - 2;
				
				four_op_text(event, &r, FOUROP_SINE_PHASE_SHIFT, "PH. SHIFT", "%X", MAKEPTR(inst->ops[mused.selected_operator - 1].sine_acc_shift), 1);
				update_rect(&view, &r);

				r.w = view.w / 2 + 26;
				
				four_op_flags(event, &r, FOUROP_FIX_NOISE_PITCH, "LOCK NOI PIT", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_FIXED_NOISE_PITCH);
				update_rect(&view, &r);
				
				SDL_Rect npitch;
				copy_rect(&npitch, &view);
				
				npitch.w = view.w / 3 - 6;
				npitch.h = 10;
				npitch.x += view.w / 2 + 28;
				npitch.y += 87 + 10;
				
				four_op_text(event, &npitch, FOUROP_FIXED_NOISE_BASE_NOTE, "", "%s", notename(inst->ops[mused.selected_operator - 1].noise_note), 3);
				
				r.w = view.w;
				
				four_op_flags(event, &r, FOUROP_1_BIT_NOISE, "ENABLE 1-BIT NOISE", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_1_BIT_NOISE);
				update_rect(&view, &r);
				
				r.w = tmp;
			}

			{
				my_separator(&view, &r);
				int tmp = r.w;
				r.w = 42;

				four_op_flags(event, &r, FOUROP_WAVE, "WAVE", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_WAVE);
				r.x += 44;
				r.w = 32;
				four_op_text(event, &r, FOUROP_WAVE_ENTRY, "", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].wavetable_entry), 2);
				update_rect(&view, &r);
				r.w = 42;
				four_op_flags(event, &r, FOUROP_WAVE_OVERRIDE_ENV, "OENV", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_WAVE_OVERRIDE_ENV);
				r.x += r.w;
				r.w = 20;
				four_op_flags(event, &r, FOUROP_WAVE_LOCK_NOTE, "L", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_WAVE_LOCK_NOTE);
				update_rect(&view, &r);

				r.w = tmp;
			}
			
			my_separator(&view, &r); //wasn't there
			
			static const char *mixtypes_4op[] = {"AND", "SUM", "bOR", "C64", "XOR"}; 

			r.w = view.w;

			four_op_text(event, &r, FOUROP_OSCMIXMODE, "OSC. MIX MODE", "%s", (char*)mixtypes_4op[inst->ops[mused.selected_operator - 1].mixmode], 3); //four_op_text(event, &r, FOUROP_OSCMIXMODE, "OSC. MIX MODE", "%s", (char*)mixtypes[inst->ops[mused.selected_operator - 1].flttype], 3);
			update_rect(&view, &r); 
			
			r.w = view.w / 2 - 2; //wasn't there end

			my_separator(&view, &r);
			four_op_text(event, &r, FOUROP_VOLUME, "VOL", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].volume), 2);
			update_rect(&view, &r);
			four_op_flags(event, &r, FOUROP_RELVOL, "RELATIVE", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_RELATIVE_VOLUME);
			update_rect(&view, &r);
			
			int temp = r.w;
			
			r.w = view.w / 3 - 2;
			
			four_op_text(event, &r, FOUROP_ATTACK, "AT", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].adsr.a), 2);
			update_rect(&view, &r);
			four_op_text(event, &r, FOUROP_DECAY, "DE", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].adsr.d), 2);
			update_rect(&view, &r);
			
			r.x -= 1;
			
			four_op_text(event, &r, FOUROP_SUSTAIN, "SL", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].adsr.s), 2);
			update_rect(&view, &r);
			
			r.w = temp;
			
			four_op_text(event, &r, FOUROP_SUSTAIN_RATE, "SUS.R", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].adsr.sr), 2);
			update_rect(&view, &r);
			
			four_op_text(event, &r, FOUROP_RELEASE, "REL", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].adsr.r), 2);
			update_rect(&view, &r);
			
			
			r.w = view.w / 4 - 2;
			
			four_op_flags(event, &r, FOUROP_EXP_VOL, "E.V", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_EXPONENTIAL_VOLUME);
			update_rect(&view, &r);
			four_op_flags(event, &r, FOUROP_EXP_ATTACK, "ATK", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_EXPONENTIAL_ATTACK);
			update_rect(&view, &r);
			four_op_flags(event, &r, FOUROP_EXP_DECAY, "DEC", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_EXPONENTIAL_DECAY);
			update_rect(&view, &r);
			four_op_flags(event, &r, FOUROP_EXP_RELEASE, "REL", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_EXPONENTIAL_RELEASE);
			update_rect(&view, &r);
			
			r.w = temp;
			
			r.w = 8 * 2 + 19 + 4;
			
			four_op_flags(event, &r, FOUROP_VOL_KSL, "VKS", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_VOLUME_KEY_SCALING);
			update_rect(&view, &r);
			
			r.w = 8 * 4;
			
			four_op_text(event, &r, FOUROP_VOL_KSL_LEVEL, "", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].vol_ksl_level), 2);
			update_rect(&view, &r);
			
			r.w = 8 * 2 + 19;
			r.x += 1;
			
			four_op_flags(event, &r, FOUROP_ENV_KSL, "EKS", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_ENVELOPE_KEY_SCALING);
			update_rect(&view, &r);
			
			r.w = 8 * 4;
			r.x += 3;
			
			four_op_text(event, &r, FOUROP_ENV_KSL_LEVEL, "", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].env_ksl_level), 2);
			update_rect(&view, &r);
			
			r.w = temp;
			
			my_separator(&view, &r);
			
			temp = r.w;
			r.w = temp - 34 - 1;
			
			four_op_flags(event, &r, FOUROP_SYNC, "H.S", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_SYNC);
			update_rect(&view, &r);
			
			r.w = 34;
			r.x -= 3;
			
			four_op_text(event, &r, FOUROP_SYNCSRC, "", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].sync_source), 2);
			update_rect(&view, &r);
			
			r.w = temp - 34;
			r.x += 1;
			
			four_op_flags(event, &r, FOUROP_RINGMOD, "R.M", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_RING_MODULATION);
			update_rect(&view, &r);
			
			r.w = 34;
			r.x -= 3;
			
			r.y -= 10;
			r.x += temp + temp - 34 + 7;
			
			four_op_text(event, &r, FOUROP_RINGMODSRC, "", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].ring_mod), 2);
			update_rect(&view, &r);
			
			r.w = temp;

			static const char *flttype_4op[] = { "LPF", "HPF", "BPF", "LHP", "HBP", "LBP", "ALL" }; //was `{ "LP", "HP", "BP" };`
			static const char *slope_4op[] = { "12  dB/oct", "24  dB/oct", "48  dB/oct", "96  dB/oct", "192 dB/oct", "384 dB/oct" };

			my_separator(&view, &r);
			four_op_flags(event, &r, FOUROP_FILTER, "FILTER", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_FILTER);
			update_rect(&view, &r);
			four_op_text(event, &r, FOUROP_FLTTYPE, "TYPE", "%s", (char*)flttype_4op[inst->ops[mused.selected_operator - 1].flttype], 3); //was `(char*)flttype[inst->ops[mused.selected_operator - 1].flttype], 2);`
			update_rect(&view, &r);
			four_op_text(event, &r, FOUROP_CUTOFF, "CUT", "%03X", MAKEPTR(inst->ops[mused.selected_operator - 1].cutoff), 3);
			update_rect(&view, &r);
			four_op_text(event, &r, FOUROP_RESONANCE, "RES", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].resonance), 2);
			update_rect(&view, &r);
			
			r.w = view.w;
			
			four_op_text(event, &r, FOUROP_SLOPE, "SLOPE", "%s", (char*)slope_4op[inst->ops[mused.selected_operator - 1].slope], 10);
			update_rect(&view, &r);
			
			my_separator(&view, &r);
		
			//four_op_text(event, &r, FOUROP_NUM_OF_MACROS, "PROGRAM NUMBER", "%1X", MAKEPTR(inst->ops[mused.selected_operator - 1].num_macros), 1);
			four_op_text(event, &r, FOUROP_NUM_OF_MACROS, "PROGRAM NUMBER", "%1X", MAKEPTR(mused.current_fourop_program[mused.selected_operator - 1]), 1);
			
			update_rect(&view, &r);
			
			inst_field(event, &r, P_NAME, MUS_MACRO_NAME_LEN, mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_names[mused.current_fourop_program[mused.selected_operator - 1]]);
			
			r.y += 10;
			
			four_op_text(event, &r, FOUROP_PROGPERIOD, "PROGRAM PERIOD", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].prog_period[mused.current_fourop_program[mused.selected_operator - 1]]), 2);
			update_rect(&view, &r);
		}
		
		//========================================================================
		
		{
			op_alg_view.x = frame_x + VIEW_WIDTH - 4;
			op_alg_view.y = frame_y + TOP_VIEW_H - 4;
			op_alg_view.h = ALG_VIEW_H;
			op_alg_view.w = (frame_w - VIEW_WIDTH) / 2 + 3;
			
			int op_alg_view_x = op_alg_view.x;
			int op_alg_view_y = op_alg_view.y;
			int op_alg_view_h = op_alg_view.h;
			int op_alg_view_w = op_alg_view.w;
			
			adjust_rect(&op_alg_view, 2);
			bevelex(domain, &op_alg_view, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
			
			adjust_rect(&op_alg_view, 4);
			
			SDL_Rect but1, but2, but3, but4;
			
			if(inst->alg != 0)
			{
				but1.w = but2.w = but3.w = but4.w = 22;
				but1.h = but2.h = but3.h = but4.h = 13;
			}
			
			else
			{
				but1.w = but2.w = but3.w = but4.w = 0;
				but1.h = but2.h = but3.h = but4.h = 0;
			}
			
			switch(inst->alg)
			{
				case 1:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 34, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 60, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					
					// F  F  F  F
					// 4--3--2--1--(OUT)
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 54, op_alg_view_y + op_alg_view_h / 2 - 6, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 - 25, op_alg_view_y + op_alg_view_h / 2 - 6, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 + 4, op_alg_view_y + op_alg_view_h / 2 - 6, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 33, op_alg_view_y + op_alg_view_h / 2 - 6, 0, inst);
				break;
				
				case 2:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 22, op_alg_view_y + op_alg_view_h / 2 - 15, op_alg_view_x + op_alg_view_w / 2 - 17, op_alg_view_y + op_alg_view_h / 2 - 15, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 22, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 - 17, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 17, op_alg_view_y + op_alg_view_h / 2 - 15, op_alg_view_x + op_alg_view_w / 2 - 17, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 17, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 - 10, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 9, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 19, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 38, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					
					// F
					// 4--| F  F
					//    |--2--1--(OUT)
					// F  |
					// 3--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 42, op_alg_view_y + op_alg_view_h / 2 - 21, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 - 42, op_alg_view_y + op_alg_view_h / 2 + 9, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 - 11, op_alg_view_y + op_alg_view_h / 2 - 6, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 18, op_alg_view_y + op_alg_view_h / 2 - 6, 0, inst);
				break;
				
				case 3:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 24, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 - 12, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 7, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 + 12, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 12, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 + 12, op_alg_view_y + op_alg_view_h / 2 - 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 12, op_alg_view_y + op_alg_view_h / 2 - 15, op_alg_view_x + op_alg_view_w / 2 + 7, op_alg_view_y + op_alg_view_h / 2 - 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 12, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 19, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 38, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					
					//    F
					//    4--|  F
					//       |--1--(OUT)
					// F  F  |
					// 3--2--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 13, op_alg_view_y + op_alg_view_h / 2 - 21, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 - 42, op_alg_view_y + op_alg_view_h / 2 + 9, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 - 13, op_alg_view_y + op_alg_view_h / 2 + 9, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 18, op_alg_view_y + op_alg_view_h / 2 - 6, 0, inst);
				break;
				
				case 4:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 22, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 - 10, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 9, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 + 21, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 40, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2 - 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2 - 15, op_alg_view_x + op_alg_view_w / 2 + 40, op_alg_view_y + op_alg_view_h / 2 - 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 51, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					
					//       F
					//       1--|
					//          |
					// F  F  F  |--(OUT)
					// 4--3--2--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 42, op_alg_view_y + op_alg_view_h / 2 + 9, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 - 11, op_alg_view_y + op_alg_view_h / 2 + 9, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 + 20, op_alg_view_y + op_alg_view_h / 2 + 9, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 20, op_alg_view_y + op_alg_view_h / 2 - 21, 0, inst);
				break;
				
				case 5:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 20, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 - 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 - 15, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2 - 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2 - 15, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 45, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 51, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					
					//          F
					// F  F  |--1--|
					// 4--3--|     |--(OUT) 
					//       |  F  |
					//       |--2--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 40, op_alg_view_y + op_alg_view_h / 2 - 6, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 - 10, op_alg_view_y + op_alg_view_h / 2 - 6, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 + 20, op_alg_view_y + op_alg_view_h / 2 + 9, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 20, op_alg_view_y + op_alg_view_h / 2 - 21, 0, inst);
				break;
				
				case 6:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 6, op_alg_view_y + op_alg_view_h / 2 - 15, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 - 15, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 6, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 - 15, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 36, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					
					// F  F
					// 4--3--|
					//       |--(OUT)
					// F  F  |
					// 2--1--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 26, op_alg_view_y + op_alg_view_h / 2 - 21, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 - 21, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 - 26, op_alg_view_y + op_alg_view_h / 2 + 9, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 + 9, 0, inst);
				break;
				
				case 7:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 10, op_alg_view_y + op_alg_view_h / 2 - 21, op_alg_view_x + op_alg_view_w / 2 - 5, op_alg_view_y + op_alg_view_h / 2 - 21, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 10, op_alg_view_y + op_alg_view_h / 2 + 9, op_alg_view_x + op_alg_view_w / 2 - 5, op_alg_view_y + op_alg_view_h / 2 + 9, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 5, op_alg_view_y + op_alg_view_h / 2 - 21, op_alg_view_x + op_alg_view_w / 2 - 5, op_alg_view_y + op_alg_view_h / 2 + 9, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 5, op_alg_view_y + op_alg_view_h / 2 - 6, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 - 6, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 25, op_alg_view_y + op_alg_view_h / 2 + 24, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 + 24, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 - 6, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 + 24, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 + 9, op_alg_view_x + op_alg_view_w / 2 + 36, op_alg_view_y + op_alg_view_h / 2 + 9, 0x90A7A0);
					
					// F  
					// 4--|  F
					//    |--2--|
					// F  |     |--(OUT)
					// 3--|  F  |
					//       1--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 30, op_alg_view_y + op_alg_view_h / 2 - 27, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 - 30, op_alg_view_y + op_alg_view_h / 2 + 3, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 - 12, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 + 18, 0, inst);
				break;
				
				case 8:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 8, op_alg_view_y + op_alg_view_h / 2 - 26, op_alg_view_x + op_alg_view_w / 2 - 3, op_alg_view_y + op_alg_view_h / 2 - 26, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 8, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 34, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 8, op_alg_view_y + op_alg_view_h / 2 + 26, op_alg_view_x + op_alg_view_w / 2 - 3, op_alg_view_y + op_alg_view_h / 2 + 26, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 3, op_alg_view_y + op_alg_view_h / 2 - 26, op_alg_view_x + op_alg_view_w / 2 - 3, op_alg_view_y + op_alg_view_h / 2 + 26, 0x90A7A0);
					
					// F  
					// 4--|
					//    |
					// F  |  F
					// 3--|--1--(OUT)
					//    |
					// F  |
					// 2--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 28, op_alg_view_y + op_alg_view_h / 2 - 32, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 - 28, op_alg_view_y + op_alg_view_h / 2 - 6, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 - 28, op_alg_view_y + op_alg_view_h / 2 + 20, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 7, op_alg_view_y + op_alg_view_h / 2 - 6, 0, inst);
				break;
				
				case 9:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 6, op_alg_view_y + op_alg_view_h / 2 - 26, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 - 26, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 25, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 36, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 25, op_alg_view_y + op_alg_view_h / 2 + 26, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 + 26, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 - 26, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 + 26, 0x90A7A0);
					
					// F  F
					// 4--3--|
					//       |
					//    F  |
					//    2--|--(OUT)
					//       |
					//    F  |
					//    1--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 26, op_alg_view_y + op_alg_view_h / 2 - 32, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 - 32, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 - 6, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 + 20, 0, inst);
				break;
				
				case 10:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 6, op_alg_view_y + op_alg_view_h / 2 - 13, op_alg_view_x + op_alg_view_w / 2, op_alg_view_y + op_alg_view_h / 2 - 13, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2, op_alg_view_y + op_alg_view_h / 2 - 26, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 - 26, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 36, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 25, op_alg_view_y + op_alg_view_h / 2 + 26, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 + 26, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2, op_alg_view_y + op_alg_view_h / 2 - 26, op_alg_view_x + op_alg_view_w / 2, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 - 26, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 + 26, 0x90A7A0);
					
					//       F
					// F  |--3--|
					// 4--|     |
					//    |  F  |
					//    |--2--|--(OUT)
					//          |
					//       F  |
					//       1--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 26, op_alg_view_y + op_alg_view_h / 2 - 19, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 - 32, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 - 6, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 + 20, 0, inst);
				break;
				
				case 11:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 6, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2, op_alg_view_y + op_alg_view_h / 2 - 26, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 - 26, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 36, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2, op_alg_view_y + op_alg_view_h / 2 + 26, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 + 26, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2, op_alg_view_y + op_alg_view_h / 2 - 26, op_alg_view_x + op_alg_view_w / 2, op_alg_view_y + op_alg_view_h / 2 + 26, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 - 26, op_alg_view_x + op_alg_view_w / 2 + 30, op_alg_view_y + op_alg_view_h / 2 + 26, 0x90A7A0);
					
					//       F
					//    |--3--|
					//    |     |
					// F  |  F  |
					// 4--|--2--|--(OUT)
					//    |     |
					//    |  F  |
					//    |--1--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 26, op_alg_view_y + op_alg_view_h / 2 - 6, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 - 32, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 - 6, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 5, op_alg_view_y + op_alg_view_h / 2 + 20, 0, inst);
				break;
				
				case 12:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 21, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 - 15, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 48, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 15, op_alg_view_y + op_alg_view_h / 2 - 15, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 - 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 15, op_alg_view_y + op_alg_view_h / 2 + 15, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 - 15, op_alg_view_y + op_alg_view_h / 2 - 15, op_alg_view_x + op_alg_view_w / 2 - 15, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 - 15, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 + 15, 0x90A7A0);
					
					//       F
					//    |--3--|
					// F  |     |
					// 4--|  F  |--1--(OUT)
					//    |--2--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 41, op_alg_view_y + op_alg_view_h / 2 - 6, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 - 10, op_alg_view_y + op_alg_view_h / 2 - 21, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 - 10, op_alg_view_y + op_alg_view_h / 2 + 9, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 + 21, op_alg_view_y + op_alg_view_h / 2 - 6, 0, inst);
				break;
				
				case 13:
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 10, op_alg_view_y + op_alg_view_h / 2 - 27, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 - 27, 0x90A7A0); //lines between ops
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 10, op_alg_view_y + op_alg_view_h / 2 - 9, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 - 9, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 10, op_alg_view_y + op_alg_view_h / 2 + 9, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 + 9, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 10, op_alg_view_y + op_alg_view_h / 2 + 27, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 + 27, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 - 27, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2 + 27, 0x90A7A0);
					gfx_line(domain, op_alg_view_x + op_alg_view_w / 2 + 16, op_alg_view_y + op_alg_view_h / 2, op_alg_view_x + op_alg_view_w / 2 + 22, op_alg_view_y + op_alg_view_h / 2, 0x90A7A0);
					
					// F
					// 4--|
					//    |
					// F  |
					// 3--|
					//    |
					// F  |--(OUT)
					// 2--|
					//    |
					// F  |
					// 1--|
					
					draw_op(&but4, op_alg_view_x + op_alg_view_w / 2 - 10, op_alg_view_y + op_alg_view_h / 2 - 33, 3, inst);
					draw_op(&but3, op_alg_view_x + op_alg_view_w / 2 - 10, op_alg_view_y + op_alg_view_h / 2 - 15, 2, inst);
					draw_op(&but2, op_alg_view_x + op_alg_view_w / 2 - 10, op_alg_view_y + op_alg_view_h / 2 + 3, 1, inst);
					draw_op(&but1, op_alg_view_x + op_alg_view_w / 2 - 10, op_alg_view_y + op_alg_view_h / 2 + 21, 0, inst);
				break;
				
				default: break;
			} // `\ff` red `\fe` black
			
			bool ops_playing[CYD_FM_NUM_OPS] = { 0 };
			
			bool should_continue = true;
			
			for(int i = 0; (i < mused.song.num_channels) && should_continue; ++i)
			{
				if(mused.mus.channel[i].instrument != NULL)
				{
					if(mused.mus.channel[i].instrument == inst && (mused.mus.cyd->channel[i].fm.flags & CYD_FM_ENABLE_4OP))
					{
						for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
						{
							if((mused.mus.cyd->channel[i].fm.ops[j].adsr.envelope > 0 || mused.mus.cyd->channel[i].fm.ops[j].flags & CYD_FM_OP_ENABLE_GATE) && mused.mus.cyd->channel[i].fm.ops[j].adsr.volume > 0)
							{
								ops_playing[j] = true;
								should_continue = false;
							}
						}
					}
				}
			}
			
			if (button_text_event(dest_surface, event, &but1, mused.slider_bevel, &mused.largefont,
				(mused.selected_operator != 1) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, 
				(mused.selected_operator != 1) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, ops_playing[0] ? " 1\xff" : " 1\xfe", NULL, MAKEPTR(1), 0, 0) & 1)
			{
				snapshot(S_T_INSTRUMENT);
				mused.selected_operator = 1;
			}
			
			if (button_text_event(dest_surface, event, &but2, mused.slider_bevel, &mused.largefont,
				(mused.selected_operator != 2) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, 
				(mused.selected_operator != 2) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, ops_playing[1] ? " 2\xff" : " 2\xfe", NULL, MAKEPTR(1), 0, 0) & 1)
			{
				snapshot(S_T_INSTRUMENT);
				mused.selected_operator = 2;
			}
			
			if (button_text_event(dest_surface, event, &but3, mused.slider_bevel, &mused.largefont,
				(mused.selected_operator != 3) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, 
				(mused.selected_operator != 3) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, ops_playing[2] ? " 3\xff" : " 3\xfe", NULL, MAKEPTR(1), 0, 0) & 1)
			{
				snapshot(S_T_INSTRUMENT);
				mused.selected_operator = 3;
			}
			
			if (button_text_event(dest_surface, event, &but4, mused.slider_bevel, &mused.largefont,
				(mused.selected_operator != 4) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, 
				(mused.selected_operator != 4) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, ops_playing[3] ? " 4\xff" : " 4\xfe", NULL, MAKEPTR(1), 0, 0) & 1)
			{
				snapshot(S_T_INSTRUMENT);
				mused.selected_operator = 4;
			}
			
			SDL_Rect OPL3, OPN, DX, OPM;
			copy_rect(&OPL3, dest);
			copy_rect(&OPN, dest);
			copy_rect(&DX, dest);
			copy_rect(&OPM, dest);
			
			OPL3.x = op_alg_view_x + 4;
			OPL3.y = op_alg_view_y + 5;
			OPN.x = op_alg_view_x + 4;
			OPN.y = op_alg_view_y + 11;
			DX.x = op_alg_view_x + 4;
			DX.y = op_alg_view_y + 17;
			OPM.x = op_alg_view_x + 4;
			OPM.y = op_alg_view_y + 23;
			
			OPL3.w = OPN.w = DX.w = OPM.w = 20;
			OPL3.h = OPN.h = DX.h = OPM.h = 6;
			
			if(inst->alg == 1 || inst->alg == 4 || inst->alg == 6 || inst->alg == 9) //OPL3
			{
				console_set_color(mused.console, colors[COLOR_WAVETABLE_SAMPLE]);
				font_write_args(&mused.tinyfont, dest_surface, &OPL3, "OPL3");
			}
			
			if(inst->alg == 1 || inst->alg == 2 || inst->alg == 3 || inst->alg == 6 || 
			inst->alg == 9 || inst->alg == 11 || inst->alg == 13) //OPN, OPM, DX-9
			{
				console_set_color(mused.console, colors[COLOR_WAVETABLE_SAMPLE]);
				font_write_args(&mused.tinyfont, dest_surface, &OPN, "OPN");
				
				console_set_color(mused.console, colors[COLOR_WAVETABLE_SAMPLE]);
				font_write_args(&mused.tinyfont, dest_surface, &OPM, "OPM");
				
				console_set_color(mused.console, colors[COLOR_WAVETABLE_SAMPLE]);
				font_write_args(&mused.tinyfont, dest_surface, &DX, "DX-9");
			}
		}
		
		{
			SDL_Rect r;
			
			view2.x = frame_x + VIEW_WIDTH + (frame_w - VIEW_WIDTH) / 2 - 4;
			view2.y = frame_y + TOP_VIEW_H - 4;
			view2.h = ALG_VIEW_H + 1;
			view2.w = (frame_w - VIEW_WIDTH) / 2 + 4;
			
			adjust_rect(&view2, 2);
			bevelex(domain, &view2, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
			
			adjust_rect(&view2, 4);
			
			copy_rect(&r, &view2);
			r.w = r.w / 2 - 2;
			r.h = 10;
			
			four_op_text(event, &r, FOUROP_VIBSPEED,   "VIB.S", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].vibrato_speed), 2);
			update_rect(&view2, &r);
			four_op_text(event, &r, FOUROP_VIBDEPTH,   "VIB.D", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].vibrato_depth), 2);
			update_rect(&view2, &r);
			four_op_text(event, &r, FOUROP_VIBSHAPE,   "VIB.SH", "%c", inst->ops[mused.selected_operator - 1].vibrato_shape < 5 ? MAKEPTR(inst->ops[mused.selected_operator - 1].vibrato_shape + 0xf4) : MAKEPTR(inst->ops[mused.selected_operator - 1].vibrato_shape + 0xed), 1);
			update_rect(&view2, &r);
			four_op_text(event, &r, FOUROP_VIBDELAY,   "V.DEL", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].vibrato_delay), 2);
			update_rect(&view2, &r);
			four_op_text(event, &r, FOUROP_PWMSPEED,   "PWM.S", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].pwm_speed), 2);
			update_rect(&view2, &r);
			four_op_text(event, &r, FOUROP_PWMDEPTH,   "PWM.D", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].pwm_depth), 2);
			update_rect(&view2, &r);
			four_op_text(event, &r, FOUROP_PWMSHAPE,   "PWM.SH", "%c", inst->ops[mused.selected_operator - 1].pwm_shape < 5 ? MAKEPTR(inst->ops[mused.selected_operator - 1].pwm_shape + 0xf4) : MAKEPTR(inst->ops[mused.selected_operator - 1].pwm_shape + 0xed), 1);
			update_rect(&view2, &r);
			four_op_text(event, &r, FOUROP_PWMDELAY,   "PWM.DEL", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].pwm_delay), 2); //wasn't there
			update_rect(&view2, &r);
			four_op_text(event, &r, FOUROP_TREMSPEED,   "TR.S", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].tremolo_speed), 2); //wasn't there
			update_rect(&view2, &r);
			four_op_text(event, &r, FOUROP_TREMDEPTH,   "TR.D", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].tremolo_depth), 2);
			update_rect(&view2, &r);
			four_op_text(event, &r, FOUROP_TREMSHAPE,   "TR.SH", "%c", inst->ops[mused.selected_operator - 1].tremolo_shape < 5 ? MAKEPTR(inst->ops[mused.selected_operator - 1].tremolo_shape + 0xf4) : MAKEPTR(inst->ops[mused.selected_operator - 1].tremolo_shape + 0xed), 1);
			update_rect(&view2, &r);
			four_op_text(event, &r, FOUROP_TREMDELAY,   "TR.DEL", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].tremolo_delay), 2);
			update_rect(&view2, &r);
			//four_op_text(event, &r, FOUROP_PROGPERIOD, "P.PRD", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].prog_period[mused.current_fourop_program[mused.selected_operator - 1]]), 2);
			//update_rect(&view2, &r);
			four_op_flags(event, &r, FOUROP_NORESTART, "NO RESTART", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_NO_PROG_RESTART);
			update_rect(&view2, &r);
			four_op_flags(event, &r, FOUROP_SAVE_LFO_SETTINGS, "SAVE LFO SET.", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_SAVE_LFO_SETTINGS);
			update_rect(&view2, &r);
			
			four_op_text(event, &r, FOUROP_TRIG_DELAY, "TRIG.DEL.", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].trigger_delay), 2);
			update_rect(&view2, &r);

			four_op_text(event, &r, FOUROP_ENV_OFFSET, "E.START", "%02X", MAKEPTR(inst->ops[mused.selected_operator - 1].env_offset), 2);
			
			r.w = 80;
			
			r.x -= view2.w / 2 + 2;
			r.y += 10;
			
			four_op_flags(event, &r, FOUROP_ENABLE_CSM_TIMER, "CSM TIMER", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_CSM_TIMER);
			update_rect(&view2, &r);
			
			SDL_Rect note;
			copy_rect(&note, &r);

			note.w = 40;
			note.h = 10;

			four_op_text(event, &note, FOUROP_CSM_TIMER_NOTE, "", "%s", notename(inst->ops[mused.selected_operator - 1].CSM_timer_note), 3);
			note.x += note.w + 2;
			note.w += 8;
			four_op_text(event, &note, FOUROP_CSM_TIMER_FINETUNE, "", "%+4d", MAKEPTR(inst->ops[mused.selected_operator - 1].CSM_timer_finetune), 4);
			
			//r.w = 122;
			//r.x -= 84;
			//r.y += 10;
			update_rect(&view2, &r);

			r.x += 8;
			r.w += 40;
			
			four_op_flags(event, &r, FOUROP_LINK_CSM_TIMER_NOTE, "LINK TO OP NOTE", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_LINK_CSM_TIMER_NOTE);
			//update_rect(&view2, &r);

			r.y += 10;
			r.x = view2.x;

			r.w -= 18 + 22;
			
			four_op_flags(event, &r, FOUROP_ENABLE_PHASE_RESET_TIMER, "PH. RESET", &inst->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_PHASE_RESET_TIMER);
			update_rect(&view2, &r);
			
			copy_rect(&note, &r);

			note.w = 40;
			note.h = 10;

			four_op_text(event, &note, FOUROP_PHASE_RESET_TIMER_NOTE, "", "%s", notename(inst->ops[mused.selected_operator - 1].phase_reset_timer_note), 3);
			note.x += note.w + 2;
			note.w += 8;
			four_op_text(event, &note, FOUROP_PHASE_RESET_TIMER_FINETUNE, "", "%+4d", MAKEPTR(inst->ops[mused.selected_operator - 1].phase_reset_timer_finetune), 4);
			
			//r.w = 122;
			r.x += 92;
			//r.y += 10;
			r.w += 18 + 22;
			
			four_op_flags(event, &r, FOUROP_LINK_PHASE_RESET_TIMER_NOTE, "LINK TO OP NOTE", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_LINK_PHASE_RESET_TIMER_NOTE);

			r.w = view2.w / 2 - 2;
			r.h = 19;
			r.y += 10;
			r.x = view2.x;
			
			button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, !(mused.show_4op_point_envelope_editor) ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "PROGRAM", open_4op_prog, NULL, NULL, NULL);
			update_rect(&view2, &r);
			
			button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, mused.show_4op_point_envelope_editor ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "ENVELOPE", open_4op_env, NULL, NULL, NULL);
			update_rect(&view2, &r);
		}
	}
}


void four_op_program_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	//debug("four_op_program_view");
	
	if(mused.show_4op_point_envelope_editor) return;
	
	if(mused.show_four_op_menu)
	{
		//debug("four_op_program_view");
		
		SDL_Rect area, clip;
		copy_rect(&area, dest);
		console_set_clip(mused.console, &area);
		console_clear(mused.console);
		bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
		adjust_rect(&area, 2);
		copy_rect(&clip, &area);
		adjust_rect(&area, 1);
		area.w = 4000;
		console_set_clip(mused.console, &area);

		MusInstrument *inst = &mused.song.instrument[mused.current_instrument];
		
		if(inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]] == NULL)
		{
			while(inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]] == NULL)
			{
				mused.current_fourop_program[mused.selected_operator - 1]--;
			}
		}
		
		if(inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]] == NULL) return;
		if(inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]] == NULL) return;
		
		int start = mused.fourop_program_position[mused.selected_operator - 1];

		int pos = 0, prev_pos = -1;
		int selection_begin = -1, selection_end = -1;

		for (int i = 0; i < start; ++i)
		{
			prev_pos = pos;
			if (!(inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] & (1 << (i & 7))) || (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_JUMP || (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_LABEL || (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_LOOP) ++pos; //old command if (!(inst->program[i] & 0x8000) || (inst->program[i] & 0xf000) == 0xf000) ++pos;
		}

		gfx_domain_set_clip(domain, &clip);

		for (int i = start, s = 0, y = 0; i < MUS_PROG_LEN && y < area.h; ++i, ++s, y += mused.console->font.h)
		{
			SDL_Rect row = { area.x - 1, area.y + y - 1, area.w + 2, mused.console->font.h + 1};
			SDL_Rect row1 = { row.x + 8 * 6, row.y, 8 * 5, row.h};

			if (mused.current_program_step == i && mused.focus == EDITPROG4OP)
			{
				bevel(domain, &row, mused.slider_bevel, BEV_SELECTED_PATTERN_ROW);
				console_set_color(mused.console, colors[COLOR_PROGRAM_SELECTED]);
			}
			
			else
			{
				console_set_color(mused.console, pos & 1 ? colors[COLOR_PROGRAM_ODD] : colors[COLOR_PROGRAM_EVEN]);
			}

			if (i == mused.selection.start)
			{
				selection_begin = row.y;
				
				//debug("sel beg");
			}

			if (i == mused.selection.end - 1)
			{
				selection_end = row.y + row.h + 1;
				
				//debug("sel end");
			}

			char box[6], cur = ' ';
			bool pointing_at_command = false;

			for (int c = 0; c < CYD_MAX_CHANNELS; ++c)
			{
				if (mused.channel[c].instrument == inst && ((mused.cyd.channel[c].fm.ops[mused.selected_operator - 1].adsr.envelope > 0 || 
				(mused.cyd.channel[c].fm.ops[mused.selected_operator - 1].flags & CYD_FM_OP_ENABLE_GATE) /* so arrow does not blink when CSM timer is used */) && 
				(mused.channel[c].ops[mused.selected_operator - 1].program_flags & (1 << (mused.current_fourop_program[mused.selected_operator - 1]))) && 
				mused.channel[c].ops[mused.selected_operator - 1].program_tick[mused.current_fourop_program[mused.selected_operator - 1]] == i) && !(mused.channel[c].instrument->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG))
				//if (mused.channel[c].instrument == inst && ((mused.cyd.channel[c].fm.ops[mused.selected_operator - 1].flags & CYD_FM_OP_ENABLE_GATE) && (mused.channel[c].ops[mused.selected_operator - 1].flags & MUS_FM_OP_PROGRAM_RUNNING) && mused.channel[c].ops[mused.selected_operator - 1].program_tick == i) && !(inst->fm_flags & CYD_FM_FOUROP_USE_MAIN_INST_PROG))
				{
					cur = '½'; //where arrow pointing at current instrument (operator) program step is drawn
					pointing_at_command = true;
				}
			}

			if (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] == MUS_FX_NOP)
			{
				strcpy(box, "....");
			}
			
			else
			{
				sprintf(box, "%04X", inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i]); //old command sprintf(box, "%04X", ((inst->program[i] & 0xf000) != 0xf000) ? (inst->program[i] & 0x7fff) : inst->program[i]);
			}
			
			Uint32 temp_color = mused.console->current_color;
			Uint32 highlight_color;
			
			if((inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step] & 0xff00) == MUS_FX_JUMP && 
			inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step] != MUS_FX_NOP && (mused.flags2 & HIGHLIGHT_COMMANDS) && 
			(inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step] & 0xff) == i && mused.focus == EDITPROG4OP)
			{
				highlight_color = 0x00ee00;
				console_set_color(mused.console, highlight_color);
			}

			if (pos == prev_pos)
			{
				if(mused.flags2 & DRAG_SELECT_4OP)
				{
					if(mused.jump_in_program_4op)
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%02X%c   ", i, cur), select_program_step, MAKEPTR(i), 0, 0);
					}
					
					else
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%02X%c   ", i, cur), NULL, 0, 0, 0);
					}
				}
				
				else
				{
					check_event(event, console_write_args(mused.console, "%02X%c   ", i, cur), select_program_step, MAKEPTR(i), 0, 0);
				}
				
				bool highlight_united = false;
				
				if(mused.flags2 & HIGHLIGHT_COMMANDS)
				{
					console_set_color(mused.console, temp_color);
					
					if((inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][my_max(i - 1, 0) / 8] & (1 << (my_max(i - 1, 0) & 7))))
					{
						for(int q = i - 1; ((inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][my_max(q, 0) / 8] & (1 << (my_max(q, 0) & 7)))) && (q >= 0); --q)
						{
							for (int c = 0; c < CYD_MAX_CHANNELS; ++c)
							{
								if (mused.channel[c].instrument == inst && ((mused.cyd.channel[c].fm.ops[mused.selected_operator - 1].adsr.envelope > 0) && (mused.channel[c].ops[mused.selected_operator - 1].program_flags & (1 << mused.current_fourop_program[mused.selected_operator - 1])) && mused.channel[c].ops[mused.selected_operator - 1].program_tick[mused.current_fourop_program[mused.selected_operator - 1]] == q))
								{
									highlight_united = true;
									break;
								}
							}
						}
					}
				}
				
				write_command(event, box, i, mused.current_program_step, pointing_at_command || highlight_united);
				
				if(mused.flags2 & DRAG_SELECT_4OP)
				{
					if(mused.jump_in_program_4op)
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%c ", (!(inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] & (1 << (i & 7)))) ? '\xd' : '|'), //old command check_event(event, console_write_args(mused.console, "%c ", (!(inst->program[i] & 0x8000) || (inst->program[i] & 0xf000) == 0xf000) ? '�' : '|'),
							select_program_step, MAKEPTR(i), 0, 0);
					}
					
					else
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%c ", (!(inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] & (1 << (i & 7)))) ? '\xd' : '|'), //old command check_event(event, console_write_args(mused.console, "%c ", (!(inst->program[i] & 0x8000) || (inst->program[i] & 0xf000) == 0xf000) ? '�' : '|'),
							NULL, 0, 0, 0);
					}
				}
				
				else
				{
					check_event(event, console_write_args(mused.console, "%c ", (!(inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] & (1 << (i & 7)))) ? '\xd' : '|'), //old command check_event(event, console_write_args(mused.console, "%c ", (!(inst->program[i] & 0x8000) || (inst->program[i] & 0xf000) == 0xf000) ? '�' : '|'),
							select_program_step, MAKEPTR(i), 0, 0);
				}
			}
			
			else
			{
				if(mused.flags2 & DRAG_SELECT_4OP)
				{
					if(mused.jump_in_program_4op)
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%02X%c%02X ", i, cur, pos), select_program_step, MAKEPTR(i), 0, 0);
					}
					
					else
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%02X%c%02X ", i, cur, pos), NULL, 0, 0, 0);
					}
				}
				
				else
				{
					check_event(event, console_write_args(mused.console, "%02X%c%02X ", i, cur, pos), select_program_step, MAKEPTR(i), 0, 0);
				}
				
				bool highlight_united = false;
				
				if(mused.flags2 & HIGHLIGHT_COMMANDS)
				{
					console_set_color(mused.console, temp_color);
					
					if((inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][(i) / 8] & (1 << ((i) & 7))) || (inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][my_max(i - 1, 0) / 8] & (1 << (my_max(i - 1, 0) & 7))))
					{
						for(int q = i; (inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][q / 8] & (1 << (q & 7))) && (q >= 0); --q)
						{
							for (int c = 0; c < CYD_MAX_CHANNELS; ++c)
							{
								if (mused.channel[c].instrument == inst && ((mused.cyd.channel[c].fm.ops[mused.selected_operator - 1].adsr.envelope > 0) && (mused.channel[c].ops[mused.selected_operator - 1].program_flags & (1 << mused.current_fourop_program[mused.selected_operator - 1])) && mused.channel[c].ops[mused.selected_operator - 1].program_tick[mused.current_fourop_program[mused.selected_operator - 1]] == q))
								{
									highlight_united = true;
									break;
								}
							}
						}
					}
				}
				
				write_command(event, box, i, mused.current_program_step, pointing_at_command || highlight_united);
				
				if(mused.flags2 & DRAG_SELECT_4OP)
				{
					if(mused.jump_in_program_4op)
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%c ", ((inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] & (1 << (i & 7)))) ? '`' : ' '), //old command check_event(event, console_write_args(mused.console, "%c ", ((inst->program[i] & 0x8000) && (inst->program[i] & 0xf000) != 0xf000) ? '`' : ' '),
							select_program_step, MAKEPTR(i), 0, 0);
					}
					
					else
					{
						check_event_mousebuttonup(event, console_write_args(mused.console, "%c ", ((inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] & (1 << (i & 7)))) ? '`' : ' '), //old command check_event(event, console_write_args(mused.console, "%c ", ((inst->program[i] & 0x8000) && (inst->program[i] & 0xf000) != 0xf000) ? '`' : ' '),
							NULL, 0, 0, 0);
					}
				}
				
				else
				{
					check_event(event, console_write_args(mused.console, "%c ", ((inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] & (1 << (i & 7)))) ? '`' : ' '), //old command check_event(event, console_write_args(mused.console, "%c ", ((inst->program[i] & 0x8000) && (inst->program[i] & 0xf000) != 0xf000) ? '`' : ' '),
							select_program_step, MAKEPTR(i), 0, 0);
				}
			}
			
			if(mused.flags2 & DRAG_SELECT_4OP)
			{
				if(mused.selection.drag_selection_program_4op)
				{
					int mouse_x, mouse_y;
					
					Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
					UNUSED(buttons);
					
					gfx_convert_mouse_coordinates(domain, &mouse_x, &mouse_y);
					
					if ((mouse_x > row1.x) && (mouse_y > row1.y) && (mouse_x <= row1.x + row1.w) && (mouse_y < row1.y + row1.h - 1))
					{
						if(mused.selection.prev_start != i)
						{
							mused.selection.start = mused.selection.prev_start;
							mused.selection.end = i;
							
							if(mused.selection.start > mused.selection.end)
							{
								int temp = mused.selection.start;
								mused.selection.start = mused.selection.end;
								mused.selection.end = temp;
							}
						}
					}
					
					if(event->type == SDL_MOUSEBUTTONUP)
					{
						mused.selection.drag_selection_program_4op = false;
						
						if(mused.selection.start != mused.selection.end)
						{
							mused.jump_in_program_4op = false;
						}
					}
				}
				
				if(check_event(event, &row1, NULL, NULL, NULL, NULL))
				{
					if(!(mused.selection.drag_selection_program_4op) && (mused.selection.start == mused.selection.end))
					{
						mused.selection.prev_start = mused.current_program_step;
						mused.selection.drag_selection_program_4op = true;
					}
					
					if(!(mused.selection.drag_selection_program_4op) && (mused.selection.start != mused.selection.end))
					{
						mused.selection.drag_selection_program_4op = false;
						mused.jump_in_program_4op = true;
						mused.selection.start = mused.selection.end = -1;
					}
				}
			}

			if (!is_valid_command(inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i]))
			{
				console_write_args(mused.console, "???");
			}
			
			else if ((inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_ARPEGGIO || (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_ARPEGGIO_ABS)
			{
				if ((inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff) != 0xf0 && (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff) != 0xf1)
					console_write_args(mused.console, "%s", notename(((inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_ARPEGGIO_ABS ? 0 : ((inst->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? inst->ops[mused.selected_operator - 1].base_note : inst->base_note)) + (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff)));
				else
					console_write_args(mused.console, "EXT%x", inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0x0f);
			}
			
			else if ((inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_ARPEGGIO_DOWN)
			{
				console_write_args(mused.console, "%s", notename(((inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_ARPEGGIO_ABS ? 0 : ((inst->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? inst->ops[mused.selected_operator - 1].base_note : inst->base_note)) - (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff)));
			}
			
			else if ((inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_SET_2ND_ARP_NOTE || (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_SET_3RD_ARP_NOTE)
			{
				console_write_args(mused.console, "%s", notename(((inst->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? inst->ops[mused.selected_operator - 1].base_note : inst->base_note) + (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff)));
			}
			
			else if ((inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_SET_NOISE_CONSTANT_PITCH)
			{
				console_write_args(mused.console, "NOI %s", notename(inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff));
			}
			
			else if (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] != MUS_FX_NOP)
			{
				const InstructionDesc *d = get_instruction_desc(inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i]);
				
				if (d)
				{
					console_write(mused.console, d->shortname ? d->shortname : d->name);
				}
			}

			console_write_args(mused.console, "\n");

			if (row.y + row.h < area.y + area.h)
			{
				//slider_set_params(&mused.four_op_slider_param, 0, MUS_PROG_LEN - 1, start, i, &mused.program_position, 1, SLIDER_VERTICAL, mused.slider_bevel);
				slider_set_params(&mused.four_op_slider_param, 0, MUS_PROG_LEN - 1, start, i, &mused.fourop_program_position[mused.selected_operator - 1], 1, SLIDER_VERTICAL, mused.slider_bevel);
			}

			prev_pos = pos;

			if (!(inst->ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] & (1 << (i & 7))) || (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_JUMP || (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_LABEL || (inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] & 0xff00) == MUS_FX_LOOP) ++pos;
		}

		if (mused.focus == EDITPROG4OP && mused.selection.start != mused.selection.end
			&& !(mused.selection.start > mused.four_op_slider_param.visible_last || mused.selection.end <= mused.four_op_slider_param.visible_first) && mused.selection.start >= 0 && selection_begin >= 0 && selection_end >= 0)
		{
			if (selection_begin > selection_end) swap(selection_begin, selection_end);

			SDL_Rect selection = { area.x + 8 * 6 - 3, selection_begin - 1, 8 * 4 + 5 + 5, selection_end - selection_begin + 1 };
			adjust_rect(&selection, -3);
			bevel(domain, &selection, mused.slider_bevel, BEV_SELECTION);
		}

		gfx_domain_set_clip(domain, NULL);
		
		if (mused.focus == EDITPROG4OP)
		{
			//check_mouse_wheel_event(event, dest, &mused.four_op_slider_param);
			
			if (event->type == SDL_MOUSEWHEEL)
			{
				if (event->wheel.y > 0)
				{
					mused.fourop_program_position[mused.selected_operator - 1] -= 2;
					*(mused.four_op_slider_param.position) -= 2;
				}
				
				else
				{
					mused.fourop_program_position[mused.selected_operator - 1] += 2;
					*(mused.four_op_slider_param.position) += 2;
				}
				
				mused.fourop_program_position[mused.selected_operator - 1] = my_max(0, my_min(MUS_PROG_LEN - area.h / 8, mused.fourop_program_position[mused.selected_operator - 1]));
				*(mused.four_op_slider_param.position) = my_max(0, my_min(MUS_PROG_LEN - area.h / 8, *(mused.four_op_slider_param.position)));
			}
		}
	}
}

void open_fx_lfo(void *unused1, void *unused2, void *unused3)
{
	mused.show_fm_settings = false;
	mused.show_fx_lfo_settings = true;
	mused.show_timer_lfo_settings = false;
	
	if(mused.selected_param >= P_FM_ENABLE && mused.selected_param <= P_FM_ENABLE_4OP)
	{
		mused.selected_param = P_FX;
	}
}

void open_fm_set(void *unused1, void *unused2, void *unused3)
{
	mused.show_fm_settings = true;
	mused.show_fx_lfo_settings = false;
	mused.show_timer_lfo_settings = false;
	
	if(mused.selected_param >= P_FX && mused.selected_param <= P_SAVE_LFO_SETTINGS)
	{
		mused.selected_param = P_FM_ENABLE;
	}
}

void open_timer(void *unused1, void *unused2, void *unused3)
{
	mused.show_fm_settings = false;
	mused.show_fx_lfo_settings = false;
	mused.show_timer_lfo_settings = true;
	
	if(mused.selected_param >= P_FM_ENABLE_4OP && mused.selected_param <= P_LINK_PHASE_RESET_TIMER_NOTE)
	{
		mused.selected_param = P_ENABLE_PHASE_RESET_TIMER;
	}
}

void instrument_view2(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	if(!(mused.show_four_op_menu) || ((mused.focus != EDIT4OP) && (mused.focus != EDITPROG4OP) && (mused.focus != EDITENVELOPE4OP)))
	{
		MusInstrument *inst = &mused.song.instrument[mused.current_instrument];

		SDL_Rect r, frame;
		copy_rect(&frame, dest);
		
		if(mused.show_fx_lfo_settings)
		{
			frame.h -= 60;
		}

		if(mused.show_timer_lfo_settings)
		{
			frame.h -= 130;
		}
		
		bevelex(domain,&frame, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
		adjust_rect(&frame, 4);
		copy_rect(&r, &frame);
		
		int init_width = r.w;
		
		r.w = r.w / 3 - 2;
		r.h = 10;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, mused.show_fx_lfo_settings ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "FX AND LFO", open_fx_lfo, NULL, NULL, NULL);
		update_rect(&frame, &r);
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, mused.show_fm_settings ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "FM", open_fm_set, NULL, NULL, NULL);
		update_rect(&frame, &r);
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, mused.show_timer_lfo_settings ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "TIMER", open_timer, NULL, NULL, NULL);
		update_rect(&frame, &r);
		
		r.w = init_width / 2 - 2;

		int temp666;
		int temp2;
		int tmp;
		int temp;
		
		if(mused.show_fx_lfo_settings)
		{
			inst_flags(event, &r, P_FX, "FX", &inst->cydflags, CYD_CHN_ENABLE_FX);
			update_rect(&frame, &r);
			inst_text(event, &r, P_FXBUS,   "FXBUS", "%02X", MAKEPTR(inst->fx_bus), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_VIBSPEED,   "VIB.S", "%02X", MAKEPTR(inst->vibrato_speed), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_VIBDEPTH,   "VIB.D", "%02X", MAKEPTR(inst->vibrato_depth), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_VIBSHAPE,   "VIB.SH", "%c", inst->vibrato_shape < 5 ? MAKEPTR(inst->vibrato_shape + 0xf4) : MAKEPTR(inst->vibrato_shape + 0xed), 1);
			update_rect(&frame, &r);
			inst_text(event, &r, P_VIBDELAY,   "V.DEL", "%02X", MAKEPTR(inst->vibrato_delay), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_PWMSPEED,   "PWM.S", "%02X", MAKEPTR(inst->pwm_speed), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_PWMDEPTH,   "PWM.D", "%02X", MAKEPTR(inst->pwm_depth), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_PWMSHAPE,   "PWM.SH", "%c", inst->pwm_shape < 5 ? MAKEPTR(inst->pwm_shape + 0xf4) : MAKEPTR(inst->pwm_shape + 0xed), 1);
			update_rect(&frame, &r);
			inst_text(event, &r, P_PWMDELAY,   "PWM.DEL", "%02X", MAKEPTR(inst->pwm_delay), 2); //wasn't there
			update_rect(&frame, &r);
			
			inst_text(event, &r, P_TREMSPEED,   "TR.S", "%02X", MAKEPTR(inst->tremolo_speed), 2); //wasn't there
			update_rect(&frame, &r);
			inst_text(event, &r, P_TREMDEPTH,   "TR.D", "%02X", MAKEPTR(inst->tremolo_depth), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_TREMSHAPE,   "TR.SH", "%c", inst->tremolo_shape < 5 ? MAKEPTR(inst->tremolo_shape + 0xf4) : MAKEPTR(inst->tremolo_shape + 0xed), 1);
			update_rect(&frame, &r);
			inst_text(event, &r, P_TREMDELAY,   "TR.DEL", "%02X", MAKEPTR(inst->tremolo_delay), 2);
			update_rect(&frame, &r);
			
			//inst_text(event, &r, P_PROGPERIOD, "P.PRD", "%02X", MAKEPTR(inst->prog_period[mused.current_instrument_program]), 2);
			//update_rect(&frame, &r);
			
			temp666 = r.w;
			r.w = 110;
			
			inst_flags(event, &r, P_NORESTART, "NO RESTART", &inst->flags, MUS_INST_NO_PROG_RESTART);
			update_rect(&frame, &r);
			inst_flags(event, &r, P_MULTIOSC, "MULTIOSC", &inst->flags, MUS_INST_MULTIOSC);
			update_rect(&frame, &r);
			
			
			inst_flags(event, &r, P_SAVE_LFO_SETTINGS, "SAVE LFO SET.", &inst->flags, MUS_INST_SAVE_LFO_SETTINGS);
			//update_rect(&frame, &r);
			
			r.w = temp666;
			
			r.y += 10;
			r.x = frame.x;
		}
		
		//my_separator(&frame, &r);
		
		if(mused.show_fm_settings)
		{
			inst_flags(event, &r, P_FM_ENABLE, "FM", &inst->cydflags, CYD_CHN_ENABLE_FM);
			update_rect(&frame, &r);
			inst_text(event, &r, P_FM_MODULATION, "VOL", "%02X", MAKEPTR(inst->fm_modulation), 2);
			update_rect(&frame, &r);
			
			temp2 = r.w;
			
			r.w = 7 * 8 + 15;
			
			inst_flags(event, &r, P_FM_VOL_KSL_ENABLE, "FM V.KSL", &inst->fm_flags, CYD_FM_ENABLE_VOLUME_KEY_SCALING);
			update_rect(&frame, &r);
			
			r.w = 8 * 2 + 18;
			r.x = frame.x + frame.w / 2 - r.w - 2;
			
			inst_text(event, &r, P_FM_VOL_KSL_LEVEL, "", "%02X", MAKEPTR(inst->fm_vol_ksl_level), 2);
			update_rect(&frame, &r);
			
			r.x = frame.x + frame.w / 2 + 2;
			r.w = 7 * 8 + 15;
			
			inst_flags(event, &r, P_FM_ENV_KSL_ENABLE, "FM E.KSL", &inst->fm_flags, CYD_FM_ENABLE_ENVELOPE_KEY_SCALING);
			update_rect(&frame, &r);
			
			r.w = 8 * 2 + 18;
			r.x = frame.x + frame.w - r.w;
			
			inst_text(event, &r, P_FM_ENV_KSL_LEVEL, "", "%02X", MAKEPTR(inst->fm_env_ksl_level), 2);
			update_rect(&frame, &r);
			
			//r.y += 10;
			//r.x = frame.x;
			r.w = temp2;
			
			inst_text(event, &r, P_FM_FEEDBACK, "FEEDBACK", "%01X", MAKEPTR(inst->fm_feedback), 1);
			update_rect(&frame, &r);
			
			tmp = r.w;
			r.w -= 27;
			inst_text(event, &r, P_FM_HARMONIC_CARRIER, "MUL", "%01X", MAKEPTR(inst->fm_harmonic >> 4), 1);
			r.x += r.w + 11;
			r.w = 16;

			inst_text(event, &r, P_FM_HARMONIC_MODULATOR, "", "%01X", MAKEPTR(inst->fm_harmonic & 15), 1);
			update_rect(&frame, &r);
			
			r.w = 72; //wasn't there

			inst_text(event, &r, P_FM_BASENOTE, "BASE", "%s", notename(inst->fm_base_note), 3);
			update_rect(&frame, &r);
			
			r.w = 46;
			inst_text(event, &r, P_FM_FINETUNE, "", "%+4d", MAKEPTR(inst->fm_finetune), 4);
			update_rect(&frame, &r);
			
			r.w = init_width / 2 - 2;
			
			r.x = r.x - 124 + init_width / 2;
			
			const char* freq_luts[] = { "OPL", "OPN" };
			
			inst_text(event, &r, P_FM_FREQ_LUT, "FREQ. TABLE", "%s", (char*)freq_luts[inst->fm_freq_LUT], 3);
			update_rect(&frame, &r);
			
			r.w = tmp;
			
			inst_text(event, &r, P_FM_ATTACK, "ATK", "%02X", MAKEPTR(inst->fm_adsr.a), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_FM_DECAY, "DEC", "%02X", MAKEPTR(inst->fm_adsr.d), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_FM_SUSTAIN, "SUS", "%02X", MAKEPTR(inst->fm_adsr.s), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_FM_RELEASE, "REL", "%02X", MAKEPTR(inst->fm_adsr.r), 2);
			update_rect(&frame, &r);
			
			temp = r.w;
			r.w = 82;
			
			inst_flags(event, &r, P_FM_EXP_VOL, "FM EXP.VOL", &inst->fm_flags, CYD_FM_ENABLE_EXPONENTIAL_VOLUME);
			update_rect(&frame, &r);
			
			r.w = 35;
			
			inst_flags(event, &r, P_FM_EXP_ATTACK, "ATK", &inst->fm_flags, CYD_FM_ENABLE_EXPONENTIAL_ATTACK);
			update_rect(&frame, &r);
			
			//r.w = init_width / 2 - 2;
			
			r.x = r.x - 123 + init_width / 2;
			
			inst_flags(event, &r, P_FM_EXP_DECAY, "DEC", &inst->fm_flags, CYD_FM_ENABLE_EXPONENTIAL_DECAY);
			update_rect(&frame, &r);
			
			r.w = init_width / 2 - 39;
			
			inst_flags(event, &r, P_FM_EXP_RELEASE, "REL", &inst->fm_flags, CYD_FM_ENABLE_EXPONENTIAL_RELEASE);
			update_rect(&frame, &r);
			
			r.w = temp;
			
			inst_text(event, &r, P_FM_ENV_START, "E.START", "%02X", MAKEPTR(inst->fm_attack_start), 2);
			update_rect(&frame, &r);
			tmp = r.w;
			r.w = 42;
			inst_flags(event, &r, P_FM_WAVE, "WAVE", &inst->fm_flags, CYD_FM_ENABLE_WAVE);
			r.x += 44;
			r.w = 32;
			inst_text(event, &r, P_FM_WAVE_ENTRY, "", "%02X", MAKEPTR(inst->fm_wave), 2);
			update_rect(&frame, &r);
			
			r.w = tmp;
			r.y += 10;
			r.x -= 42 + 32 + 2 + 8 + tmp;
			
			inst_text(event, &r, P_FM_VIBSPEED,   "FM VIB.S", "%02X", MAKEPTR(inst->fm_vibrato_speed), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_FM_VIBDEPTH,   "FM VIB.D", "%02X", MAKEPTR(inst->fm_vibrato_depth), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_FM_VIBSHAPE,   "FM VIB.SH", "%c", inst->fm_vibrato_shape < 5 ? MAKEPTR(inst->fm_vibrato_shape + 0xf4) : MAKEPTR(inst->fm_vibrato_shape + 0xed), 1);
			update_rect(&frame, &r);
			inst_text(event, &r, P_FM_VIBDELAY,   "FM V.DEL", "%02X", MAKEPTR(inst->fm_vibrato_delay), 2);
			update_rect(&frame, &r);
			
			inst_text(event, &r, P_FM_TREMSPEED,   "FM TR.S", "%02X", MAKEPTR(inst->fm_tremolo_speed), 2); //wasn't there
			update_rect(&frame, &r);
			inst_text(event, &r, P_FM_TREMDEPTH,   "FM TR.D", "%02X", MAKEPTR(inst->fm_tremolo_depth), 2);
			update_rect(&frame, &r);
			inst_text(event, &r, P_FM_TREMSHAPE,   "FM TR.SH", "%c", inst->fm_tremolo_shape < 5 ? MAKEPTR(inst->fm_tremolo_shape + 0xf4) : MAKEPTR(inst->fm_tremolo_shape + 0xed), 1);
			update_rect(&frame, &r);
			inst_text(event, &r, P_FM_TREMDELAY,   "FM TR.DEL", "%02X", MAKEPTR(inst->fm_tremolo_delay), 2);
			update_rect(&frame, &r);
			
			inst_flags(event, &r, P_FM_ADDITIVE, "ADDITIVE", &inst->fm_flags, CYD_FM_ENABLE_ADDITIVE);
			update_rect(&frame, &r);
			
			inst_flags(event, &r, P_FM_SAVE_LFO_SETTINGS, "SAVE FM LFO SET.", &inst->fm_flags, CYD_FM_SAVE_LFO_SETTINGS);
			update_rect(&frame, &r);
			
			inst_flags(event, &r, P_FM_ENABLE_4OP, "4-OP", &inst->fm_flags, CYD_FM_ENABLE_4OP);
			update_rect(&frame, &r);
			
			button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "OPEN MENU", open_4op, NULL, NULL, NULL);
			update_rect(&frame, &r);
		}

		if(mused.show_timer_lfo_settings)
		{
			r.w = 90;

			inst_flags(event, &r, P_ENABLE_PHASE_RESET_TIMER, "PHASE RESET", &inst->cydflags, CYD_CHN_ENABLE_PHASE_RESET_TIMER);
			update_rect(&frame, &r);
			
			SDL_Rect note;
			copy_rect(&note, &r);

			note.w = 40;
			note.h = 10;

			inst_text(event, &note, P_PHASE_RESET_TIMER_NOTE, "", "%s", notename(inst->phase_reset_timer_note), 3);
			note.x += note.w + 2;
			note.w += 8;
			inst_text(event, &note, P_PHASE_RESET_TIMER_FINETUNE, "", "%+4d", MAKEPTR(inst->phase_reset_timer_finetune), 4);
			
			//r.w = 122;
			r.x += 92;
			//r.y += 10;
			r.w += 18 + 1;
			
			inst_flags(event, &r, P_LINK_PHASE_RESET_TIMER_NOTE, "LINK TO NOTE", (Uint32*)&inst->flags, MUS_INST_LINK_PHASE_RESET_TIMER_NOTE);

			r.y += 10;
			r.x = frame.x;
		}
		
		r.w = init_width / 3 - 2;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, (!(mused.show_point_envelope_editor) && !(mused.show_local_samples)) ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "PROGRAM", open_prog, NULL, NULL, NULL);
		update_rect(&frame, &r);
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, (mused.show_point_envelope_editor && !(mused.show_local_samples)) ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "ENVELOPE", open_env, NULL, NULL, NULL);
		update_rect(&frame, &r);
		
		r.w = frame.x + frame.w - r.x;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, (!(mused.show_point_envelope_editor) && mused.show_local_samples) ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "LOCAL SAMPLES", open_local_samples, NULL, NULL, NULL);
	}
}

void open_4op(void *unused1, void *unused2, void *unused3)
{
	snapshot(S_T_MODE);
	
	mused.show_four_op_menu = true;

	change_mode(EDIT4OP);
	
	mused.selected_operator = 1;
	
	if(mused.song.instrument[mused.current_instrument].alg == 0)
	{
		mused.song.instrument[mused.current_instrument].alg = 1;
	}
	
	for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
	{
		mused.fourop_program_position[i] = 0;
	}

	set_cursor(NULL);
}

void open_prog(void *unused1, void *unused2, void *unused3)
{
	mused.show_point_envelope_editor = false;
	
	mused.vol_env_point = -1;
	mused.vol_env_scale = 1;
	mused.vol_env_horiz_scroll = 0;
	
	mused.pan_env_point = -1;
	mused.pan_env_scale = 1;
	mused.pan_env_horiz_scroll = 0;
	
	mused.point_env_editor_scroll = 0;
	
	mused.show_local_samples = false;

	mused.focus = EDITPROG;

	set_cursor(NULL);
}

void open_env(void *unused1, void *unused2, void *unused3)
{
	mused.show_point_envelope_editor = true;
	
	mused.current_volume_envelope_point = 0;
	mused.current_panning_envelope_point = 0;
	mused.env_selected_param = 0;
	
	mused.show_local_samples = false;

	mused.focus = EDITENVELOPE;

	set_cursor(NULL);
}

void open_local_samples(void *unused1, void *unused2, void *unused3)
{
	snapshot(S_T_MODE);
	
	mused.show_local_samples = true;
	mused.local_sample_list_position = 0;
	
	mused.local_sample_note_list_position = MIDDLE_C;
	
	mused.show_local_samples_list = true;
	
	change_mode(EDITLOCALSAMPLE);
	
	mused.local_sample_param = LS_ENABLE;

	set_cursor(NULL);

	invalidate_wavetable_view();
}


void instrument_list(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	if(!(mused.show_four_op_menu) || ((mused.focus != EDIT4OP) && (mused.focus != EDITPROG4OP)))
	{
		SDL_Rect area;
		copy_rect(&area, dest);
		console_set_clip(mused.console, &area);
		console_clear(mused.console);
		bevelex(domain,&area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
		adjust_rect(&area, 3);
		console_set_clip(mused.console, &area);

		int y = area.y;

		int start = mused.instrument_list_position;

		for (int i = start; i < NUM_INSTRUMENTS && y < area.h + area.y; ++i, y += mused.console->font.h)
		{
			SDL_Rect row = { area.x - 1, y - 1, area.w + 2, mused.console->font.h + 1};

			if (i == mused.current_instrument)
			{
				bevel(domain, &row, mused.slider_bevel, BEV_SELECTED_PATTERN_ROW);
				console_set_color(mused.console, colors[COLOR_INSTRUMENT_SELECTED]);
			}
			
			else
			{
				console_set_color(mused.console, colors[COLOR_INSTRUMENT_NORMAL]);
			}

			char temp[sizeof(mused.song.instrument[i].name) + 1];

			strcpy(temp, mused.song.instrument[i].name);
			temp[my_min(sizeof(mused.song.instrument[i].name), my_max(0, area.w / mused.console->font.w - 3))] = '\0';

			console_write_args(mused.console, "%02X %-16s\n", i, temp);

			check_event(event, &row, select_instrument, MAKEPTR(i), 0, 0);

			slider_set_params(&mused.instrument_list_slider_param, 0, NUM_INSTRUMENTS - 1, start, i, &mused.instrument_list_position, 1, SLIDER_VERTICAL, mused.slider_bevel);
		}

		if (mused.focus == EDITINSTRUMENT)
			check_mouse_wheel_event(event, dest, &mused.instrument_list_slider_param);
	}
}


static void fx_text(const SDL_Event *e, const SDL_Rect *area, int p, const char *_label, const char *format, void *value, int width)
{
	int d = generic_field(e, area, EDITFX, p, _label, format, value, width);
	if (d)
	{
		if (p >= 0) mused.selected_param = p;
		if (p != R_FX_BUS) snapshot_cascade(S_T_FX, mused.fx_bus, p);
		if (d < 0) mused.fx_bus = my_max(0, mused.fx_bus - 1);
		else if (d > 0) mused.fx_bus = my_min(CYD_MAX_FX_CHANNELS - 1, mused.fx_bus + 1);
	}
}


void fx_name_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect farea, larea, tarea;
	copy_rect(&farea,dest);
	copy_rect(&larea,dest);
	copy_rect(&tarea,dest);

	farea.w = 2 * mused.console->font.w + 2 + 16;

	larea.w = 16;

	label("FX", &larea);

	tarea.w = dest->w - farea.w - larea.w - 1;
	farea.x = larea.w + dest->x;
	tarea.x = farea.x + farea.w;

	fx_text(event, &farea, R_FX_BUS, "FX", "%02X", MAKEPTR(mused.fx_bus), 2);
	inst_field(event, &tarea, R_FX_BUS_NAME, sizeof(mused.song.fx[mused.fx_bus].name), mused.song.fx[mused.fx_bus].name);

	if (is_selected_param(EDITFX, R_FX_BUS_NAME) || (mused.mode == EDITFX && (mused.edit_buffer == mused.song.fx[mused.fx_bus].name && mused.focus == EDITBUFFER)))
	{
		SDL_Rect r;
		copy_rect(&r, &tarea);
		adjust_rect(&r, -2);
		set_cursor(&r);
	}
}


void fx_global_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain,&area, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 2);
	console_set_clip(mused.console, &area);
	SDL_Rect r;
	copy_rect(&r, &area);
	r.x += 2;
	r.h = 10;

	r.w = 96;

	generic_flags(event, &r, EDITFX, R_MULTIPLEX, "MULTIPLEX", &mused.song.flags, MUS_ENABLE_MULTIPLEX);
	update_rect(&area, &r);

	int d;

	r.x = 100;
	r.w = 80;

	if ((d = generic_field(event, &r, EDITFX, R_MULTIPLEX_PERIOD, "PERIOD", "%2X", MAKEPTR(mused.song.multiplex_period), 2)))
	{
		mused.edit_reverb_param = R_MULTIPLEX_PERIOD;
		fx_add_param(d);
	}

	update_rect(&area, &r);

	r.x += 8;
	r.w = 120;

	if ((d = generic_field(event, &r, EDITFX, R_PITCH_INACCURACY, "INACCURACY", "%2d", MAKEPTR(mused.song.pitch_inaccuracy), 2)))
	{
		mused.edit_reverb_param = R_PITCH_INACCURACY;
		fx_add_param(d);
	}
}


void fx_reverb_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 4);
	console_set_clip(mused.console, &area);

	int c = 0;

	int row_ms = (1000.0 / (float)mused.song.song_rate) * mused.song.song_speed;
	int row_ms2 = (1000.0 / (float)mused.song.song_rate) * mused.song.song_speed2;
	
	if(row_ms == 0 || row_ms2 == 0) goto skip;
	
	for (int ms = 0; ms < CYDRVB_SIZE / 4; c++) //was for (int ms = 0; ms < CYDRVB_SIZE; c++)
	{
		SDL_Rect r = { area.x + ms * area.w / (CYDRVB_SIZE / 4), area.y, 1, area.h}; //was SDL_Rect r = { area.x + ms * area.w / CYDRVB_SIZE, area.y, 1, area.h};

		if (ms > 0)
		{
			Uint32 color = timesig(c, colors[COLOR_PATTERN_BAR], colors[COLOR_PATTERN_BEAT], colors[COLOR_PATTERN_NORMAL]);

			gfx_rect(dest_surface, &r, color);
		}

		if (timesig(c, 1, 1, 0))
		{
			SDL_Rect text = { r.x + 2, r.y + r.h - mused.smallfont.h, 16, mused.smallfont.h};
			font_write_args(&mused.smallfont, domain, &text, "%d", c * 4); //was font_write_args(&mused.smallfont, domain, &text, "%d", c);
		}

		if (c & 1)
			ms += row_ms2;
		else
			ms += row_ms;
	}
	
	skip:;
	
	c = 0;

	if (mused.fx_axis == 0)
	{
		for (int db = 0; db < -CYDRVB_LOW_LIMIT; db += 100, c++)
		{
			Uint32 color = colors[COLOR_PATTERN_BAR];
			if (c & 1)
				color = colors[COLOR_PATTERN_BEAT];

			SDL_Rect r = { area.x, area.y + db * area.h / -CYDRVB_LOW_LIMIT, area.w, 1};

			if (!(c & 1))
			{
				SDL_Rect text = { r.x + r.w - 40, r.y + 2, 40, 8};
				font_write_args(&mused.smallfont, domain, &text, "%3d dB", -db / 10);
			}

			if (db != 0)
				gfx_rect(dest_surface, &r, color);
		}
	}
	
	else
	{
		for (int pan = CYD_PAN_LEFT; pan < CYD_PAN_RIGHT; pan += CYD_PAN_CENTER / 2, c++)
		{
			Uint32 color = colors[COLOR_PATTERN_BAR];
			if (c & 1)
				color = colors[COLOR_PATTERN_BEAT];

			SDL_Rect r = { area.x, area.y + pan * area.h / CYD_PAN_RIGHT, area.w, 1};

			if (pan != 0)
				gfx_rect(dest_surface, &r, color);
		}

		{
			SDL_Rect text = { area.x + area.w - 6, area.y + 4, 8, 8};
			font_write(&mused.smallfont, domain, &text, "L");
		}
		
		{
			SDL_Rect text = { area.x + area.w - 6, area.y + area.h - 16, 8, 8};
			font_write(&mused.smallfont, domain, &text, "R");
		}
	}

	for (int i = 0; i < CYDRVB_TAPS; ++i)
	{
		int h;

		if (mused.fx_axis == 0)
			h = mused.song.fx[mused.fx_bus].rvb.tap[i].gain * area.h / CYDRVB_LOW_LIMIT;
		else
			h = mused.song.fx[mused.fx_bus].rvb.tap[i].panning * area.h / CYD_PAN_RIGHT;

		SDL_Rect r = { area.x + mused.song.fx[mused.fx_bus].rvb.tap[i].delay * area.w / CYDRVB_SIZE - mused.smallfont.w / 2,
			area.y + h - mused.smallfont.h / 2, mused.smallfont.w, mused.smallfont.h};

		if (mused.song.fx[mused.fx_bus].rvb.tap[i].flags & 1)
			font_write(&mused.smallfont, dest_surface, &r, "\2");
		else
			font_write(&mused.smallfont, dest_surface, &r, "\1");

		if (i == mused.fx_tap)
		{
			SDL_Rect sel;
			copy_rect(&sel, &r);
			adjust_rect(&sel, -4);
			bevelex(domain, &sel, mused.slider_bevel, BEV_CURSOR, BEV_F_STRETCH_ALL);
		}

		if (event->type == SDL_MOUSEBUTTONDOWN)
		{
			if (check_event(event, &r, NULL, 0, 0, 0))
			{
				mused.fx_tap = i;

				if (SDL_GetModState() & KMOD_SHIFT)
				{
					mused.song.fx[mused.fx_bus].rvb.tap[i].flags ^= 1;
				}
			}
		}
	}

	int mx, my;
	
	if (mused.mode == EDITFX && (SDL_GetMouseState(&mx, &my) & SDL_BUTTON(1)))
	{
		mx /= mused.pixel_scale;
		my /= mused.pixel_scale;

		if (mx >= area.x && mx < area.x + area.w && my > area.y && my < area.y + area.h)
		{
			if (mused.fx_room_prev_x != -1)
			{
				snapshot_cascade(S_T_FX, mused.fx_bus, mused.fx_tap);

				mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].delay += (mx - mused.fx_room_prev_x) * CYDRVB_SIZE / area.w;

				if (mused.fx_axis == 0)
				{
					mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].gain += (my - mused.fx_room_prev_y) * CYDRVB_LOW_LIMIT / area.h;
					if (mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].gain > 0)
						mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].gain = 0;
					else if (mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].gain < CYDRVB_LOW_LIMIT)
						mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].gain = CYDRVB_LOW_LIMIT;
				}
				
				else
				{
					mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].panning += (my - mused.fx_room_prev_y) * CYD_PAN_RIGHT / area.h;

					if (mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].panning < CYD_PAN_LEFT)
						mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].panning = CYD_PAN_LEFT;

					if (mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].panning > CYD_PAN_RIGHT)
						mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].panning = CYD_PAN_RIGHT;
				}
			}

			mused.fx_room_prev_x = mx;
			mused.fx_room_prev_y = my;
		}
	}
	
	else
	{
		mused.fx_room_prev_x = -1;
		mused.fx_room_prev_y = -1;
	}
}


void fx_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain,&area, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 4);
	console_set_clip(mused.console, &area);
	SDL_Rect r;
	copy_rect(&r, &area);

	int d;

	r.h = 10;
	r.w = 48;

	generic_flags(event, &r, EDITFX, R_CRUSH, "CRUSH", &mused.song.fx[mused.fx_bus].flags, CYDFX_ENABLE_CRUSH);
	update_rect(&area, &r);

	r.x = 56;
	r.w = 56;

	if ((d = generic_field(event, &r, EDITFX, R_CRUSHBITS, "BITS", "%01X", MAKEPTR(mused.song.fx[mused.fx_bus].crush.bit_drop), 1)))
	{
		fx_add_param(d);
	}

	update_rect(&area, &r);

	r.w = 64;

	if ((d = generic_field(event, &r, EDITFX, R_CRUSHDOWNSAMPLE, "DSMP", "%02d", MAKEPTR(mused.song.fx[mused.fx_bus].crushex.downsample), 2)))
	{
		fx_add_param(d);
	}

	update_rect(&area, &r);

	generic_flags(event, &r, EDITFX, R_CRUSHDITHER, "DITHER", &mused.song.fx[mused.fx_bus].flags, CYDFX_ENABLE_CRUSH_DITHER);
	update_rect(&area, &r);

	r.w = 56;

	if ((d = generic_field(event, &r, EDITFX, R_CRUSHGAIN, "VOL", "%02X", MAKEPTR(mused.song.fx[mused.fx_bus].crushex.gain), 2)))
	{
		fx_add_param(d);
	}
	update_rect(&area, &r);

	my_separator(&area, &r);

	r.w = 60;

	generic_flags(event, &r, EDITFX, R_CHORUS, "STEREO", &mused.song.fx[mused.fx_bus].flags, CYDFX_ENABLE_CHORUS);

	update_rect(&area, &r);

	{
		// hacky-hack, we need different addresses for the different fields
		// because the address of the string is used as an ad-hoc identifier for
		// the fields...

		char temp1[10], temp2[10], temp3[10];

		sprintf(temp1, "%4.1f ms", (float)mused.song.fx[mused.fx_bus].chr.min_delay / 10);

		r.x = 100;
		r.w = 104;

		if ((d = generic_field(event, &r, EDITFX, R_MINDELAY, "MIN", temp1, NULL, 7)))
		{
			fx_add_param(d);
		}

		update_rect(&area, &r);

		sprintf(temp2, "%4.1f ms", (float)mused.song.fx[mused.fx_bus].chr.max_delay / 10);

		if ((d = generic_field(event, &r, EDITFX, R_MAXDELAY, "MAX", temp2, NULL, 7)))
		{
			fx_add_param(d);
		}

		r.x = 100;
		r.y += r.h;

		if ((d = generic_field(event, &r, EDITFX, R_SEPARATION, "PHASE", "%02X", MAKEPTR(mused.song.fx[mused.fx_bus].chr.sep), 2)))
		{
			fx_add_param(d);
		}

		update_rect(&area, &r);

		if (mused.song.fx[mused.fx_bus].chr.rate != 0)
			sprintf(temp3, "%5.2f Hz", (((float)(mused.song.fx[mused.fx_bus].chr.rate - 1) + 10) / 40));
		else
			strcpy(temp3, "OFF");

		if ((d = generic_field(event, &r, EDITFX, R_RATE, "MOD", temp3, NULL, 8)))
		{
			fx_add_param(d);
		}

		update_rect(&area, &r);
	}

	my_separator(&area, &r);

	generic_flags(event, &r, EDITFX, R_ENABLE, "REVERB", &mused.song.fx[mused.fx_bus].flags, CYDFX_ENABLE_REVERB);

	update_rect(&area, &r);

	r.w = 80;

	r.x = 4;
	r.y += r.h;

	{
		r.x = 130;
		r.y -= r.h;

		int tmp = r.w;
		r.w = 60 + 32;

		if ((d = generic_field(event, &r, EDITFX, R_ROOMSIZE, "ROOMSIZE", "%02X", MAKEPTR(mused.fx_room_size), 2)))
		{
			fx_add_param(d);
		}

		update_rect(&area, &r);

		r.w = 320 - 8 - r.x;

		if ((d = generic_field(event, &r, EDITFX, R_ROOMVOL, "VOLUME", "%02X", MAKEPTR(mused.fx_room_vol), 2)))
		{
			fx_add_param(d);
		}

		r.x = 130;
		r.y += r.h;

		r.w = 60+32;

		if ((d = generic_field(event, &r, EDITFX, R_ROOMDECAY, "DECAY", "%d", MAKEPTR(mused.fx_room_dec), 1)))
		{
			fx_add_param(d);
		}

		update_rect(&area, &r);

		r.w = 41;

		generic_flags(event, &r, EDITFX, R_SNAPTICKS, "SNAP", (Uint32*)&mused.fx_room_ticks, 1);

		update_rect(&area, &r);

		if (button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "SET", NULL, NULL, NULL, NULL) & 1)
		{
			set_room_size(mused.fx_bus, mused.fx_room_size, mused.fx_room_vol, mused.fx_room_dec);
		}

		update_rect(&area, &r);

		r.w = tmp;
	}

	{
		my_separator(&area, &r);

		char value[20];
		int d;

		r.w = 32;
		r.h = 10;

		Uint32 _flags = mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].flags;

		generic_flags(event, &r, EDITFX, R_TAPENABLE, "TAP", &_flags, 1);

		mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].flags = _flags;

		update_rect(&area, &r);

		r.w = 32;

		if ((d = generic_field(event, &r, EDITFX, R_TAP, "", "%02d", MAKEPTR(mused.fx_tap), 2)))
		{
			fx_add_param(d);
		}
		
		if((mused.fx_tap > mused.song.fx[mused.fx_bus].rvb.taps_quant) && (mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].flags == 1))
		{
			mused.song.fx[mused.fx_bus].rvb.taps_quant = mused.fx_tap + 1;
		}

		update_rect(&area, &r);

		r.w = 80;

		if (mused.flags & SHOW_DELAY_IN_TICKS)
		{
			char tmp[10];
			float ticks = (float)mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].delay / (1000 / (float)mused.song.song_rate);
			snprintf(tmp, sizeof(tmp), "%5.2f", ticks);
			d = generic_field(event, &r, EDITFX, R_DELAY, "", "%s t", tmp, 7);
		}
		
		else
		{
			d = generic_field(event, &r, EDITFX, R_DELAY, "", "%4d ms", MAKEPTR(mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].delay), 7);
		}

		if (d)
		{
			fx_add_param(d);
		}

		update_rect(&area, &r);

		r.w = 80;

		if (mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].gain <= CYDRVB_LOW_LIMIT)
			strcpy(value, "- INF");
		else
			sprintf(value, "%+5.1f", (double)mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].gain * 0.1);

		if ((d = generic_field(event, &r, EDITFX, R_GAIN, "", "%s dB", value, 8)))
		{
			fx_add_param(d);
		}

		r.x += r.w + 4;

		r.w = 32;

		int panning = (int)mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].panning - CYD_PAN_CENTER;
		char tmp[10];

		if (panning != 0)
			snprintf(tmp, sizeof(tmp), "%c%X", panning < 0 ? '\xf9' : '\xfa', panning == 127 ? 8 : ((abs((int)panning) >> 3) & 0xf));
		else
			strcpy(tmp, "\xfa\xf9");

		if ((d = generic_field(event, &r, EDITFX, R_PANNING, "", "%s", tmp, 2)))
		{
			fx_add_param(d);
		}

		r.x += r.w + 4;

		r.w = 32;

		if (button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, mused.fx_axis == 0 ? "GAIN" : "PAN", NULL, NULL, NULL, NULL) & 1)
		{
			mused.fx_axis ^= 1;
		}
		
		r.x += r.w + 4;
		r.w = 65;
		
		if ((d = generic_field(event, &r, EDITFX, R_NUM_TAPS, "TAPS", "%02d", MAKEPTR(mused.song.fx[mused.fx_bus].rvb.taps_quant), 2))) //wasn't there
		{
			fx_add_param(d);
			
			for(int i = 0; i < CYDRVB_TAPS; i++)
			{
				if(i < mused.song.fx[mused.fx_bus].rvb.taps_quant)
				{
					mused.song.fx[mused.fx_bus].rvb.tap[i].flags = 1;
				}
				
				else
				{
					mused.song.fx[mused.fx_bus].rvb.tap[i].flags = 0;
				}
			}
		}

		r.y += r.h + 4;
		r.h = area.h - r.y + area.y;
		r.w = area.w;
		r.x = area.x;

		fx_reverb_view(dest_surface, &r, event, param);
	}

	mirror_flags();
}


void instrument_disk_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	//mused.import_mml_string = false;
	
	if(!(mused.show_four_op_menu) || ((mused.focus != EDIT4OP) && (mused.focus != EDITPROG4OP) && (mused.focus != EDITENVELOPE4OP)))
	{
		SDL_Rect area;
		copy_rect(&area, dest);
		console_set_clip(mused.console, &area);
		console_clear(mused.console);
		bevelex(domain,&area, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
		adjust_rect(&area, 2);

		SDL_Rect button = { area.x + 2, area.y, area.w / 3 - 8, area.h };

		int open = button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "LOAD", NULL, MAKEPTR(1), NULL, NULL);
		//update_rect(&area, &button);
		
		button.x += button.w;
		
		int save = button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "SAVE", NULL, MAKEPTR(2), NULL, NULL);
		//update_rect(&area, &button);
		
		button.x += button.w;
		
		button.w += 20;
		
		int import_mml_string = button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "IMP.MML", NULL, MAKEPTR(3), NULL, NULL);
		update_rect(&area, &button);

		if (open & 1) open_data(param, MAKEPTR(OD_A_OPEN), 0);
		else if (save & 1) open_data(param, MAKEPTR(OD_A_SAVE), 0);
		//else if (import_mml_string & 1) import_mml_fm_instrument_string(&mused.song.instrument[mused.current_instrument]);
		else if (import_mml_string & 1)
		{
			mused.import_mml_string = true;
		}
	}
}

void fx_disk_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain,&area, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 2);

	SDL_Rect button = { area.x + 2, area.y, area.w / 2 - 4, area.h };

	int open = button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "LOAD", NULL, MAKEPTR(1), NULL, NULL);
	update_rect(&area, &button);
	int save = button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "SAVE", NULL, MAKEPTR(2), NULL, NULL);
	update_rect(&area, &button);

	if (open & 1) open_data(param, MAKEPTR(OD_A_OPEN), 0);
	else if (save & 1) open_data(param, MAKEPTR(OD_A_SAVE), 0);
}

void wave_disk_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain,&area, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 2);

	SDL_Rect button = { area.x + 2, area.y, area.w / 2 - 4, area.h };

	int open = button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "LOAD", NULL, MAKEPTR(1), NULL, NULL);
	update_rect(&area, &button);
	int save = button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "SAVE", NULL, MAKEPTR(2), NULL, NULL);
	update_rect(&area, &button);

	if (open & 1) open_data(param, MAKEPTR(OD_A_OPEN), 0);
	else if (save & 1) open_data(param, MAKEPTR(OD_A_SAVE), 0);
}

void local_sample_disk_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain,&area, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 2);

	SDL_Rect button = { area.x + 2, area.y, 34, area.h };

	int open = button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "LOAD", NULL, MAKEPTR(1), NULL, NULL);
	button.x += button.w;
	int save = button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "SAVE", NULL, MAKEPTR(2), NULL, NULL);
	button.x += button.w;
	
	button.w = area.w - 34 * 2 - 4;
	
	int import_wavetable_sequence = button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "IMP.WAVETABLE", NULL, MAKEPTR(2), NULL, NULL);

	if (open & 1) open_data(param, MAKEPTR(OD_A_OPEN), 0);
	else if (save & 1) open_data(param, MAKEPTR(OD_A_SAVE), 0);
	
	if(import_wavetable_sequence & 1)
	{
		mused.import_wavetable_string = true;
	}
}

void song_name_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect larea, farea;
	copy_rect(&larea, dest);
	copy_rect(&farea, dest);
	larea.w = 32;
	farea.w -= larea.w;
	farea.x += larea.w;
	label("SONG", &larea);

	inst_field(event, &farea, 0, MUS_SONG_TITLE_LEN, mused.song.title);
}


void bevel_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	bevelex(domain, dest, mused.slider_bevel, CASTPTR(int, param), BEV_F_STRETCH_ALL);
}


void sequence_spectrum_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	if (mused.flags & SHOW_LOGO)
	{
		if (mused.logo != NULL)
		{
			SDL_Rect a;
			copy_rect(&a, dest);
			a.w += SCROLLBAR;
			gfx_rect(dest_surface, &a, colors[COLOR_OF_BACKGROUND]);
			SDL_Rect d, s = {0,0,a.w,a.h};
			gfx_domain_set_clip(domain, &a);
			copy_rect(&d, &a);
			d.x = d.w / 2 - mused.logo->surface->w / 2 + d.x;
			d.w = mused.logo->surface->w;
			s.w = mused.logo->surface->w;
			my_BlitSurface(mused.logo, &s, dest_surface, &d);
			gfx_domain_set_clip(domain, NULL);
			
			if (check_event(event, &a, NULL, NULL, NULL, NULL))
			{
				mused.flags &= ~SHOW_LOGO;
			}
		}
		
		else
		{
			mused.flags &= ~SHOW_LOGO;
		}
	}
	
	else if (mused.flags & SHOW_ANALYZER)
	{
		SDL_Rect a;
		copy_rect(&a, dest);
		a.w += SCROLLBAR;
		gfx_domain_set_clip(dest_surface, &a);

		check_event(event, &a, toggle_visualizer, NULL, NULL, NULL);

		switch (mused.current_visualizer)
		{
			default:
			case VIS_SPECTRUM:
				spectrum_analyzer_view(dest_surface, &a, event, param);
				break;

			case VIS_CATOMETER:
				catometer_view(dest_surface, &a, event, param);
				break;
		}

		gfx_domain_set_clip(domain, NULL);
	}
	
	else
	{
		sequence_view2(dest_surface, dest, event, param);
	}
}


void toolbar_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect button;
	copy_rect(&button, dest);
	button.w = dest->w - 3 * (dest->h + 2);

	button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont,
		BEV_BUTTON, BEV_BUTTON_ACTIVE, 	"MENU", open_menu_action, 0, 0, 0);

	button.x += button.w;
	button.w = button.h + 2;

	if (button_event(domain, event, &button, mused.slider_bevel,
		!(mused.flags & SHOW_ANALYZER) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		!(mused.flags & SHOW_ANALYZER) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		DECAL_TOOLBAR_VISUALIZATIONS, flip_bit_action, &mused.flags, MAKEPTR(SHOW_ANALYZER), 0))
			mused.cursor.w = mused.cursor_target.w = 0;

	button.x += button.w;

	if (button_event(domain, event, &button, mused.slider_bevel,
		!(mused.flags & FULLSCREEN) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		!(mused.flags & FULLSCREEN) ? BEV_BUTTON : BEV_BUTTON_ACTIVE,
		DECAL_TOOLBAR_FULLSCREEN, NULL, 0, 0, 0))
	{
		toggle_fullscreen(0,0,0);
		return; // dest_surface is now invalid
	}

	button.x += button.w;

	
	button_event(domain, event, &button, mused.slider_bevel,
	BEV_BUTTON, BEV_BUTTON_ACTIVE, DECAL_TOOLBAR_QUIT, quit_action, 0, 0, 0);

	button.x += button.w;
}