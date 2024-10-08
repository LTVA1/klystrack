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

#include "action.h"
#include "optimize.h"
#include "mused.h"
#include "gui/toolutil.h"
#include "view.h"
#include "event.h"
#include "gui/msgbox.h"
#include "version.h"
#include "klystron_version.h"
#include "gfx/gfx.h"
#include "theme.h"
#include "key.h"
#include "gui/menu.h"
#include "export.h"
#include <stdbool.h>
#include "gui/mouse.h"
#include "view/wavetableview.h"
#include "view/songmessage.h"
#include "help.h"
#include <string.h>

#include "export/fzt.h"

#include "../klystron/src/snd/music.c"

extern Mused mused;
extern GfxDomain *domain;
extern const Menu mainmenu[];
extern Menu pixelmenu[];
extern Menu patternlengthmenu[];
extern Menu oversamplemenu[];

bool inside_undo = false;

void select_sequence_position(void *channel, void *position, void* unused)
{
	if ((mused.flags & SONG_PLAYING) && (mused.flags & FOLLOW_PLAY_POSITION))
		return;

	if (CASTPTR(int, channel) != -1)
		mused.current_sequencetrack = CASTPTR(int, channel);

	if (CASTPTR(int, position) < mused.song.song_length)
		mused.current_sequencepos = CASTPTR(int, position);

	mused.pattern_position = mused.current_patternpos = mused.current_sequencepos;

	mused.focus = EDITSEQUENCE;

	update_horiz_sliders();
}


void select_pattern_param(void *id, void *position, void *track)
{
	mused.current_patternx = CASTPTR(int, id);
	mused.current_sequencetrack = CASTPTR(int, track);
	mused.pattern_position = mused.current_sequencepos = CASTPTR(int, position);

	mused.focus = EDITPATTERN;
	
	int temp = mused.current_sequencetrack; //this should fix the unnoying as fuck thing when you launch klystrack and can't move cursor to the right from leftmost pattern

	update_horiz_sliders();
	
	mused.current_sequencetrack = temp;
}


void select_instrument_page(void *page, void *unused1, void *unused2)
{
	mused.instrument_page = CASTPTR(int, page) * 10;
	set_info_message("Selected instrument bank %d-%d", mused.instrument_page, mused.instrument_page + 9);
}


void select_instrument(void *idx, void *relative, void *pagey)
{
	if (pagey)
	{
		mused.current_instrument = mused.instrument_page + CASTPTR(int,idx);
	}
	else
	{
		if (relative)
			mused.current_instrument += CASTPTR(int,idx);
		else
			mused.current_instrument = CASTPTR(int, idx);
	}

	if (mused.current_instrument >= NUM_INSTRUMENTS)
		mused.current_instrument = NUM_INSTRUMENTS-1;
	else if (mused.current_instrument < 0)
		mused.current_instrument = 0;

}


void select_wavetable(void *idx, void *unused1, void *unused2)
{
	mused.selected_wavetable = CASTPTR(int,idx);

	if (mused.selected_wavetable >= CYD_WAVE_MAX_ENTRIES)
		mused.selected_wavetable = CYD_WAVE_MAX_ENTRIES-1;
	else if (mused.selected_wavetable < 0)
		mused.selected_wavetable = 0;

}

void select_local_sample(void *idx, void *unused1, void *unused2)
{
	int prev = mused.selected_local_sample;
	
	mused.selected_local_sample = CASTPTR(int, idx);

	if (mused.selected_local_sample >= MUS_MAX_INST_SAMPLES)
		mused.selected_local_sample = MUS_MAX_INST_SAMPLES-1;
	else if (mused.selected_local_sample < 0)
		mused.selected_local_sample = 0;
	
	if(!(mused.song.instrument[mused.current_instrument].local_samples[mused.selected_local_sample])) //don't allow to get invalid pointer
	{
		mused.selected_local_sample = prev;
	}
}

void select_local_sample_note(void *idx, void *unused1, void *unused2)
{
	mused.selected_local_sample_note = my_min(FREQ_TAB_SIZE - 1, CASTPTR(int, idx));
}

void change_octave(void *delta, void *unused1, void *unused2)
{
	mused.octave += CASTPTR(int,delta);
	if (mused.octave > 9) mused.octave = 9;
	else if (mused.octave < -5) mused.octave = -5;
}


void change_oversample(void *oversample, void *unused1, void *unused2)
{
	cyd_set_oversampling(&mused.cyd, CASTPTR(int,oversample));

	mused.oversample = CASTPTR(int,oversample);

	for (int i = 0; i < 4; ++i)
	{
		if (oversamplemenu[i].p1 == oversample)
			oversamplemenu[i].flags |= MENU_BULLET;
		else
			oversamplemenu[i].flags &= ~MENU_BULLET;
	}
}


void change_song_rate(void *delta, void *unused1, void *unused2)
{
	if (CASTPTR(int, delta) > 0)
	{
		if ((int)mused.song.song_rate + CASTPTR(int, delta) <= 44100)
			mused.song.song_rate += CASTPTR(int, delta);
		else
			mused.song.song_rate = 44100;
	}
	
	else if (CASTPTR(int, delta) < 0)
	{
		if ((int)mused.song.song_rate + CASTPTR(int, delta) >= 0x1)
			mused.song.song_rate += CASTPTR(int, delta);
		else
			mused.song.song_rate = 1;
	}

	enable_callback(true);
}


void change_time_signature(void *beat, void *unused1, void *unused2)
{
	if (!beat)
	{
		mused.time_signature = (mused.time_signature & 0x00ff) | (((((mused.time_signature >> 8) + 1) & 0xff) % 17) << 8);
		if ((mused.time_signature & 0xff00) == 0) mused.time_signature |= 0x100;
	}
	else
	{
		mused.time_signature = (mused.time_signature & 0xff00) | ((((mused.time_signature & 0xff) + 1) & 0xff) % 17);
		if ((mused.time_signature & 0xff) == 0) mused.time_signature |= 1;
	}
}


void play(void *from_cursor, void *unused1, void *unused2)
{
	int pos = from_cursor ? ((mused.flags2 & SNAP_PLAY_FROM_CURSOR_TO_NEAREST_PATTERN_START) ? mused.current_sequencepos : mused.current_patternpos) : 0;
	mused.play_start_at = get_playtime_at(pos);
	enable_callback(true);
	mus_set_song(&mused.mus, &mused.song, pos);
	mused.flags |= SONG_PLAYING;
	if (mused.flags & STOP_EDIT_ON_PLAY) mused.flags &= ~EDIT_MODE;
	
	mused.draw_passes_since_song_start = 0;
}


void play_position(void *unused1, void *unused2, void *unused3)
{
	if (!(mused.flags & LOOP_POSITION))
	{
		int p = current_pattern(), len;

		if (p < 0)
			len = mused.sequenceview_steps;
		else
			len = mused.song.pattern[p].num_steps;


		mused.flags |= LOOP_POSITION;
		mused.loop_store_length = mused.song.song_length;
		mused.loop_store_loop = mused.song.loop_point;

		mused.song.song_length = mused.current_sequencepos + len;
		mused.song.loop_point = mused.current_sequencepos;

		debug("%d Looping %d-%d", p, mused.current_sequencepos, mused.current_sequencepos + len);

		play(MAKEPTR(1), 0, 0);
	}
}

void play_one_row(void *unused1, void *unused2, void *unused3)
{
	if (!(mused.flags & LOOP_POSITION))
	{
		mused.flags |= LOOP_POSITION;
		mused.loop_store_length = mused.song.song_length;
		mused.loop_store_loop = mused.song.loop_point;

		mused.song.song_length = mused.current_patternpos + 1;
		mused.song.loop_point = mused.current_patternpos;

		mused.flags2 |= ((mused.song.flags & MUS_NO_REPEAT) ? NO_REPEAT_COPY : 0);
		mused.song.flags |= MUS_NO_REPEAT;

		mused.mus.flags |= MUS_ENGINE_PLAY_ONE_STEP;

		debug("Playing one row");

		play(MAKEPTR(1), 0, 0);
	}
}


void stop(void *unused1, void *unused2, void *unused3)
{
	mus_set_song(&mused.mus, NULL, 0);

	if (mused.flags & LOOP_POSITION)
	{
		mused.song.song_length = mused.loop_store_length;
		mused.song.loop_point = mused.loop_store_loop;
	}

	if(mused.flags2 & NO_REPEAT_COPY)
	{
		mused.song.flags |= MUS_NO_REPEAT;
		mused.flags2 &= ~NO_REPEAT_COPY;
	}

	else
	{
		mused.song.flags &= ~MUS_NO_REPEAT;
	}

	mused.flags &= ~(SONG_PLAYING | LOOP_POSITION);
	mused.mus.flags &= ~MUS_ENGINE_PLAY_ONE_STEP;
}


void change_song_speed(void *speed, void *delta, void *unused)
{
	if (!speed)
	{
		if ((int)mused.song.song_speed + CASTPTR(int,delta) >= 1 && (int)mused.song.song_speed + CASTPTR(int,delta) <= 255)
			mused.song.song_speed += CASTPTR(int,delta);
	}
	else
	{
		if ((int)mused.song.song_speed2 + CASTPTR(int,delta) >= 1 && (int)mused.song.song_speed2 + CASTPTR(int,delta) <= 255)
		mused.song.song_speed2 += CASTPTR(int,delta);
	}
}


void select_instrument_param(void *idx, void *unused1, void *unused2)
{
	mused.selected_param = CASTPTR(int,idx);
}


void select_program_step(void *idx, void *digit, void *unused2)
{
	mused.current_program_step = CASTPTR(int, idx);
	mused.editpos = CASTPTR(int, digit);
}


void new_song_action(void *unused1, void *unused2, void *unused3)
{
	if (confirm(domain, mused.slider_bevel, &mused.largefont, "Clear song and data?"))
	{
		stop(0, 0, 0);
		new_song();
	}
}


void kill_instrument(void *unused1, void *unused2, void *unused3)
{
	cyd_lock(&mused.cyd, 1);
	mus_get_default_instrument(&mused.song.instrument[mused.current_instrument]);
	cyd_lock(&mused.cyd, 0);
}


void generic_action(void *func, void *unused1, void *unused2)
{
	mus_set_song(&mused.mus, NULL, 0);
	cyd_lock(&mused.cyd, 1);

	((void *(*)(void))func)(); /* I love the smell of C in the morning */

	cyd_lock(&mused.cyd, 0);
}


void quit_action(void *unused1, void *unused2, void *unused3)
{
	if (confirm(domain, mused.slider_bevel, &mused.largefont, "Do you really want to exit?"))
	{
		mused.done = 1;
	}
}

void change_mode_action(void *mode, void *from_shortcut, void *unused2)
{
	if(CASTPTR(int, from_shortcut) == 1)
	{
		change_mode(CASTPTR(int, mode));
	}
	
	else
	{
		change_mode(CASTPTR(int, mode));
	}
}


void enable_channel(void *channel, void *unused1, void *unused2)
{
	debug("Toggle chn %d", CASTPTR(int,channel));
	mused.mus.channel[CASTPTR(int,channel)].flags ^= MUS_CHN_DISABLED;

	set_info_message("%s channel %d", (mused.mus.channel[CASTPTR(int, channel)].flags & MUS_CHN_DISABLED) ? "Muted" : "Unmuted", CASTPTR(int,channel));
}


void expand_command(int channel, void *unused2, void *unused3) //wasn't there
{
	if(mused.command_columns[channel] <= MUS_MAX_COMMANDS - 2)
	{
		mused.command_columns[channel]++;
	}
}

void hide_command(int channel, void *unused2, void *unused3) //wasn't there
{
	if(mused.command_columns[channel] > 0)
	{
		mused.command_columns[channel]--;
	}
}

void solo_channel(void *_channel, void *unused1, void *unused2)
{
	int c = 0;
	int channel = CASTPTR(int, _channel); //was int channel = CASTPTR(int,channel);
	//dirty hack
	
	debug("Current chn %d", channel); //wasn't there

	if (channel == -1)
		channel = mused.current_sequencetrack;

	for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
	{
		if (!(mused.mus.channel[i].flags & MUS_CHN_DISABLED))
		{
			++c;
		}
	}

	if (c == 1 && !(mused.mus.channel[channel].flags & MUS_CHN_DISABLED))
	{
		debug("Unmuted all");
		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
		{
			mused.mus.channel[i].flags &= ~MUS_CHN_DISABLED;
		}

		set_info_message("Unmuted all channels");
	}
	
	else
	{
		debug("Solo chn %d", CASTPTR(int,channel));
		
		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
		{
			mused.mus.channel[i].flags |= MUS_CHN_DISABLED;
		}

		mused.mus.channel[channel].flags &= ~MUS_CHN_DISABLED;

		set_info_message("Channel %d solo", channel);
	}
	
	update_all_volumes(&mused.mus);
}


void unmute_all_action(void *unused1, void *unused2, void *unused3)
{
	for(int i = 0; i < MUS_MAX_CHANNELS; ++i)
		mused.mus.channel[i].flags &= ~MUS_CHN_DISABLED;
}


void select_all(void *unused1, void *unused2, void *unused3)
{
	switch (mused.focus)
	{
		case EDITPATTERN:
			//mused.selection.start = mused.song.pattern[current_pattern()].position; //mused.selection.start = 0;
			//mused.selection.end = mused.song.pattern[current_pattern()].position + mused.song.pattern[current_pattern()].num_steps; //mused.selection.end = mused.song.pattern[current_pattern(NULL)].num_steps;
			for(int i = 0; i < NUM_SEQUENCES - 1; ++i)
			{
				const MusSeqPattern *sp = &mused.song.sequence[mused.current_sequencetrack][i];
				//debug("start");
				
				/*if((sp + 1)->position + mused.song.pattern[(sp + 1)->pattern].num_steps - 1 >= mused.current_patternpos && (sp + 1)->position <= mused.current_patternpos) //for one next pattern which may overlap
				{
					//debug("start %d, end %d, seqpos %d, patpos %d", sp->position, sp->position + mused.song.pattern[sp->pattern].num_steps, mused.current_sequencepos, mused.current_patternpos);
					mused.selection.start = (sp + 1)->position;
					
					mused.selection.end = (sp + 1)->position + mused.song.pattern[(sp + 1)->pattern].num_steps;
					
					mused.selection.patternx_start = 0;
					mused.selection.patternx_end = 1 + 2 + 2 + 4 + ((mused.command_columns[mused.current_sequencetrack] + 1) * 4) - 1;
					
					break;
				}
				
				else*/ if(sp->position + mused.song.pattern[sp->pattern].num_steps - 1 >= mused.current_patternpos && sp->position <= mused.current_patternpos)
				{
					mused.selection.start = sp->position;

					//debug("start %d, end %d, seqpos %d, patpos %d", sp->position, sp->position + mused.song.pattern[sp->pattern].num_steps, mused.current_sequencepos, mused.current_patternpos);
					
					mused.selection.end = my_min(sp->position + mused.song.pattern[sp->pattern].num_steps, (sp + 1)->position);
					//mused.selection.end = sp->position + mused.song.pattern[sp->pattern].num_steps;
					
					mused.selection.patternx_start = 0;
					mused.selection.patternx_end = 1 + 2 + 2 + 4 + ((mused.command_columns[mused.current_sequencetrack] + 1) * 4) - 1;
					
					break;
				}
			}
			break;

		case EDITPROG:
		case EDITPROG4OP:
			mused.selection.start = 0;
			mused.selection.end = MUS_PROG_LEN;
			break;

		case EDITSEQUENCE:
			mused.selection.start = 0;
			mused.selection.end = mused.song.song_length;
			break;
	}
}


void clear_selection(void *unused1, void *unused2, void *unused3)
{
	mused.selection.start = -1;
	mused.selection.end = -1;
}


void cycle_focus(void *_views, void *_focus, void *_mode)
{
	View **viewlist = _views;
	int *focus = _focus, *mode = _mode;
	View *views = viewlist[*mode];

	int i;
	for (i = 0; views[i].handler; ++i)
	{
		if (views[i].focus == *focus) break;
	}

	if (!views[i].handler) return;

	int next;

	for (next = i + 1; i != next; ++next)
	{
		if (views[next].handler == NULL)
		{
			next = -1;
			continue;
		}

		if (views[next].focus != -1 && views[next].focus != *focus)
		{
			*focus = views[next].focus;
			break;
		}
	}
}


void change_song_length(void *delta, void *unused1, void *unused2)
{
	int l = mused.song.song_length;
	l += CASTPTR(int, delta);
	l = l - (l % mused.sequenceview_steps);

	mused.song.song_length = my_max(0, my_min(0xfffe, l));
}


void change_loop_point(void *delta, void *unused1, void *unused2)
{
	if (CASTPTR(int,delta) < 0)
	{
		if (mused.song.loop_point >= -CASTPTR(int,delta))
			mused.song.loop_point += CASTPTR(int,delta);
		else
			mused.song.loop_point = 0;
	}
	else if (CASTPTR(int,delta) > 0)
	{
		mused.song.loop_point += CASTPTR(int,delta);
		if (mused.song.loop_point >= mused.song.song_length)
			mused.song.loop_point = mused.song.song_length;
	}
}


void change_seq_steps(void *delta, void *unused1, void *unused2)
{
	if (CASTPTR(int,delta) < 0)
	{
		if (mused.sequenceview_steps > -CASTPTR(int,delta))
		{
			mused.sequenceview_steps += CASTPTR(int,delta);
		}
		else
			mused.sequenceview_steps = 1;
	}
	else if (CASTPTR(int,delta) > 0)
	{
		if (mused.sequenceview_steps == 1 && CASTPTR(int,delta) > 1)
			mused.sequenceview_steps = 0;

		if (mused.sequenceview_steps < 256)
		{
			mused.sequenceview_steps += CASTPTR(int,delta);
		}
		else
			mused.sequenceview_steps = 256;
	}

	mused.current_sequencepos = (mused.current_sequencepos/mused.sequenceview_steps) * mused.sequenceview_steps;

	if (mused.flags & LOCK_SEQUENCE_STEP_AND_PATTERN_LENGTH)
		change_default_pattern_length(MAKEPTR(mused.sequenceview_steps), NULL, NULL);
}


void show_about_box(void *unused1, void *unused2, void *unused3)
{
	msgbox(domain, mused.slider_bevel, &mused.largefont, VERSION_STRING "\n" KLYSTRON_VERSION_STRING, MB_OK);
}


void change_channels(void *delta, void *unused1, void *unused2)
{
	if (CASTPTR(int, delta) < 0 && mused.song.num_channels > abs(CASTPTR(int, delta)))
	{
		set_channels(mused.song.num_channels + CASTPTR(int, delta));
		mused.cyd.n_channels = mused.song.num_channels;
	}
	
	else if (CASTPTR(int, delta) > 0 && mused.song.num_channels + CASTPTR(int, delta) <= MUS_MAX_CHANNELS)
	{
		set_channels(mused.song.num_channels + CASTPTR(int, delta));
		mused.cyd.n_channels = mused.song.num_channels;
	}
}


void change_master_volume(void *delta, void *unused1, void *unused2)
{
	if (CASTPTR(int, delta) < 0 && mused.song.master_volume + CASTPTR(int, delta) >= 0)
	{
		mused.song.master_volume += CASTPTR(int, delta);
		mused.mus.volume = mused.song.master_volume;
	}
	
	else if (CASTPTR(int, delta) > 0 && mused.song.master_volume + CASTPTR(int, delta) <= MAX_VOLUME)
	{
		mused.song.master_volume += CASTPTR(int, delta);
		mused.mus.volume = mused.song.master_volume;
	}
}


void begin_selection_action(void *unused1, void *unused2, void *unused3)
{
	switch (mused.focus)
	{
		case EDITPATTERN:
		begin_selection(current_patternstep());
		break;

		case EDITSEQUENCE:
		begin_selection(mused.current_sequencepos);
		break;

		case EDITPROG:
		case EDITPROG4OP:
		begin_selection(mused.current_program_step);
		break;
	}
}


void end_selection_action(void *unused1, void *unused2, void *unused3)
{
	switch (mused.focus)
	{
		case EDITPATTERN:
		select_range(current_patternstep());
		break;

		case EDITSEQUENCE:
		select_range(mused.current_sequencepos);
		break;

		case EDITPROG:
		case EDITPROG4OP:
		select_range(mused.current_program_step);
		break;
	}
}


void toggle_pixel_scale(void *a, void*b, void*c)
{
	change_pixel_scale(MAKEPTR(((domain->scale) & 3) + 1), 0, 0);
}


void change_pixel_scale(void *scale, void*b, void*c)
{
	mused.pixel_scale = CASTPTR(int,scale);
	domain->screen_w = my_max(320, mused.window_w / mused.pixel_scale);
	domain->screen_h = my_max(240, mused.window_h / mused.pixel_scale);
	domain->scale = mused.pixel_scale;
	gfx_domain_update(domain, true);

	set_scaled_cursor();

	for (int i = 0; i < 4; ++i)
	{
		if (pixelmenu[i].p1 == scale)
			pixelmenu[i].flags |= MENU_BULLET;
		else
			pixelmenu[i].flags &= ~MENU_BULLET;
	}
}


void toggle_fullscreen(void *a, void*b, void*c)
{
	SDL_Event e;
	e.button.button = SDL_BUTTON_LEFT;
	mouse_released(&e);
	mused.flags ^= FULLSCREEN;
	change_fullscreen(0,0,0);
}


void change_fullscreen(void *a, void*b, void*c)
{
	domain->fullscreen = (mused.flags & FULLSCREEN) != 0;

	if (!(mused.flags & FULLSCREEN))
	{
		domain->screen_w = mused.window_w / domain->scale;
		domain->screen_h = mused.window_h / domain->scale;
	}

	gfx_domain_update(domain, true);
}


void toggle_render_to_texture(void *a, void*b, void*c)
{
	SDL_Event e;
	e.button.button = SDL_BUTTON_LEFT;
	mouse_released(&e);
	mused.flags ^= DISABLE_RENDER_TO_TEXTURE;
	change_render_to_texture(0,0,0);
}


void toggle_mouse_cursor(void *a, void*b, void*c)
{
	SDL_Event e;
	e.button.button = SDL_BUTTON_LEFT;
	mouse_released(&e);
	mused.flags ^= USE_SYSTEM_CURSOR;
	set_scaled_cursor();
}


void change_render_to_texture(void *a, void*b, void*c)
{
	domain->flags = (domain->flags & ~GFX_DOMAIN_DISABLE_RENDER_TO_TEXTURE) | ((mused.flags & DISABLE_RENDER_TO_TEXTURE) ? GFX_DOMAIN_DISABLE_RENDER_TO_TEXTURE : 0);
	gfx_domain_update(domain, true);
}


void load_theme_action(void *name, void*b, void*c)
{
	load_theme((char*)name);
}


void load_keymap_action(void *name, void*b, void*c)
{
	load_keymap((char*)name);
}


void change_timesig(void *delta, void *part, void *c)
{
	// http://en.wikipedia.org/wiki/Time_signature says the following signatures are common.
	// I'm a 4/4 or 3/4 man myself so I'll trust the article :)
	
	int parti = CASTPTR(int, part);
	int deltai = CASTPTR(int, delta);

	/*static const Uint16 sigs[] = { 0x0404, 0x0304, 0x0604, 0x0308, 0x0608, 0x0908, 0x0c08 };
	int i;
	for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); ++i)
	{
		if (sigs[i] == mused.time_signature)
			break;
	}

	i += CASTPTR(int, delta);

	if (i >= (int)(sizeof(sigs) / sizeof(sigs[0]))) i = 0;
	if (i < 0) i = sizeof(sigs) / sizeof(sigs[0]) - 1;
	mused.time_signature = sigs[i];*/
	if(parti == 1)
	{
		Uint8 temp = (mused.time_signature & 0xff00) >> 8;
		
		if(temp + deltai > 0 && temp + deltai <= 20)
		{
			mused.time_signature &= 0x00ff;
			mused.time_signature |= (Uint8)(temp + deltai) << 8;
		}
	}
	
	if(parti == 2)
	{
		Uint8 temp = mused.time_signature & 0xff;
		
		if(temp + deltai > 0 && temp + deltai <= 20)
		{
			mused.time_signature &= 0xff00;
			mused.time_signature |= (Uint8)(temp + deltai);
		}
	}
}


void export_wav_action(void *a, void *b, void *c)
{
	char def[1000];
	
	mused.song.flags |= MUS_NO_REPEAT; //wasn't there
	mused.mus.flags &= ~MUS_ENGINE_PLAY_ONE_STEP;

	if (strlen(mused.previous_song_filename) == 0)
	{
		snprintf(def, sizeof(def), "%s.wav", mused.song.title);
	}
	
	else
	{
		strncpy(def, mused.previous_export_filename, sizeof(mused.previous_export_filename) - 1);
	}

	char filename[5000];

	if (open_dialog_fn("wb", "Export .WAV", "wav", domain, mused.slider_bevel, &mused.largefont, &mused.smallfont, def, filename, sizeof(filename)))
	{
		strncpy(mused.previous_export_filename, filename, sizeof(mused.previous_export_filename) - 1);

		FILE *f = fopen(filename, "wb");

		if (f)
		{
			export_wav(&mused.song, mused.mus.cyd->wavetable_entries, f, -1);
			// f is closed inside of export_wav (inside of ww_finish)
		}
		
		mused.song.flags &= ~MUS_NO_REPEAT; //wasn't there
	}
	
	mused.song.flags &= ~MUS_NO_REPEAT; //wasn't there
}

void export_fzt_action(void *a, void *b, void *c)
{
	char def[1000];

	if (strlen(mused.previous_song_filename) == 0)
	{
		snprintf(def, sizeof(def), "%s.fzt", mused.song.title);
	}
	
	else
	{
		strncpy(def, mused.previous_export_filename, sizeof(mused.previous_export_filename) - 1);
	}

	char filename[5000];

	if (open_dialog_fn("wb", "Export .FZT (Flizzer Tracker module)", "fzt", domain, mused.slider_bevel, &mused.largefont, &mused.smallfont, def, filename, sizeof(filename)))
	{
		strncpy(mused.previous_export_filename, filename, sizeof(mused.previous_export_filename) - 1);

		FILE *f = fopen(filename, "wb");

		if (f)
		{
			export_fzt(&mused.song, mused.mus.cyd->wavetable_entries, f);
		}
	}
}

void export_hires_wav_action(void *a, void*b, void*c)
{
	char def[1000];
	
	mused.song.flags |= MUS_NO_REPEAT; //wasn't there
	mused.mus.flags &= ~MUS_ENGINE_PLAY_ONE_STEP;

	if (strlen(mused.previous_song_filename) == 0)
	{
		snprintf(def, sizeof(def), "%s.wav", mused.song.title);
	}
	
	else
	{
		strncpy(def, mused.previous_export_filename, sizeof(mused.previous_export_filename) - 1);
	}

	char filename[5000];

	if (open_dialog_fn("wb", "Export hires .WAV", "wav", domain, mused.slider_bevel, &mused.largefont, &mused.smallfont, def, filename, sizeof(filename)))
	{
		strncpy(mused.previous_export_filename, filename, sizeof(mused.previous_export_filename) - 1);

		FILE *f = fopen(filename, "wb");

		if (f)
		{
			export_wav_hires(&mused.song, mused.mus.cyd->wavetable_entries, f, -1);
			// f is closed inside of export_wav (inside of ww_finish)
		}
		
		mused.song.flags &= ~MUS_NO_REPEAT; //wasn't there
	}
	
	mused.song.flags &= ~MUS_NO_REPEAT; //wasn't there
}


/*typedef struct 
{
	int channel;
	MusSong *song;
	const char filename[5000];
} THREAD_PAYLOAD;

static int TestThread(void *ptr)
{
    THREAD_PAYLOAD *payload = ptr;
    //Cyd *cyd = cyd_init(44100, whatever);
    //export_channel(cyd, payload->song, payload->channel, payload->filename);
    //cyd_deinit(cyd)
	//export_wav(&mused.song, mused.mus.cyd->wavetable_entries, payload, channel);

	//snprintf(c_filename, sizeof(c_filename) - 1, "%s-%02d.wav", tmp, payload->channel);

	debug("Exporting channel %d to \"%s\"", payload->channel, payload->filename);

	FILE *f = fopen(payload->filename, "wb");

	if (f)
	{
		debug("Calling export wav");
		export_wav(payload->song, mused.mus.cyd->wavetable_entries, f, payload->channel);
		// f is closed inside of export_wav (inside of ww_finish)
	}
	
    return 0;
}

void export_channels_action(void *a, void*b, void*c)
{
	SDL_Thread *thread[8]; //wasn't there
    THREAD_PAYLOAD payload[8] = {0}; //wasn't there
	
	mused.song.flags |= MUS_NO_REPEAT; //wasn't there
	
	char def[1000];

	if (strlen(mused.previous_song_filename) == 0)
	{
		snprintf(def, sizeof(def), "%s.wav", mused.song.title);
	}
	else
	{
		strncpy(def, mused.previous_export_filename, sizeof(mused.previous_export_filename) - 1);
	}

	char filename[5000];

	if (open_dialog_fn("wb", "Export .WAV", "wav", domain, mused.slider_bevel, &mused.largefont, &mused.smallfont, def, filename, sizeof(filename)))
	{
		strncpy(mused.previous_export_filename, filename, sizeof(mused.previous_export_filename) - 1);

		for (int i = 0; i < mused.song.num_channels; ++i)
		{
			char c_filename[1500], tmp[1000];
			strncpy(tmp, filename, sizeof(tmp) - 1);

			for (int c = strlen(tmp) - 1; c >= 0; --c)
			{
				if (tmp[c] == '.')
				{
					tmp[c] = '\0';
					break;
				}
			}

			snprintf(payload[i].filename, sizeof(c_filename) - 1, "%s-%02d.wav", tmp, i);

			//debug("Exporting ch. %d to %s", i, c_filename);

			//snprintf(payload[i].filename, 50, "channel_%d.wav", i);
			payload[i].channel = i;
			payload[i].song = &mused.song;
			thread[i] = SDL_CreateThread(TestThread, "TestThread", &payload[i]);
			
			//debug("Exporting ch. %d to %s", i, c_filename);
		}
		
		for (int i = 0; i < mused.song.num_channels; ++i) 
		{
			int val;
			SDL_WaitThread(thread[i], &val);
			debug("Thread %d returned %d", i, val); 
		}
		
		mused.song.flags &= ~MUS_NO_REPEAT; //wasn't there
	}
}*/


void export_channels_action(void *a, void*b, void*c)
{
	char def[1000];
	
	mused.song.flags |= MUS_NO_REPEAT; //wasn't there

	if (strlen(mused.previous_song_filename) == 0)
	{
		snprintf(def, sizeof(def), "%s.wav", mused.song.title);
	}
	else
	{
		strncpy(def, mused.previous_export_filename, sizeof(mused.previous_export_filename) - 1);
	}

	char filename[5000];

	if (open_dialog_fn("wb", "Export .WAV", "wav", domain, mused.slider_bevel, &mused.largefont, &mused.smallfont, def, filename, sizeof(filename)))
	{
		strncpy(mused.previous_export_filename, filename, sizeof(mused.previous_export_filename) - 1);

		for (int i = 0; i < mused.song.num_channels; ++i)
		{
			char c_filename[1500], tmp[1000];
			strncpy(tmp, filename, sizeof(tmp) - 1);

			for (int c = strlen(tmp) - 1; c >= 0; --c)
			{
				if (tmp[c] == '.')
				{
					tmp[c] = '\0';
					break;
				}
			}

			snprintf(c_filename, sizeof(c_filename) - 1, "%s-%02d.wav", tmp, i);

			debug("Exporting channel %d to %s", i, c_filename);

			FILE *f = fopen(c_filename, "wb");

			if (f)
			{
				mused.song.flags |= MUS_NO_REPEAT;
				
				export_wav(&mused.song, mused.mus.cyd->wavetable_entries, f, i);
				// f is closed inside of export_wav (inside of ww_finish)
			}
		}
		
		mused.song.flags &= ~MUS_NO_REPEAT; //wasn't there
	}
	
	mused.song.flags &= ~MUS_NO_REPEAT; //wasn't there
}


void do_undo(void *a, void *b, void *c)
{
	UndoFrame *frame = a ? undo(&mused.redo) : undo(&mused.undo);

	debug("%s frame %p", a ? "Redo" : "Undo", frame);

	if (!frame) return;

	if (!a)
	{
		UndoStack tmp = mused.redo;
		mused.redo = mused.undo;
		mused.undo = tmp;
	}

	switch (frame->type)
	{
		case UNDO_PATTERN:
			undo_store_pattern(&mused.undo, frame->event.pattern.idx, &mused.song.pattern[frame->event.pattern.idx], mused.modified);

			resize_pattern(&mused.song.pattern[frame->event.pattern.idx], frame->event.pattern.n_steps);
			memcpy(mused.song.pattern[frame->event.pattern.idx].step, frame->event.pattern.step, frame->event.pattern.n_steps * sizeof(frame->event.pattern.step[0]));
			break;

		case UNDO_SEQUENCE:
			mused.current_sequencetrack = frame->event.sequence.channel;

			undo_store_sequence(&mused.undo, mused.current_sequencetrack, mused.song.sequence[mused.current_sequencetrack], mused.song.num_sequences[mused.current_sequencetrack], mused.modified);

			mused.song.num_sequences[mused.current_sequencetrack] = frame->event.sequence.n_seq;

			memcpy(mused.song.sequence[mused.current_sequencetrack], frame->event.sequence.seq, frame->event.sequence.n_seq * sizeof(frame->event.sequence.seq[0]));
			break;

		case UNDO_MODE:
			change_mode(frame->event.mode.old_mode);
			mused.focus = frame->event.mode.focus;
			mused.show_four_op_menu = frame->event.mode.show_four_op_menu; //wasn't there
			
			break;

		case UNDO_INSTRUMENT:
			mused.current_instrument = frame->event.instrument.idx;

			undo_store_instrument(&mused.undo, mused.current_instrument, &mused.song.instrument[mused.current_instrument], mused.modified);
			
			MusInstrument* inst = &mused.song.instrument[mused.current_instrument];
			
			mus_free_inst_programs(inst);

			memcpy(&mused.song.instrument[mused.current_instrument], frame->event.instrument.instrument, sizeof(*(frame->event.instrument.instrument)));
			
			mused.current_instrument_program = frame->event.instrument.current_instrument_program;
			
			for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
			{
				mused.current_fourop_program[i] = frame->event.instrument.current_fourop_program[i];
			}
			
			mused.selected_operator = frame->event.instrument.selected_operator;
			
			mused.program_position = frame->event.instrument.program_position;
			
			for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
			{
				mused.fourop_program_position[i] = frame->event.instrument.fourop_program_position[i];
			}

			break;

		case UNDO_FX:
			mused.fx_bus = frame->event.fx.idx;

			undo_store_fx(&mused.undo, mused.fx_bus, &mused.song.fx[mused.fx_bus], mused.song.multiplex_period, mused.modified);

			memcpy(&mused.song.fx[mused.fx_bus], &frame->event.fx.fx, sizeof(frame->event.fx.fx));
			mused.song.multiplex_period = frame->event.fx.multiplex_period;
			mus_set_fx(&mused.mus, &mused.song);
			break;

		case UNDO_SONGINFO:
		{
			undo_store_songinfo(&mused.undo, &mused.song, mused.modified);

			MusSong *song = &mused.song;
			song->song_length = frame->event.songinfo.song_length;
			mused.sequenceview_steps = song->sequence_step = frame->event.songinfo.sequence_step;
			song->loop_point = frame->event.songinfo.loop_point;
			song->song_speed = frame->event.songinfo.song_speed;
			song->song_speed2 = frame->event.songinfo.song_speed2;
			song->song_rate = frame->event.songinfo.song_rate;
			song->time_signature = frame->event.songinfo.time_signature;
			song->flags = frame->event.songinfo.flags;
			song->num_channels = frame->event.songinfo.num_channels;
			strcpy(song->title, frame->event.songinfo.title);
			song->master_volume = frame->event.songinfo.master_volume;
			memcpy(song->default_volume, frame->event.songinfo.default_volume, sizeof(frame->event.songinfo.default_volume));
			memcpy(song->default_panning, frame->event.songinfo.default_panning, sizeof(frame->event.songinfo.default_panning));
		}
		break;

		case UNDO_WAVE_PARAM:
		{
			mused.selected_wavetable = frame->event.wave_param.idx;

			undo_store_wave_param(&mused.undo, mused.selected_wavetable, &mused.mus.cyd->wavetable_entries[mused.selected_wavetable], mused.modified);

			CydWavetableEntry *entry = &mused.mus.cyd->wavetable_entries[mused.selected_wavetable];
			entry->flags = frame->event.wave_param.flags;
			entry->sample_rate = frame->event.wave_param.sample_rate;
			entry->samples = frame->event.wave_param.samples;
			entry->loop_begin = frame->event.wave_param.loop_begin;
			entry->loop_end = frame->event.wave_param.loop_end;
			entry->base_note = frame->event.wave_param.base_note;
		}
		break;

		case UNDO_WAVE_DATA:
		{
			mused.selected_wavetable = frame->event.wave_param.idx;

			undo_store_wave_data(&mused.undo, mused.selected_wavetable, &mused.mus.cyd->wavetable_entries[mused.selected_wavetable], mused.modified);

			CydWavetableEntry *entry = &mused.mus.cyd->wavetable_entries[mused.selected_wavetable];
			entry->data = realloc(entry->data, frame->event.wave_data.length * sizeof(entry->data[0]));
			memcpy(entry->data, frame->event.wave_data.data, frame->event.wave_data.length * sizeof(entry->data[0]));
			entry->samples = frame->event.wave_data.length;
			entry->sample_rate = frame->event.wave_data.sample_rate;
			entry->samples = frame->event.wave_data.samples;
			entry->loop_begin = frame->event.wave_data.loop_begin;
			entry->loop_end = frame->event.wave_data.loop_end;
			entry->flags = frame->event.wave_data.flags;
			entry->base_note = frame->event.wave_data.base_note;

			invalidate_wavetable_view();
		}
		break;

		case UNDO_WAVE_NAME:
		{
			mused.selected_wavetable = frame->event.wave_name.idx;

			undo_store_wave_name(&mused.undo, mused.selected_wavetable, mused.song.wavetable_names[mused.selected_wavetable], mused.modified);

			strcpy(mused.song.wavetable_names[mused.selected_wavetable], frame->event.wave_name.name);

			invalidate_wavetable_view();
		}
		break;

		default: warning("Undo type %d not handled", frame->type); break;
	}

	mused.modified = frame->modified;

	if (!a)
	{
		UndoStack tmp = mused.redo;
		mused.redo = mused.undo;
		mused.undo = tmp;
	}


	mused.last_snapshot_a = -1;

/*#ifdef DEBUG
	undo_show_stack(&mused.undo);
	undo_show_stack(&mused.redo);
#endif*/

	undo_destroy_frame(frame);
}


void kill_wavetable_entry(void *a, void*b, void*c)
{
	snapshot(S_T_WAVE_DATA);
	cyd_wave_entry_init(&mused.mus.cyd->wavetable_entries[mused.selected_wavetable], NULL, 0, 0, 0, 0, 0);
}


void open_menu_action(void*a,void*b,void*c)
{
	my_open_menu(mainmenu, NULL);
}


void flip_bit_action(void *a, void *b, void *c)
{
	*(Uint32*)a ^= CASTPTR(Uint32,b);
}


void set_note_jump(void *steps, void *unused1, void *unused2)
{
	mused.note_jump = CASTPTR(int, steps);

	set_info_message("Note jump set to %d", mused.note_jump);
}


void change_default_pattern_length(void *length, void *unused1, void *unused2)
{
	for (int i = 0; i < NUM_PATTERNS; ++i)
	{
		if (mused.song.pattern[i].num_steps == mused.default_pattern_length && is_pattern_empty(&mused.song.pattern[i]))
		{
			resize_pattern(&mused.song.pattern[i], CASTPTR(int, length));
		}
	}

	mused.sequenceview_steps = mused.default_pattern_length = CASTPTR(int, length);

	for (Menu *m = patternlengthmenu; m->text; ++m)
	{
		if (CASTPTR(int, m->p1) == mused.default_pattern_length)
			m->flags |= MENU_BULLET;
		else
			m->flags &= ~MENU_BULLET;
	}
}


void toggle_visualizer(void *unused1, void *unused2, void *unused3)
{
	++mused.current_visualizer;

	if (mused.current_visualizer >= VIS_NUM_TOTAL)
		mused.current_visualizer = 0;
}


void change_visualizer_action(void *vis, void *unused1, void *unused2)
{
	change_visualizer(CASTPTR(int,vis));
}


void open_help(void *unused0, void *unused1, void *unused2)
{
	cyd_lock(&mused.cyd, 0);
	open_help_no_lock(NULL, NULL, NULL);
	cyd_lock(&mused.cyd, 1);
}


void open_help_no_lock(void *unused0, void *unused1, void *unused2)
{
	helpbox("Help", domain, mused.slider_bevel, &mused.largefont, &mused.smallfont);
}


void open_songmessage(void *unused0, void *unused1, void *unused2)
{
	song_message_view(domain, mused.slider_bevel, &mused.largefont, &mused.smallfont);
}


void toggle_follow_play_position(void *unused1, void *unused2, void *unused3)
{
	mused.flags ^= FOLLOW_PLAY_POSITION;

	if (mused.flags & FOLLOW_PLAY_POSITION)
		set_info_message("Following song position");
	else
		set_info_message("Not following song position");
}


void open_recent_file(void *path, void *b, void *c)
{
	debug("Opening recent file %s", (char*)path);
	FILE *f = fopen(path, "rb");

	if (f)
	{
		open_song(f);
		fclose(f);

		// Need to copy this var because update_recent_files_list()
		// might free the string in *path...

		char *temp = strdup(path);
		update_recent_files_list(temp);
		free(temp);
	}
	else
	{
		warning("Could not open %s", (char*)path);
	}
}
