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

#include "wavetableview.h"
#include "../mused.h"
#include "../view.h"
#include "../event.h"
#include "gui/mouse.h"
#include "gui/dialog.h"
#include "gui/bevel.h"
#include "theme.h"
#include "mybevdefs.h"
#include "action.h"
#include "wave_action.h"

extern Mused mused;

void wavetable_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect r, frame;
	copy_rect(&frame, dest);
	bevelex(domain, &frame, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	adjust_rect(&frame, 4);
	copy_rect(&r, &frame);
	
	CydWavetableEntry *w = &mused.mus.cyd->wavetable_entries[mused.selected_wavetable];
	
	{
		r.w = 64;
		r.h = 10;
		
		int d;
				
		r.w = 128;
		
		if ((d = generic_field(event, &r, EDITWAVETABLE, W_RATE, "RATE", "%6d Hz", MAKEPTR(w->sample_rate), 9)) != 0)
		{
			wave_add_param(d);
		}
		
		update_rect(&frame, &r);
		
		r.w = 72;
		r.h = 10;
		
		if ((d = generic_field(event, &r, EDITWAVETABLE, W_BASE, "BASE", "%s", notename((w->base_note + 0x80) >> 8), 3)) != 0)
		{
			wave_add_param(d);
		}
		
		update_rect(&frame, &r);
		r.w = 48;
		
		if ((d = generic_field(event, &r, EDITWAVETABLE, W_BASEFINE, "", "%+4d", MAKEPTR((Sint8)w->base_note), 4)) != 0)
		{
			wave_add_param(d);
		}
		
		r.w = 128;
		
		update_rect(&frame, &r);
		
		generic_flags(event, &r, EDITWAVETABLE, W_INTERPOLATE, "NO INTERPOLATION", &w->flags, CYD_WAVE_NO_INTERPOLATION);
		
		update_rect(&frame, &r);
		
		r.w = 138;
		
		static const char *interpolations[] = { "NO INT", "LINEAR", "COSINE", "CUBIC", "GAUSS", "SINC" };
		
		if ((d = generic_field(event, &r, EDITWAVETABLE, W_INTERPOLATION_TYPE, "INT. TYPE", "%s", MAKEPTR(interpolations[w->flags & CYD_WAVE_NO_INTERPOLATION ? 0 : ((w->flags & (CYD_WAVE_INTERPOLATION_BIT_1|CYD_WAVE_INTERPOLATION_BIT_2|CYD_WAVE_INTERPOLATION_BIT_3)) >> 5) + 1]), 6)) != 0)
		{
			wave_add_param(d);
		}
		
		update_rect(&frame, &r);
	}
	
	my_separator(&frame, &r);
	
	{
		r.w = 80;
		
		generic_flags(event, &r, EDITWAVETABLE, W_LOOP, "LOOP", &w->flags, CYD_WAVE_LOOP);
		
		update_rect(&frame, &r);
		
		r.w = 112;
		
		int d;
		
		if ((d = generic_field(event, &r, EDITWAVETABLE, W_LOOPBEGIN, "BEGIN", "%7d", MAKEPTR(w->loop_begin), 7)) != 0)
		{
			wave_add_param(d);
		}
		
		update_rect(&frame, &r);
		
		r.w = 80;
		
		generic_flags(event, &r, EDITWAVETABLE, W_LOOPPINGPONG, "PINGPONG", &w->flags, CYD_WAVE_PINGPONG);
		
		update_rect(&frame, &r);
		
		
		r.w = 112;
		
		if ((d = generic_field(event, &r, EDITWAVETABLE, W_LOOPEND, "END", "%7d", MAKEPTR(w->loop_end), 7)) != 0)
		{
			wave_add_param(d);
		}
		
		update_rect(&frame, &r);
	}
}


void wavetablelist_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	const int chars = area.w / mused.console->font.w - 3;
	console_clear(mused.console);
	bevelex(dest_surface, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 3);
	console_set_clip(mused.console, &area);
	gfx_domain_set_clip(dest_surface, &area);

	int y = area.y;
	
	int start = mused.wavetable_list_position;
	
	for (int i = start; i < CYD_WAVE_MAX_ENTRIES && y < area.h + area.y; ++i, y += mused.console->font.h)
	{
		SDL_Rect row = { area.x - 1, y - 1, area.w + 2, mused.console->font.h + 1};
		if (i == mused.selected_wavetable)
		{
			bevel(dest_surface, &row, mused.slider_bevel, BEV_SELECTED_PATTERN_ROW);
			console_set_color(mused.console, colors[COLOR_INSTRUMENT_SELECTED]);
		}
		else
		{
			console_set_color(mused.console, colors[COLOR_INSTRUMENT_NORMAL]);
		}
			
		const CydWavetableEntry *w = &mused.mus.cyd->wavetable_entries[i];
		char temp[1000] = "";
		
		if (w->samples > 0 || mused.song.wavetable_names[i][0])
			snprintf(temp, chars, "%s (%u smp)", mused.song.wavetable_names[i][0] ? mused.song.wavetable_names[i] : "No name", w->samples);
		
		console_write_args(mused.console, "%02X %s\n", i, temp);
		
		check_event(event, &row, select_wavetable, MAKEPTR(i), 0, 0);
		
		slider_set_params(&mused.wavetable_list_slider_param, 0, CYD_WAVE_MAX_ENTRIES - 1, start, i, &mused.wavetable_list_position, 1, SLIDER_VERTICAL, mused.slider_bevel);
	}
	
	gfx_domain_set_clip(dest_surface, NULL);
	
	if (event->type == SDL_MOUSEWHEEL && mused.focus == EDITWAVETABLE)
	{
		if (event->wheel.y > 0)
		{
			mused.wavetable_list_position -= 4;
			*(mused.wavetable_list_slider_param.position) -= 4;
		}
		
		else
		{
			mused.wavetable_list_position += 4;
			*(mused.wavetable_list_slider_param.position) += 4;
		}
		
		mused.wavetable_list_position = my_max(0, my_min(CYD_WAVE_MAX_ENTRIES - area.h / 8, mused.wavetable_list_position));
		*(mused.wavetable_list_slider_param.position) = my_max(0, my_min(CYD_WAVE_MAX_ENTRIES - area.h / 8, *(mused.wavetable_list_slider_param.position)));
	}
}


static void update_sample_preview(GfxDomain *dest, const SDL_Rect* area)
{
	if (!mused.wavetable_preview || (mused.wavetable_preview->surface->w != area->w || mused.wavetable_preview->surface->h != area->h))
	{
		if (mused.wavetable_preview) gfx_free_surface(mused.wavetable_preview);
		
		mused.wavetable_preview = gfx_create_surface(dest, area->w, area->h);	
	}
	
	else if (mused.wavetable_preview_idx == mused.selected_wavetable) return;
	
	mused.wavetable_preview_idx = mused.selected_wavetable;
	
	SDL_FillRect(mused.wavetable_preview->surface, NULL, SDL_MapRGB(mused.wavetable_preview->surface->format, (colors[COLOR_WAVETABLE_BACKGROUND] >> 16) & 255, (colors[COLOR_WAVETABLE_BACKGROUND] >> 8) & 255, colors[COLOR_WAVETABLE_BACKGROUND] & 255));

	const CydWavetableEntry *w = &mused.mus.cyd->wavetable_entries[mused.selected_wavetable];
	
	mused.wavetable_bits = 0;
	
	if (w->samples > 0)
	{
		const int res = 4096;
		const int dadd = my_max(res, w->samples * res / area->w);
		const int gadd = my_max(res, (Uint32)area->w * res / w->samples);
		int c = 0, d = 0;
				
		for (int x = 0; x < area->w * res; )
		{
			int min = 32768;
			int max = -32768;
			
			for (; d < w->samples && c < dadd; c += res, ++d)
			{
				min = my_min(min, w->data[d]);
				max = my_max(max, w->data[d]);
				mused.wavetable_bits |= w->data[d];
			}
			
			c -= dadd;
			
			if (min < 0 && max < 0)
				max = 0;
				
			if (min > 0 && max > 0)
				min = 0;
			
			min = (32768 + min) * area->h / 65536;
			max = (32768 + max) * area->h / 65536 - min;
		
			int prev_x = x - 1;
			x += gadd;
			
			SDL_Rect r = { prev_x / res, min, my_max(1, x / res - prev_x / res), max + 1 };
			
			SDL_FillRect(mused.wavetable_preview->surface, &r, SDL_MapRGB(mused.wavetable_preview->surface->format, (colors[COLOR_WAVETABLE_SAMPLE] >> 16) & 255, (colors[COLOR_WAVETABLE_SAMPLE] >> 8) & 255, colors[COLOR_WAVETABLE_SAMPLE] & 255));
		}
		
		debug("Wavetable item bitmask = %x, lowest bit = %d", mused.wavetable_bits, __builtin_ffs(mused.wavetable_bits) - 1);
		set_info_message("Sample quality %d bits", 16 - (__builtin_ffs(mused.wavetable_bits) - 1));
	}
	
	gfx_update_texture(dest, mused.wavetable_preview);
}


void wavetable_sample_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 3);
	update_sample_preview(dest_surface, &area);
	my_BlitSurface(mused.wavetable_preview, NULL, dest_surface, &area);
	
	int mx, my;
	
	if (mused.mode == EDITWAVETABLE && (SDL_GetMouseState(&mx, &my) & SDL_BUTTON(1)))
	{
		mx /= mused.pixel_scale;
		my /= mused.pixel_scale;
		
		if (mused.prev_wavetable_x == -1)
		{
			mused.prev_wavetable_x = mx;
			mused.prev_wavetable_y = my;
		}
		
		int dx;
		int d = abs(mx - mused.prev_wavetable_x);
		
		if (mx < mused.prev_wavetable_x)
			dx = 1;
		else
			dx = -1;
		
		if (mx >= area.x && my >= area.y
			&& mx < area.x + area.w && my < area.y + area.h)
		{
			if (d > 0)
			{
				for (int x = mx, i = 0; i <= d; x += dx, ++i)
					wavetable_draw((float)(x - area.x) / area.w, (float)((my + (mused.prev_wavetable_y - my) * i / d) - area.y) / area.h, 1.0f / area.w);
			}
			else
				wavetable_draw((float)(mx - area.x) / area.w, (float)(my - area.y) / area.h, 1.0f / area.w);
		}
		
		mused.prev_wavetable_x = mx;
		mused.prev_wavetable_y = my;
	}
	
	else
	{
		mused.prev_wavetable_x = -1;
		mused.prev_wavetable_y = -1;
	}

	Sint32 offset = 0;
	Sint32 offset_loopbegin = 0;
	Sint32 offset_loopend = 0;

	SDL_Rect loop_begin, loop_end;

	copy_rect(&loop_begin, &area);

	loop_begin.w = 16;
	loop_begin.y = area.y;
	loop_begin.h = area.h;

	copy_rect(&loop_end, &area);

	loop_end.w = 16;
	loop_end.y = area.y;
	loop_end.h = area.h;

	if(mused.cyd.wavetable_entries[mused.selected_wavetable].flags & CYD_WAVE_LOOP) //draw below current position line
	{
		offset_loopbegin = (Uint16)((double)area.w * (double)mused.cyd.wavetable_entries[mused.selected_wavetable].loop_begin / (double)mused.cyd.wavetable_entries[mused.selected_wavetable].samples);
		loop_begin.x = area.x + offset_loopbegin;
		bevelex(domain, &loop_begin, mused.slider_bevel, BEV_ENV_LOOP_START, BEV_F_NORMAL);

		offset_loopend = (Uint16)((double)area.w * (double)mused.cyd.wavetable_entries[mused.selected_wavetable].loop_end / (double)mused.cyd.wavetable_entries[mused.selected_wavetable].samples) - 16;
		loop_end.x = area.x + offset_loopend;
		bevelex(domain, &loop_end, mused.slider_bevel, BEV_ENV_LOOP_END, BEV_F_NORMAL);
	}

	for(int i = 0; i < mused.song.num_channels; i++) //show the line indicating current position if sample is playing
	{
		if(mused.mus.channel[i].instrument != NULL)
		{
			if(mused.cyd.channel[i].flags & CYD_CHN_ENABLE_WAVE)
			{
				if(mused.cyd.channel[i].wave_entry)
				{
					if(mused.cyd.channel[i].wave_entry == &mused.cyd.wavetable_entries[mused.selected_wavetable] && mused.cyd.channel[i].wave_entry->samples > 0) //so we don't divide by 0
					{
						SDL_Rect current_position;
		
						copy_rect(&current_position, &area);

						current_position.w = 16;
						current_position.y = area.y;
						current_position.h = area.h;

						offset = (Uint16)((double)mused.cyd.channel[i].subosc[0].wave.acc * (double)area.w / (double)WAVETABLE_RESOLUTION / (double)mused.cyd.channel[i].wave_entry->samples) - 8;
						current_position.x = area.x + offset;
						bevelex(domain, &current_position, mused.slider_bevel, BEV_ENV_CURRENT_ENV_POSITION, BEV_F_NORMAL);

						goto next;
					}
				}
			}
		}
	}

	next:;
}


void invalidate_wavetable_view()
{
	mused.wavetable_preview_idx = -1;
	mused.wavetable_bits = 0;
}

void wavetable_tools_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect r, frame;
	copy_rect(&frame, dest);
	bevelex(domain, &frame, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	adjust_rect(&frame, 4);
	copy_rect(&r, &frame);
	
	r.h = 12;
	
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "DROP LOWEST BIT", wavetable_drop_lowest_bit, NULL, NULL, NULL);
	
	r.y += r.h;
	
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "HALVE RATE", wavetable_halve_samplerate, NULL, NULL, NULL);
	
	r.y += r.h;
	
	{
		int temp_x = r.x;
		int temp = r.w;
		
		r.w /= 2;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "NORMALIZE", wavetable_normalize, MAKEPTR(32768), NULL, NULL);
		
		r.x += r.w;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "DISTORT", wavetable_distort, MAKEPTR((int)(0.891 * 32768)), NULL, NULL);
		
		r.y += r.h;
		
		r.x = temp_x;
		r.w = temp;
	}
	
	{
		int temp_x = r.x;
		int temp = r.w;
		
		r.w /= 2;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "+1 dB", wavetable_amp, MAKEPTR((int)(1.122 * 32768)), NULL, NULL);
		
		r.x += r.w;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "-1 dB", wavetable_amp, MAKEPTR((int)(0.891 * 32768)), NULL, NULL);
		
		r.y += r.h;
		
		r.x = temp_x;
		r.w = temp;
	}
	
	{
		int temp_x = r.x;
		int temp = r.w;
		
		r.w /= 2;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "CUT TAIL", wavetable_cut_tail, NULL, NULL, NULL);
		
		r.x += r.w;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "CUT HEAD", wavetable_cut_head, NULL, NULL, NULL);
		
		r.y += r.h;
		
		r.x = temp_x;
		r.w = temp;
	}
	
	{
		int temp_x = r.x;
		int temp = r.w;
		
		r.w /= 2;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "KILL DC", wavetable_remove_dc, 0, NULL, NULL);
		
		r.x += r.w;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "FIND ZERO", wavetable_find_zero, NULL, NULL, NULL);
		
		r.y += r.h;
		
		r.x = temp_x;
		r.w = temp;
	}
	
	{
		int temp_x = r.x;
		int temp = r.w;
		
		r.w /= 2;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "LOPASS", wavetable_filter, MAKEPTR(0), NULL, NULL);
		
		r.x += r.w;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "HIPASS", wavetable_filter, MAKEPTR(1), NULL, NULL);
		
		r.y += r.h;
		
		r.x = temp_x;
		r.w = temp;
	}
	
	{
		int temp_x = r.x;
		int temp = r.w;
		
		r.w /= 2;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "4TH", wavetable_chord, MAKEPTR(4), NULL, NULL);
		
		r.x += r.w;
		
		button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "5TH", wavetable_chord, MAKEPTR(5), NULL, NULL);
		
		r.y += r.h;
		
		r.x = temp_x;
		r.w = temp;
	}
	
	//button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "5TH", wavetable_chord, MAKEPTR(5), NULL, NULL);
	
	//r.y += r.h;
	
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "OCTAVE", wavetable_chord, MAKEPTR(12), NULL, NULL);
	
	r.y += r.h;
	
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "WAVEGEN", flip_bit_action, &mused.flags, MAKEPTR(SHOW_WAVEGEN), NULL);
	
	if (!(mused.flags & SHOW_WAVEGEN) && mused.wavetable_param >= W_NUMOSCS && mused.wavetable_param <= W_TOOLBOX)
	{
		mused.wavetable_param = W_NUMOSCS - 1;
	}
}


void oscillator_view(GfxDomain *domain, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	WgOsc *osc = param;
	
	SDL_Rect frame;
	copy_rect(&frame, dest);
	
	int bev = BEV_THIN_FRAME;
	
	if (osc == &mused.wgset.chain[mused.selected_wg_osc])
		bev = BEV_EDIT_CURSOR;
	
	bevelex(domain, &frame, mused.slider_bevel, bev, BEV_F_STRETCH_ALL);
	adjust_rect(&frame, 2);
	
	gfx_rect(domain, &frame, colors[COLOR_WAVETABLE_BACKGROUND]);
	
	wg_init_osc(osc);
	
	float py = wg_osc(osc, 0);
	
	for (int x = 1; x < frame.w; ++x)
	{
		float y = wg_osc(osc, (float)x / frame.w);
		gfx_line(domain, frame.x + x - 1, my_min(my_max(-1, py), 1) * frame.h / 2 + frame.y + frame.h / 2, frame.x + x, my_min(my_max(-1, y), 1) * frame.h / 2 + frame.y + frame.h / 2, colors[COLOR_WAVETABLE_SAMPLE]);
		py = y;
	}
}


void wavegen_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect r, frame;
	copy_rect(&frame, dest);
	bevelex(domain, &frame, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	adjust_rect(&frame, 4);
	copy_rect(&r, &frame);
	r.h = 10;
	
	int d;
	
	static WgPreset presets[] = {
		{"OPL2 0", {{ {WG_OSC_SINE, WG_OP_ADD, 1, 0, 50, 255, 0, 0} }, 1}},
		{"OPL2 1", {{ {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 2}},
		{"OPL2 2", {{ {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 2}},
		{"OPL2 3", {{ {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_ABS}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 3}},
		{"OPL3 4", {{ {WG_OSC_SINE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 2}},
		{"OPL3 5", {{ {WG_OSC_SINE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, 0}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 3}},
		{"OPL3 6", {{ {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}}, 1}},
		{"OPL3 7", {{ {WG_OSC_EXP, WG_OP_MUL, 1, 0, 50, 255, 0, 0}}, 1}},
		
		// Yamaha OPZ waves, you can found them in synths like DX-11
		{"OPZ  2", {{ {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, 0}, {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, 0}, {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 3}},
		{"OPZ  4", {{ {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 4}},
		{"OPZ  6", {{ {WG_OSC_SINE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SINE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SINE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, 0}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 5}},
		{"OPZ  8", {{ {WG_OSC_SINE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SINE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SINE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 4}},
		
		//because why not?
		{"OPL2 2 cub.sin", {{ {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 4}},
		{"OPL2 3 cub.sin", {{ {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_ABS}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 5}},
		
		{"OPL2 0 cub.tri", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0, 50, 255, 0, 0}, {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0, 50, 255, 0, 0}, {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 3}},
		{"OPL2 1 cub.tri", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 4}},
		{"OPL2 2 cub.tri", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 4}},
		{"OPL2 3 cub.tri", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_ABS}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 5}},
		{"OPL3 4 cub.tri", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 2, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_TRIANGLE, WG_OP_MUL, 2, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_TRIANGLE, WG_OP_MUL, 2, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 4}},
		{"OPL3 5 cub.tri", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 2, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_TRIANGLE, WG_OP_MUL, 2, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_TRIANGLE, WG_OP_MUL, 2, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, 0}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 5}},
		
		{"OPL2 3 square", {{ {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_ABS}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 4}},
		
		// Yamaha MA series (MA-3 and later chips iirc, aka SMAF) and SD-1 (YMF825) waves, mostly modifications of triangle, pulse and saw waves, but also "cropped" OPL3 sines
		{"MA   8", {{ {WG_OSC_SINE, WG_OP_ADD, 1, 0, 50, 0x180, 0, 0} }, 1}},
		{"MA   9", {{ {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 0x180, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 2}},
		{"MA  10", {{ {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 0x180, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 2}},
		{"MA  11", {{ {WG_OSC_SINE, WG_OP_MUL, 1, 0, 50, 0x180, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_ABS}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 3}},
		{"MA  12", {{ {WG_OSC_SINE, WG_OP_MUL, 2, 0, 50, 0x180, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 2}},
		{"MA  13", {{ {WG_OSC_SINE, WG_OP_MUL, 2, 0, 50, 0x180, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, 0}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 3}},
		{"MA  14", {{ {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 2}},
		
		{"MA  16", {{ {WG_OSC_TRIANGLE, WG_OP_ADD, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG} }, 1}},
		{"MA  17", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 2}},
		{"MA  18", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 2}},
		{"MA  19", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_ABS}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 3}},
		{"MA  20", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 2, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 2}},
		{"MA  21", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 2, 0x40, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, 0}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 3}},
		{"MA  22", {{ {WG_OSC_SQUARE, WG_OP_ADD, 2, 0, 50, 255, 0, WG_OSC_FLAG_ABS | WG_OSC_FLAG_NEG} }, 1}},
		
		{"MA  24", {{ {WG_OSC_SAW, WG_OP_ADD, 1, 0x80, 50, 255, 0, WG_OSC_FLAG_NEG} }, 1}},
		{"MA  25", {{ {WG_OSC_SAW, WG_OP_MUL, 1, 0x7F, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 2}},
		{"MA  26", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x80, 50, 0x80, 0, 0}, {WG_OSC_SQUARE, WG_OP_ADD, 1, 0, 50, 255, 0, 0}, {WG_OSC_SQUARE, WG_OP_ADD, 1, 0, 99, 0x80, 0, WG_OSC_FLAG_NEG} }, 3}},
		{"MA  27", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 1, 0x40, 50, 0x80, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 2, 0, 50, 255, 0, WG_OSC_FLAG_ABS}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, 0} }, 3}},
		{"MA  28", {{ {WG_OSC_SAW, WG_OP_MUL, 2, 0x7F, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 2}},
		{"MA  29", {{ {WG_OSC_TRIANGLE, WG_OP_MUL, 2, 0x80, 50, 0x80, 0, 0}, {WG_OSC_SQUARE, WG_OP_ADD, 1, 0, 25, 255, 0, 0}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 0x80, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_ADD, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 4}},
		{"MA  30", {{ {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_NEG}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0x40, 50, 255, 0, WG_OSC_FLAG_ABS}, {WG_OSC_SQUARE, WG_OP_MUL, 1, 0, 50, 255, 0, WG_OSC_FLAG_ABS} }, 3}},
	};
	
	{
		SDL_Rect button;
		copy_rect(&button, &r);
		button.w = 38;
		button_text_event(domain, event, &button, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "LOAD", wavegen_preset, &presets[mused.selected_wg_preset], &mused.wgset, NULL);
	}
	
	if ((d = generic_field(event, &r, EDITWAVETABLE, -1, "", "%s", (char*)presets[mused.selected_wg_preset].name, 14)) != 0)
	{
		mused.selected_wg_preset += d;
		
		if (mused.selected_wg_preset < 0)
			mused.selected_wg_preset = 0;
		else if (mused.selected_wg_preset >= sizeof(presets) / sizeof(presets[0]))
			mused.selected_wg_preset = sizeof(presets) / sizeof(presets[0]) - 1;	
		
		wavegen_preset(&presets[mused.selected_wg_preset], &mused.wgset, NULL); //wasn't there
	}
	
	r.y += r.h + 2;
	
	int tmp = frame.w;
	
	r.w = frame.w / 2;
	
	frame.w = tmp;
		
	if ((d = generic_field(event, &r, EDITWAVETABLE, W_NUMOSCS, "OSCS", "%d", MAKEPTR(mused.wgset.num_oscs), 3)) != 0)
	{
		wave_add_param(d);
	}
	
	r.y += r.h + 2;
	
	int active_oscs = mused.wgset.num_oscs;
	
	for (int i = 0; i < active_oscs; ++i)
	{
		WgOsc *osc = &mused.wgset.chain[i];
		copy_rect(&r, &frame);
		r.y += 24;
		r.w = frame.w / active_oscs - 2;
		r.x = frame.x + (r.w + 2) * i;
		r.h = 32;
		oscillator_view(dest_surface, &r, event, osc);
		
		if (check_event(event, &r, NULL, 0, 0, 0))
			mused.selected_wg_osc = i;
	}
	
	for (int i = 0; i < active_oscs; ++i)
	{
		WgOsc *osc = &mused.wgset.chain[i];
		SDL_Rect r;
		copy_rect(&r, &frame);
		r.y += 34;
		r.w = 12;
		r.x = frame.x + (frame.w / active_oscs) * i + (frame.w / active_oscs) - 8;
		r.h = 12;
		
		const char *op[] = {"+", "x"};
		
		if (i < active_oscs - 1 && button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, op[osc->op], NULL, NULL, NULL, NULL) & 1)
			osc->op = (osc->op + 1) % WG_NUM_OPS;
	}
	
	WgOsc *osc = &mused.wgset.chain[mused.selected_wg_osc];
	
	r.y += r.h;
	
	r.x = frame.x;
	
	int row_begin = r.x;
	
	r.w = frame.w / 2 - 2;
	r.h = 10;
	
	if ((d = generic_field(event, &r, EDITWAVETABLE, W_OSCTYPE, "TYPE", "%d", MAKEPTR(osc->osc), 1)) != 0)
	{
		wave_add_param(d);
	}
	
	r.x += r.w + 4;
	
	if ((d = generic_field(event, &r, EDITWAVETABLE, W_OSCMUL, "MUL", "%02X", MAKEPTR(osc->mult), 2)) != 0)
	{
		wave_add_param(d);
	}
	
	r.x = row_begin;
	r.y += r.h;
	
	if ((d = generic_field(event, &r, EDITWAVETABLE, W_OSCSHIFT, "SHIFT", "%02X", MAKEPTR(osc->shift), 2)) != 0)
	{
		wave_add_param(d);
	}
	
	r.x += r.w + 4;
	
	if ((d = generic_field(event, &r, EDITWAVETABLE, W_OSCEXP, "EXP", ".%02u", MAKEPTR(osc->exp), 3)) != 0)
	{
		wave_add_param(d);
	}
	
	r.x = row_begin;
	r.y += r.h;
	r.w = frame.w; //was r.w = frame.w - 2;
	
	if ((d = generic_field(event, &r, EDITWAVETABLE, W_OSCVOL, "OSCILLATOR VOLUME", "%04X", MAKEPTR(osc->vol), 4)) != 0) //from there
	{
		wave_add_param(d);
	}
	
	r.x = row_begin;
	r.w = frame.w / 2 - 2;
	r.y += r.h; //up to there there wasn't this part
	
	generic_flags(event, &r, EDITWAVETABLE, W_OSCABS, "ABS", &osc->flags, WG_OSC_FLAG_ABS);
	
	r.x += r.w + 4;
	
	generic_flags(event, &r, EDITWAVETABLE, W_OSCNEG, "NEG", &osc->flags, WG_OSC_FLAG_NEG); 
	
	
	r.x = row_begin;
	r.y += r.h;
		
	r.x = frame.x;
	r.w = frame.w - 2;
	r.h = 10;
	
	if ((d = generic_field(event, &r, EDITWAVETABLE, W_WAVELENGTH, "LENGTH", "%9d", MAKEPTR(mused.wgset.length), 9)) != 0)
	{
		wave_add_param(d);
	}
	
	r.y += r.h + 2;
	r.w = r.w / 2;
	
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "RND+GEN", wavetable_randomize_and_create_one_cycle, &mused.wgset, NULL, NULL);
	
	r.x += r.w;
	
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "GENERATE", wavetable_create_one_cycle, &mused.wgset, NULL, NULL);
	
	r.y += r.h;
	r.x -= r.w;
	
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "RND", wavegen_randomize, &mused.wgset, NULL, NULL);
	
	r.x += r.w;
	
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "TOOLBOX", flip_bit_action, &mused.flags, MAKEPTR(SHOW_WAVEGEN), NULL);
	
	r.y += r.h; //wasn't there
	r.x -= r.w;
	
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "LOAD PATCH", wavegen_load, &mused.wgset, NULL, NULL);
	
	r.x += r.w;
	
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "SAVE PATCH", wavegen_save, &mused.wgset, NULL, NULL);
}


void wavetable_edit_area(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	if (mused.flags & SHOW_WAVEGEN)
		wavegen_view(dest_surface, dest, event, param);
	else
		wavetable_tools_view(dest_surface, dest, event, param);
}


void wavegen_preview(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 3);
	
	float py = wg_get_sample(mused.wgset.chain, mused.wgset.num_oscs, 0);
	
	if (py > 1.0)
		py = 1.0;
	else if (py < -1.0)
		py = -1.0;
	
	for (int x = 1; x < area.w; ++x)
	{
		float y = wg_get_sample(mused.wgset.chain, mused.wgset.num_oscs, (float)x / area.w);
		
		if (y > 1.0)
			y = 1.0;
		else if (y < -1.0)
			y = -1.0;
		
		gfx_line(domain, area.x + x - 1, py * area.h / 2 + area.y + area.h / 2, area.x + x, y * area.h / 2 + area.y + area.h / 2, colors[COLOR_WAVETABLE_SAMPLE]);
		py = y;
	}
}


void wavetable_sample_area(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	if (mused.flags & SHOW_WAVEGEN)
		wavegen_preview(dest_surface, dest, event, param);
	else
		wavetable_sample_view(dest_surface, dest, event, param);
}


void wavetable_name_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect farea, larea, tarea;
	copy_rect(&farea,dest);
	copy_rect(&larea,dest);
	copy_rect(&tarea,dest);
	
	farea.w = 2 * mused.console->font.w + 2 + 16;
	
	larea.w = 32;
	
	label("WAVE", &larea);
	
	tarea.w = dest->w - farea.w - larea.w - 1;
	farea.x = larea.w + dest->x;
	tarea.x = farea.x + farea.w;

	int d;
	
	if ((d = generic_field(event, &farea, EDITWAVETABLE, W_WAVE, "WAVE", "%02X", MAKEPTR(mused.selected_wavetable), 2)) != 0)
	{
		wave_add_param(d);
	}
	
	inst_field(event, &tarea, W_NAME, MUS_WAVETABLE_NAME_LEN + 1, mused.song.wavetable_names[mused.selected_wavetable]);
	
	if (is_selected_param(EDITWAVETABLE, W_NAME) || (mused.mode == EDITWAVETABLE && (mused.edit_buffer == mused.song.wavetable_names[mused.selected_wavetable] && mused.focus == EDITBUFFER)))
	{
		SDL_Rect r;
		copy_rect(&r, &tarea);
		adjust_rect(&r, -2);
		set_cursor(&r);
	}
}
