#include "localsample.h"

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

extern Mused mused;

void local_sample_bevel(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect frame;
	copy_rect(&frame, dest);
	adjust_rect(&frame, -2);
	
	bevelex(domain, &frame, mused.slider_bevel, BEV_MENU, BEV_F_STRETCH_ALL);
	
	const char* title = "Local samples settings (\"sample map\")";
	SDL_Rect titlearea, button;
	copy_rect(&titlearea, dest);
	
	adjust_rect(&titlearea, 6);
	
	copy_rect(&button, dest);
	adjust_rect(&button, titlearea.h);
	button.w = 12;
	
	button.h = 12;
	
	button.x = dest->w + dest->x - 16;
	button.y -= dest->h - 17;
	
	font_write(&mused.largefont, dest_surface, &titlearea, title);
	
	if (button_event(dest_surface, event, &button, mused.slider_bevel, BEV_BUTTON, BEV_BUTTON_ACTIVE, DECAL_CLOSE, NULL, MAKEPTR(1), 0, 0) & 1) //button to exit filebox
	{
		snapshot(S_T_MODE);
		
		mused.show_local_samples = false;
		
		change_mode(EDITINSTRUMENT);
		
		snapshot(S_T_MODE);
	}
	
	frame.y += 15;
	frame.h -= 15;
	
	adjust_rect(&frame, 2 + 4);
	
	gfx_rect(dest_surface, &frame, 0x0);
}

void local_sample_header_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect r, frame;
	copy_rect(&frame, dest);
	bevelex(domain, &frame, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
}

void local_sample_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
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


void local_samplelist_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
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
	
	//check_mouse_wheel_event(event, dest, &mused.wavetable_list_slider_param);
	
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


static void update_local_sample_preview(GfxDomain *dest, const SDL_Rect* area)
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

void local_sample_sample_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 3);
	update_local_sample_preview(dest_surface, &area);
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
}

void local_sample_tools_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
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
	
	button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "OCTAVE", wavetable_chord, MAKEPTR(12), NULL, NULL);
}

void local_sample_edit_area(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	local_sample_tools_view(dest_surface, dest, event, param);
}


void local_sample_sample_area(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	local_sample_sample_view(dest_surface, dest, event, param);
}


void local_sample_name_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
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