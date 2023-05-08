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
	}
	
	frame.y += 15;
	frame.h -= 15;
	
	adjust_rect(&frame, 2 + 4);
	
	gfx_rect(dest_surface, &frame, 0x0);
}

void local_sample_header_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	CydWavetableEntry *w = NULL;
	
	if(mused.show_local_samples_list)
	{
		w = mused.song.instrument[mused.current_instrument].local_samples[mused.selected_local_sample];
	}
	
	else
	{
		w = &mused.mus.cyd->wavetable_entries[mused.selected_local_sample];
	}
	
	if(w == NULL)
	{
		return;
	}
	
	MusInstrument* inst = &mused.song.instrument[mused.current_instrument];
	
	SDL_Rect r, frame;
	copy_rect(&frame, dest);
	bevelex(domain, &frame, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	
	adjust_rect(&frame, 2);
	copy_rect(&r, &frame);
	
	r.h = 10;
	
	int d;
	
	r.w = 134;
	
	generic_flags(event, &r, EDITLOCALSAMPLE, LS_ENABLE, "USE LOCAL SAMPLES", &inst->flags, MUS_INST_USE_LOCAL_SAMPLES);
	update_rect(&frame, &r);
	r.x += 10;
	
	r.w = 34;
	
	if ((d = generic_field(event, &r, EDITLOCALSAMPLE, LS_LOCAL_SAMPLE, "", "%02X", MAKEPTR(inst->local_sample), 2)) != 0)
	{
		local_sample_add_param(d);
	}
	
	update_rect(&frame, &r);
	
	r.w = 70;
	
	int change_to_local = button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, mused.show_local_samples_list ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "LOCAL S.", NULL, MAKEPTR(1), NULL, NULL);
	
	update_rect(&frame, &r);
	
	int change_to_global = button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, !(mused.show_local_samples_list) ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "GLOBAL S.", NULL, MAKEPTR(1), NULL, NULL);
	
	update_rect(&frame, &r);
	
	if (event->type == SDL_MOUSEBUTTONDOWN)
	{
		if(change_to_local > 0)
		{
			mused.show_local_samples_list = true;
			invalidate_wavetable_view();
			
			if(mused.selected_local_sample > inst->num_local_samples - 1)
			{
				mused.selected_local_sample = inst->num_local_samples - 1;
			}
		}
		
		if(change_to_global > 0)
		{
			mused.show_local_samples_list = false;
			invalidate_wavetable_view();
		}
	}
	
	r.x += 30;
	r.w = 30;
	
	int add_local = button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "ADD", NULL, MAKEPTR(1), NULL, NULL);
	update_rect(&frame, &r);
	
	r.w = 50;
	
	int delete_local = button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "DELETE", NULL, MAKEPTR(1), NULL, NULL);
	update_rect(&frame, &r);
	
	if (event->type == SDL_MOUSEBUTTONDOWN && mused.show_local_samples_list)
	{
		if(add_local > 0)
		{
			if(inst->num_local_samples < 0xff)
			{
				inst->num_local_samples++;
				
				inst->local_samples = (CydWavetableEntry**)realloc(inst->local_samples, sizeof(inst->local_samples[0]) * inst->num_local_samples);
				inst->local_samples[inst->num_local_samples - 1] = (CydWavetableEntry*)calloc(1, sizeof(CydWavetableEntry));
				cyd_wave_entry_init(inst->local_samples[inst->num_local_samples - 1], NULL, 0, 0, 0, 0, 0);
				
				inst->local_sample_names = (char**)realloc(inst->local_sample_names, sizeof(inst->local_sample_names[0]) * inst->num_local_samples);
				inst->local_sample_names[inst->num_local_samples - 1] = (char*)calloc(1, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
				memset(inst->local_sample_names[inst->num_local_samples - 1], 0, sizeof(inst->local_sample_names[0][0]) * MUS_WAVETABLE_NAME_LEN);
			}
		}
		
		if(delete_local > 0)
		{
			if(inst->num_local_samples > 1)
			{
				inst->num_local_samples--;
				
				cyd_wave_entry_deinit(inst->local_samples[inst->num_local_samples]);
				free(inst->local_samples[inst->num_local_samples]);
				
				inst->local_samples = (CydWavetableEntry**)realloc(inst->local_samples, sizeof(inst->local_samples[0]) * inst->num_local_samples);
				
				free(inst->local_sample_names[inst->num_local_samples]);
				
				inst->local_sample_names = (char**)realloc(inst->local_sample_names, sizeof(inst->local_sample_names[0]) * inst->num_local_samples);
				
				if(mused.selected_local_sample > inst->num_local_samples - 1)
				{
					mused.selected_local_sample = inst->num_local_samples - 1;
				}
			}
		}
	}
}

void local_sample_notes_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect area;
	copy_rect(&area, dest);
	area.w -= 10;
	area.h -= 12;
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(dest_surface, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 3);
	console_set_clip(mused.console, &area);
	gfx_domain_set_clip(dest_surface, &area);

	int y = area.y;
	
	int start = mused.local_sample_note_list_position;
	
	MusInstrument* inst = &mused.song.instrument[mused.current_instrument];
	
	for (int i = start; i <= FREQ_TAB_SIZE && y < area.h + area.y; ++i, y += mused.console->font.h)
	{
		SDL_Rect row = { area.x - 1, y - 1, area.w + 2, mused.console->font.h + 1};
		
		if (i == mused.selected_local_sample_note)
		{
			bevel(dest_surface, &row, mused.slider_bevel, BEV_SELECTED_PATTERN_ROW);
			console_set_color(mused.console, colors[COLOR_INSTRUMENT_SELECTED]);
		}
		
		else
		{
			console_set_color(mused.console, colors[COLOR_INSTRUMENT_NORMAL]);
		}
		
		char temp[1000] = "";
		
		char sample_info[1000] = "";
		
		char* sample_name = NULL;
		
		if(inst->note_to_sample_array[i].sample != MUS_NOTE_TO_SAMPLE_NONE)
		{
			if(inst->note_to_sample_array[i].flags & MUS_NOTE_TO_SAMPLE_GLOBAL)
			{
				sample_name = mused.song.wavetable_names[inst->note_to_sample_array[i].sample];
			}
			
			else
			{
				sample_name = mused.song.instrument[mused.current_instrument].local_sample_names[inst->note_to_sample_array[i].sample];
			}
		}
		
		snprintf(sample_info, sizeof(sample_info), "[%c] %s", ((inst->note_to_sample_array[i].sample == MUS_NOTE_TO_SAMPLE_NONE) ? ' ' : ((inst->note_to_sample_array[i].flags & MUS_NOTE_TO_SAMPLE_GLOBAL) ? 'G' : 'L')), ((sample_name != NULL) ? sample_name : ""));
		
		if(i < FREQ_TAB_SIZE)
		{
			console_write_args(mused.console, "%s - %s\n", notename(i), sample_info);
		}
		
		check_event(event, &row, select_local_sample_note, MAKEPTR(i), 0, 0);
		
		slider_set_params(&mused.local_sample_note_list_slider_param, 0, FREQ_TAB_SIZE, start, i, &mused.local_sample_note_list_position, 1, SLIDER_VERTICAL, mused.slider_bevel);
	}
	
	gfx_domain_set_clip(dest_surface, NULL);
	
	adjust_rect(&area, -3);
	
	int temp = area.w;
	
	area.x += area.w;
	area.w = 10;
	
	slider(dest_surface, &area, event, &mused.local_sample_note_list_slider_param);
	
	area.y += area.h;
	
	area.h = 12;
	area.w = (temp + 10) / 2;
	area.x = dest->x;
	
	int add_to_list = button_text_event(domain, event, &area, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "\xb8\xb6\xb6\xb6", NULL, MAKEPTR(1), NULL, NULL);
	
	area.x += area.w;
	
	int remove_from_list = button_text_event(domain, event, &area, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "\xb6\xb6\xb6\xb7", NULL, MAKEPTR(1), NULL, NULL);
}

void local_sample_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect r, frame;
	copy_rect(&frame, dest);
	bevelex(domain, &frame, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	adjust_rect(&frame, 4);
	copy_rect(&r, &frame);
	
	CydWavetableEntry *w = NULL;
	
	MusInstrument* inst = &mused.song.instrument[mused.current_instrument];
	
	if(mused.show_local_samples_list)
	{
		w = mused.song.instrument[mused.current_instrument].local_samples[mused.selected_local_sample];
	}
	
	else
	{
		w = &mused.mus.cyd->wavetable_entries[mused.selected_local_sample];
	}
	
	if(w == NULL)
	{
		return;
	}
	
	{
		r.w = 64;
		r.h = 10;
		
		int d;
		
		r.w = 128;
		
		if ((d = generic_field(event, &r, EDITLOCALSAMPLE, LS_RATE, "RATE", "%6d Hz", MAKEPTR(w->sample_rate), 9)) != 0)
		{
			local_sample_add_param(d);
		}
		
		update_rect(&frame, &r);
		
		r.w = 72;
		r.h = 10;
		
		if ((d = generic_field(event, &r, EDITLOCALSAMPLE, LS_BASE, "BASE", "%s", notename((w->base_note + 0x80) >> 8), 3)) != 0)
		{
			local_sample_add_param(d);
		}
		
		update_rect(&frame, &r);
		r.w = 48;
		
		if ((d = generic_field(event, &r, EDITLOCALSAMPLE, LS_BASEFINE, "", "%+4d", MAKEPTR((Sint8)w->base_note), 4)) != 0)
		{
			local_sample_add_param(d);
		}
		
		r.w = 128;
		
		update_rect(&frame, &r);
		
		generic_flags(event, &r, EDITLOCALSAMPLE, LS_INTERPOLATE, "NO INTERPOLATION", &w->flags, CYD_WAVE_NO_INTERPOLATION);
		
		update_rect(&frame, &r);
		
		r.w = 138;
		
		static const char *interpolations[] = { "NO INT", "LINEAR", "COSINE", "CUBIC", "GAUSS", "SINC" };
		
		if ((d = generic_field(event, &r, EDITLOCALSAMPLE, LS_INTERPOLATION_TYPE, "INT. TYPE", "%s", MAKEPTR(interpolations[w->flags & CYD_WAVE_NO_INTERPOLATION ? 0 : ((w->flags & (CYD_WAVE_INTERPOLATION_BIT_1|CYD_WAVE_INTERPOLATION_BIT_2|CYD_WAVE_INTERPOLATION_BIT_3)) >> 5) + 1]), 6)) != 0)
		{
			local_sample_add_param(d);
		}
		
		update_rect(&frame, &r);
	}
	
	my_separator(&frame, &r);
	
	{
		r.w = 80;
		
		generic_flags(event, &r, EDITLOCALSAMPLE, LS_LOOP, "LOOP", &w->flags, CYD_WAVE_LOOP);
		
		update_rect(&frame, &r);
		
		r.w = 112;
		
		int d;
		
		if ((d = generic_field(event, &r, EDITLOCALSAMPLE, LS_LOOPBEGIN, "BEGIN", "%7d", MAKEPTR(w->loop_begin), 7)) != 0)
		{
			local_sample_add_param(d);
		}
		
		update_rect(&frame, &r);
		
		r.w = 80;
		
		generic_flags(event, &r, EDITLOCALSAMPLE, LS_LOOPPINGPONG, "PINGPONG", &w->flags, CYD_WAVE_PINGPONG);
		
		update_rect(&frame, &r);
		
		
		r.w = 112;
		
		if ((d = generic_field(event, &r, EDITLOCALSAMPLE, LS_LOOPEND, "END", "%7d", MAKEPTR(w->loop_end), 7)) != 0)
		{
			local_sample_add_param(d);
		}
		
		update_rect(&frame, &r);
	}
	
	my_separator(&frame, &r);
	
	r.w = 110;
	
	generic_flags(event, &r, EDITLOCALSAMPLE, LS_BINDTONOTES, "BIND TO NOTES", &inst->flags, MUS_INST_BIND_LOCAL_SAMPLES_TO_NOTES);
	
	//update_rect(&frame, &r);
	
	r.y += 10;
	
	r.w = frame.w;
	r.x = frame.x;
	
	r.h = frame.h - (r.y - frame.y);
	
	local_sample_notes_view(dest_surface, &r, event, param);
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
	
	int start = mused.local_sample_list_position;
	
	CydWavetableEntry* w = NULL;
	
	if(mused.show_local_samples_list)
	{
		w = mused.song.instrument[mused.current_instrument].local_samples[mused.selected_local_sample];
	}
	
	else
	{
		w = &mused.mus.cyd->wavetable_entries[mused.selected_local_sample];
	}
	
	if(w == NULL)
	{
		return;
	}
	
	for (int i = start; i < (mused.show_local_samples_list ? mused.song.instrument[mused.current_instrument].num_local_samples : (CYD_WAVE_MAX_ENTRIES - 1)) && y < area.h + area.y; ++i, y += mused.console->font.h)
	{
		SDL_Rect row = { area.x - 1, y - 1, area.w + 2, mused.console->font.h + 1};
		if (i == mused.selected_local_sample)
		{
			bevel(dest_surface, &row, mused.slider_bevel, BEV_SELECTED_PATTERN_ROW);
			console_set_color(mused.console, colors[COLOR_INSTRUMENT_SELECTED]);
		}
		
		else
		{
			console_set_color(mused.console, colors[COLOR_INSTRUMENT_NORMAL]);
		}
		
		CydWavetableEntry *w = NULL;
		
		if(mused.show_local_samples_list)
		{
			w = mused.song.instrument[mused.current_instrument].local_samples[i];
		}
		
		else
		{
			w = &mused.mus.cyd->wavetable_entries[i];
		}
		
		char temp[1000] = "";
		
		if(w)
		{
			if (w->samples > 0 || (mused.show_local_samples_list ? mused.song.instrument[mused.current_instrument].local_sample_names[i][0] : mused.song.wavetable_names[i][0]))
			{
				snprintf(temp, chars, "%s (%u smp)", (mused.show_local_samples_list ? (mused.song.instrument[mused.current_instrument].local_sample_names[i][0] ? mused.song.instrument[mused.current_instrument].local_sample_names[i] : "No name") : (mused.song.wavetable_names[i][0] ? mused.song.wavetable_names[i] : "No name")), w->samples);
			}
			
			console_write_args(mused.console, "%02X %s\n", i, temp);
			
			check_event(event, &row, select_local_sample, MAKEPTR(i), 0, 0);
		}
		
		slider_set_params(&mused.local_sample_list_slider_param, 0, (mused.show_local_samples_list ? (mused.song.instrument[mused.current_instrument].num_local_samples - 1) : (CYD_WAVE_MAX_ENTRIES - 1)), start, i, &mused.local_sample_list_position, 1, SLIDER_VERTICAL, mused.slider_bevel);
	}
	
	gfx_domain_set_clip(dest_surface, NULL);
	
	if (event->type == SDL_MOUSEWHEEL && mused.focus == EDITLOCALSAMPLE)
	{
		if (event->wheel.y > 0)
		{
			mused.local_sample_list_position -= 4;
			*(mused.local_sample_list_slider_param.position) -= 4;
		}
		
		else
		{
			mused.local_sample_list_position += 4;
			*(mused.local_sample_list_slider_param.position) += 4;
		}
		
		int max_num = mused.show_local_samples_list ? (mused.song.instrument[mused.current_instrument].num_local_samples) : (CYD_WAVE_MAX_ENTRIES);
		
		mused.local_sample_list_position = my_max(0, my_min(max_num - area.h / 8, mused.local_sample_list_position));
		*(mused.local_sample_list_slider_param.position) = my_max(0, my_min(max_num - area.h / 8, *(mused.local_sample_list_slider_param.position)));
	}
}


static void update_local_sample_preview(GfxDomain *dest, const SDL_Rect* area)
{
	if (!mused.wavetable_preview || (mused.wavetable_preview->surface->w != area->w || mused.wavetable_preview->surface->h != area->h))
	{
		if (mused.wavetable_preview) gfx_free_surface(mused.wavetable_preview);
		
		mused.wavetable_preview = gfx_create_surface(dest, area->w, area->h);	
	}
	
	else if (mused.wavetable_preview_idx == mused.selected_local_sample) return;
	
	mused.wavetable_preview_idx = mused.selected_local_sample;
	
	SDL_FillRect(mused.wavetable_preview->surface, NULL, SDL_MapRGB(mused.wavetable_preview->surface->format, (colors[COLOR_WAVETABLE_BACKGROUND] >> 16) & 255, (colors[COLOR_WAVETABLE_BACKGROUND] >> 8) & 255, colors[COLOR_WAVETABLE_BACKGROUND] & 255));
	
	CydWavetableEntry *w = NULL;
	
	if(mused.show_local_samples_list)
	{
		w = mused.song.instrument[mused.current_instrument].local_samples[mused.selected_local_sample];
	}
	
	else
	{
		w = &mused.mus.cyd->wavetable_entries[mused.selected_local_sample];
	}
	
	if(w == NULL)
	{
		return;
	}
	
	mused.wavetable_bits = 0;
	
	if (w->samples > 0)
	{
		const int res = 4096;
		const int dadd = my_max(res, w->samples * res / area->w);
		const int gadd = my_max(res, (Uint32)area->w * res / w->samples);
		int c = 0, d = 0;
		
		int prevmin = -66666;
		
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
			
			//if (min < 0 && max < 0)
				//max = 0;
				
			//if (min > 0 && max > 0)
				//min = 0;
			
			min = (32768 + min) * area->h / 65536;
			max = (32768 + max) * area->h / 65536 - min;
		
			int prev_x = x - 1;
			x += gadd;
			
			//SDL_Rect r = { prev_x / res, min, my_max(1, x / res - prev_x / res), max + 1 };
			SDL_Rect r = { prev_x / res, min, my_max(1, x / res - prev_x / res), 1 };
			
			SDL_FillRect(mused.wavetable_preview->surface, &r, SDL_MapRGB(mused.wavetable_preview->surface->format, (colors[COLOR_WAVETABLE_SAMPLE] >> 16) & 255, (colors[COLOR_WAVETABLE_SAMPLE] >> 8) & 255, colors[COLOR_WAVETABLE_SAMPLE] & 255));
			
			if(prevmin != -66666)
			{
				r = (SDL_Rect) { prev_x / res, (min - prevmin) > 0 ? prevmin : min, 1, abs(min - prevmin) + 1 };
				
				SDL_FillRect(mused.wavetable_preview->surface, &r, SDL_MapRGB(mused.wavetable_preview->surface->format, (colors[COLOR_WAVETABLE_SAMPLE] >> 16) & 255, (colors[COLOR_WAVETABLE_SAMPLE] >> 8) & 255, colors[COLOR_WAVETABLE_SAMPLE] & 255));
			}
			
			prevmin = min;
		}
		
		debug("Wavetable item bitmask = %x, lowest bit = %d", mused.wavetable_bits, __builtin_ffs(mused.wavetable_bits) - 1);
		set_info_message("Sample quality %d bits", 16 - (__builtin_ffs(mused.wavetable_bits) - 1));
	}
	
	gfx_update_texture(dest, mused.wavetable_preview);
}

void local_sample_draw(float x, float y, float width)
{
	CydWavetableEntry *w = NULL;
	
	if(mused.show_local_samples_list)
	{
		w = mused.song.instrument[mused.current_instrument].local_samples[mused.selected_local_sample];
	}
	
	else
	{
		w = &mused.mus.cyd->wavetable_entries[mused.selected_local_sample];
	}
	
	if(w == NULL)
	{
		return;
	}

	if (w->samples > 0)
	{
		debug("draw %f,%f w = %f", x, y, width);
		int s = w->samples * x;
		int e = my_max(w->samples * (x + width), s + 1);
		
		for (; s < e && s < w->samples; ++s)
		{
			w->data[s] = y * 65535 - 32768;
		}
		
		invalidate_wavetable_view();
	}
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
	
	if (mused.mode == EDITLOCALSAMPLE && (SDL_GetMouseState(&mx, &my) & SDL_BUTTON(1)))
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
					local_sample_draw((float)(x - area.x) / area.w, (float)((my + (mused.prev_wavetable_y - my) * i / d) - area.y) / area.h, 1.0f / area.w);
			}
			else
				local_sample_draw((float)(mx - area.x) / area.w, (float)(my - area.y) / area.h, 1.0f / area.w);
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
	
	if ((d = generic_field(event, &farea, EDITLOCALSAMPLE, LS_WAVE, "WAVE", "%02X", MAKEPTR(mused.selected_local_sample), 2)) != 0)
	{
		local_sample_add_param(d);
	}
	
	inst_field(event, &tarea, LS_NAME, MUS_WAVETABLE_NAME_LEN + 1, (mused.show_local_samples_list ? mused.song.instrument[mused.current_instrument].local_sample_names[mused.selected_local_sample] : mused.song.wavetable_names[mused.selected_local_sample]));
	
	if (is_selected_param(EDITLOCALSAMPLE, LS_NAME) || (mused.mode == EDITLOCALSAMPLE && ((mused.edit_buffer == (mused.show_local_samples_list ? mused.song.instrument[mused.current_instrument].local_sample_names[mused.selected_local_sample] : mused.song.wavetable_names[mused.selected_local_sample]) && mused.focus == EDITBUFFER))))
	{
		SDL_Rect r;
		copy_rect(&r, &tarea);
		adjust_rect(&r, -2);
		set_cursor(&r);
	}
}