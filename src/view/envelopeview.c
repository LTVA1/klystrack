/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2023 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (GfxDomain *dest_surface, the "Software"), to deal in the Software without
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

#include "envelopeview.h"

#define ENV_WINDOW_HEIGHT 128
#define ENV_ED_MARGIN 8

void env_flags(const SDL_Event *e, const SDL_Rect *area, const SDL_Rect *dest, int p, const char *label, Uint32 *flags, Uint32 mask)
{
	generic_flags(e, area, EDITENVELOPE, p, label, flags, mask);
	
	if (check_mouse_hit(e, area, EDITENVELOPE, p) || (mused.env_selected_param == p && mused.focus == EDITENVELOPE))
	{
		SDL_Rect cur;
		copy_rect(&cur, area);
		adjust_rect(&cur, -1);
		cur.x -= 1;
		cur.w += 1;
		set_cursor(&cur);
		
		if(area->y >= dest->y && area->y <= dest->y + dest->h) {}
		else
		{
			//set_cursor(NULL); //so cursor is not drawn outside of envelope editor frame
		}
	}
}


void env_text(const SDL_Event *e, const SDL_Rect *area, const SDL_Rect *dest, int p, const char *_label, const char *format, void *value, int width)
{
	int d = generic_field(e, area, EDITENVELOPE, p, _label, format, value, width);
	
	if (d)
	{
		if (p >= 0) mused.env_selected_param = p;
		//snapshot_cascade(S_T_INSTRUMENT, mused.current_instrument, p);
		snapshot(S_T_INSTRUMENT);
		if (d < 0) env_editor_add_param(-1);
		else if (d > 0) env_editor_add_param(1);
		
		if(area->y >= dest->y && area->y <= dest->y + dest->h) {}
		else
		{
			//set_cursor(NULL); //so cursor is not drawn outside of envelope editor frame
		}
	}
	
	if (check_mouse_hit(e, area, EDITENVELOPE, p) || (mused.env_selected_param == p && mused.focus == EDITENVELOPE))
	{
		SDL_Rect cur;
		copy_rect(&cur, area);
		adjust_rect(&cur, -2);
		set_cursor(&cur);
		
		if(area->y >= dest->y && area->y <= dest->y + dest->h) {}
		else
		{
			//set_cursor(NULL); //so cursor is not drawn outside of envelope editor frame
		}
	}
}

void fourop_env_flags(const SDL_Event *e, const SDL_Rect *area, const SDL_Rect *dest, int p, const char *label, Uint32 *flags, Uint32 mask)
{
	generic_flags(e, area, EDITENVELOPE4OP, p, label, flags, mask);
	
	if (check_mouse_hit(e, area, EDITENVELOPE4OP, p) || (mused.fourop_env_selected_param == p && mused.focus == EDITENVELOPE4OP))
	{
		SDL_Rect cur;
		copy_rect(&cur, area);
		adjust_rect(&cur, -1);
		cur.x -= 1;
		cur.w += 1;
		set_cursor(&cur);
		
		if(area->y >= dest->y && area->y <= dest->y + dest->h) {}
		else
		{
			set_cursor(NULL); //so cursor is not drawn outside of envelope editor frame
		}
	}
}


void fourop_env_text(const SDL_Event *e, const SDL_Rect *area, const SDL_Rect *dest, int p, const char *_label, const char *format, void *value, int width)
{
	int d = generic_field(e, area, EDITENVELOPE4OP, p, _label, format, value, width);
	
	if (d)
	{
		if (p >= 0) mused.fourop_env_selected_param = p;
		//snapshot_cascade(S_T_INSTRUMENT, mused.current_instrument, p);
		snapshot(S_T_INSTRUMENT);
		if (d < 0) fourop_env_editor_add_param(-1);
		else if (d > 0) fourop_env_editor_add_param(1);
		
		if(area->y >= dest->y && area->y <= dest->y + dest->h) {}
		else
		{
			set_cursor(NULL); //so cursor is not drawn outside of envelope editor frame
		}
	}
	
	if (check_mouse_hit(e, area, EDITENVELOPE4OP, p) || (mused.fourop_env_selected_param == p && mused.focus == EDITENVELOPE4OP))
	{
		SDL_Rect cur;
		copy_rect(&cur, area);
		adjust_rect(&cur, -2);
		set_cursor(&cur);
		
		if(area->y >= dest->y && area->y <= dest->y + dest->h) {}
		else
		{
			set_cursor(NULL); //so cursor is not drawn outside of envelope editor frame
		}
	}
}

void fourop_point_envelope_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	if(!(mused.show_4op_point_envelope_editor)) return;
	if(!(mused.show_four_op_menu)) return;
	
	MusInstrument *inst = &mused.song.instrument[mused.current_instrument];
	
	SDL_Rect area;
	copy_rect(&area, dest);
	
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	//bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 1);
	
	gfx_domain_set_clip(domain, &area);
	
	SDL_Rect vol_env_header;
	copy_rect(&vol_env_header, &area);
	
	adjust_rect(&vol_env_header, 2);
	
	vol_env_header.y -= mused.fourop_point_env_editor_scroll;
	vol_env_header.h = TOP_VIEW_H - 4;
	
	bevelex(domain, &vol_env_header, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	
	adjust_rect(&vol_env_header, 4);
	
	console_set_clip(mused.console, &vol_env_header);
	
	//font_write_args(&mused.largefont, domain, &area, "test\ntest\ntest\ntest\ntest\ntest\n");
	
	SDL_Rect r;
	
	copy_rect(&r, &vol_env_header);
	r.w = 150;
	r.h = 10;
	r.y -= 2;
	
	fourop_env_flags(event, &r, dest, FOUROP_ENV_ENABLE_VOLUME_ENVELOPE, "USE VOLUME ENVELOPE", (Uint32*)&inst->ops[mused.selected_operator - 1].flags, MUS_FM_OP_USE_VOLUME_ENVELOPE);
	update_rect(&vol_env_header, &r);
	
	r.w = 95;
	r.x += 10;
	
	fourop_env_text(event, &r, dest, FOUROP_ENV_VOLUME_ENVELOPE_FADEOUT, "FADEOUT", "%03X", MAKEPTR(inst->ops[mused.selected_operator - 1].vol_env_fadeout), 3);
	update_rect(&vol_env_header, &r);
	
	r.w = 95;
	
	float vol_zoom_mult = mused.fourop_vol_env_scale < 0 ? 2.0 : (1.0 / (float)mused.fourop_vol_env_scale);
	
	char fuck_string[10];
	
	sprintf(fuck_string, "%0.2fx", vol_zoom_mult);
	
	if(mused.fourop_vol_env_scale < 0)
	{
		fourop_env_text(event, &r, dest, FOUROP_ENV_VOLUME_ENVELOPE_SCALE, "ZOOM", "2x", MAKEPTR(vol_zoom_mult), 5);
	}
	
	else
	{
		fourop_env_text(event, &r, dest, FOUROP_ENV_VOLUME_ENVELOPE_SCALE, "ZOOM", fuck_string, (void*)&vol_zoom_mult, 5);
	}
	
	update_rect(&vol_env_header, &r);
	
	SDL_Rect vol_env_editor;
	copy_rect(&vol_env_editor, &area);
	
	adjust_rect(&vol_env_editor, 2);
	
	vol_env_editor.y -= mused.fourop_point_env_editor_scroll;
	vol_env_editor.y += TOP_VIEW_H - 4;
	vol_env_editor.h = ENV_WINDOW_HEIGHT + 2 * ENV_ED_MARGIN;
	vol_env_editor.w -= 200 - 2 * ENV_ED_MARGIN;
	
	SDL_Rect vol_env_editor_init;
	copy_rect(&vol_env_editor_init, &vol_env_editor);
	
	adjust_rect(&vol_env_editor, ENV_ED_MARGIN);
	
	SDL_Rect vol_env_info;
	
	vol_env_info.y = vol_env_editor.y - 4;
	vol_env_info.x = vol_env_editor.x + vol_env_editor.w - 4 * 9 - 4 - 4;
	vol_env_info.h = 8;
	vol_env_info.w = 4 * 12;
	
	if(mused.fourop_vol_env_point != -1)
	{
		font_write_args(&mused.tinyfont, dest_surface, &vol_env_info, "TIME:%0.2fs\n VOL:%02X", (float)mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].x / 100.0, mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].y);
	}
	
	//mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].x
	
	bool should_continue = true;
	
	for(int i = 0; (i < mused.song.num_channels) && should_continue; ++i)
	{
		if(mused.mus.channel[i].instrument != NULL)
		{
			if(mused.mus.channel[i].instrument == inst && (inst->ops[mused.selected_operator - 1].flags & MUS_FM_OP_USE_VOLUME_ENVELOPE))
			{
				if(mused.mus.cyd->channel[i].fm.ops[mused.selected_operator - 1].adsr.envelope > 0 && (mused.mus.cyd->channel[i].fm.ops[mused.selected_operator - 1].flags & CYD_FM_OP_ENABLE_GATE)) //if custom envelope is playing draw current position
				{
					should_continue = false;
					
					SDL_Rect current_position;
		
					copy_rect(&current_position, &vol_env_editor);
					
					current_position.w = 16;
					current_position.x = vol_env_editor.x + (((mused.mus.cyd->channel[i].fm.ops[mused.selected_operator - 1].adsr.envelope >> 16) - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult) - 7;
					current_position.y -= (ENV_ED_MARGIN - 2);
					current_position.h += 2 * (ENV_ED_MARGIN - 2);
					
					bevelex(domain, &current_position, mused.slider_bevel, BEV_ENV_CURRENT_ENV_POSITION, BEV_F_NORMAL);
					
					gfx_translucent_rect(domain, &current_position, colors[COLOR_WAVETABLE_BACKGROUND] | (Uint32)(255 - (mused.mus.cyd->channel[i].fm.ops[mused.selected_operator - 1].adsr.curr_vol_fadeout_value >> (31 - 8))) << 24);
				}
			}
		}
	}
	
	if(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_flags & MUS_ENV_SUSTAIN) //draw sustain
	{
		SDL_Rect sustain_point;
		
		copy_rect(&sustain_point, &vol_env_editor);
		
		sustain_point.w = 16;
		sustain_point.x = vol_env_editor.x + (mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_sustain].x - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult - 7;
		sustain_point.y -= (ENV_ED_MARGIN - 2);
		sustain_point.h += 2 * (ENV_ED_MARGIN - 2);
		
		bevelex(domain, &sustain_point, mused.slider_bevel, BEV_ENV_SUSTAIN_POINT, BEV_F_NORMAL);
	}
	
	if(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_flags & MUS_ENV_LOOP) //draw loop begin and end
	{
		SDL_Rect loop_begin, loop_end;
		
		copy_rect(&loop_begin, &vol_env_editor);
		copy_rect(&loop_end, &vol_env_editor);
		
		loop_begin.w = 16;
		loop_begin.x = vol_env_editor.x + (mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_loop_start].x - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult;
		loop_begin.y -= (ENV_ED_MARGIN - 2);
		loop_begin.h += 2 * (ENV_ED_MARGIN - 2);
		
		bevelex(domain, &loop_begin, mused.slider_bevel, BEV_ENV_LOOP_START, BEV_F_NORMAL);
		
		loop_end.w = 16;
		loop_end.x = vol_env_editor.x + (mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_loop_end].x - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult - 15;
		loop_end.y -= (ENV_ED_MARGIN - 2);
		loop_end.h += 2 * (ENV_ED_MARGIN - 2);
		
		bevelex(domain, &loop_end, mused.slider_bevel, BEV_ENV_LOOP_END, BEV_F_NORMAL);
	}
	
	for(int i = 0; i < mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points; ++i)
	{
		int x = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i].x;
		int y = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i].y;
		
		if(i > 0)
		{
			int prev_x = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i - 1].x;
			int prev_y = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i - 1].y;
			
			gfx_line(domain, vol_env_editor.x + (prev_x - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult, vol_env_editor.y + (ENV_WINDOW_HEIGHT - prev_y) - 1, vol_env_editor.x + (x - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult, vol_env_editor.y + (ENV_WINDOW_HEIGHT - y) - 1, colors[COLOR_WAVETABLE_SAMPLE]);
		}
	}
	
	for(int i = 0; i < mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points; ++i)
	{
		int x = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i].x;
		int y = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i].y;

		SDL_Rect rr = { vol_env_editor.x + (x - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult - mused.smallfont.w / 2,
			vol_env_editor.y + (ENV_WINDOW_HEIGHT - y) - mused.smallfont.h / 2, mused.smallfont.w, mused.smallfont.h };
		
		font_write(&mused.smallfont, dest_surface, &rr, "\2");

		if (i == mused.fourop_vol_env_point)
		{
			SDL_Rect sel;
			copy_rect(&sel, &rr);
			adjust_rect(&sel, -4);
			bevelex(domain, &sel, mused.slider_bevel, BEV_CURSOR, BEV_F_STRETCH_ALL);
		}

		if (event->type == SDL_MOUSEBUTTONDOWN)
		{
			if (check_event(event, &rr, NULL, 0, 0, 0) && vol_env_editor.x + (x - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult >= vol_env_editor.x && vol_env_editor.x + (x - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult <= vol_env_editor.x + vol_env_editor.w)
			{
				mused.fourop_vol_env_point = i;
			}
		}
	}
	
	int mx, my;
	
	Uint32 buttons = SDL_GetMouseState(&mx, &my);
	
	mx /= mused.pixel_scale;
	my /= mused.pixel_scale;
	
	if (mused.focus == EDITENVELOPE4OP)
	{
		if (mx >= vol_env_editor.x - 5 && mx <= vol_env_editor.x + vol_env_editor.w + 5 && my >= vol_env_editor.y - 5 && my <= vol_env_editor.y + vol_env_editor.h + 5)
		{
			if (event->type == SDL_MOUSEBUTTONDOWN)
			{
				snapshot(S_T_INSTRUMENT);
			}
		}
	}
	
	if (mused.focus == EDITENVELOPE4OP && (buttons & SDL_BUTTON(1)))
	{
		if (mx >= vol_env_editor.x - 5 && mx <= vol_env_editor.x + vol_env_editor.w + 5 && my >= vol_env_editor.y - 5 && my <= vol_env_editor.y + vol_env_editor.h + 5)
		{
			if (mused.fourop_prev_vol_env_x != -1)
			{
				int x = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].x;
				int y = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].y;

				SDL_Rect rr = { vol_env_editor.x + (x - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult - mused.smallfont.w / 2,
					vol_env_editor.y + (ENV_WINDOW_HEIGHT - y) - mused.smallfont.h / 2 - 3, mused.smallfont.w, mused.smallfont.h };
					
				adjust_rect(&rr, -4);
				
				int mouse_x = (mused.fourop_prev_vol_env_x == -1) ? mx : mused.fourop_prev_vol_env_x;
				int mouse_y = (mused.fourop_prev_vol_env_y == -1) ? my : mused.fourop_prev_vol_env_y;
				
				//debug("self dot x %d, dot x %d left edge %d right edge %d", x, vol_env_editor.x + (x - mused.vol_env_horiz_scroll) * vol_zoom_mult, vol_env_editor.x, vol_env_editor.x + vol_env_editor.w);
				
				if (mouse_x >= rr.x && mouse_x <= rr.x + rr.w && mouse_y >= rr.y && mouse_y <= rr.y + rr.h && vol_env_editor.x + (x - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult >= vol_env_editor.x && vol_env_editor.x + (x - mused.fourop_vol_env_horiz_scroll) * vol_zoom_mult <= vol_env_editor.x + vol_env_editor.w)
				{
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].x = (mx - vol_env_editor.x) / vol_zoom_mult + mused.fourop_vol_env_horiz_scroll;
					
					clamp(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].x, 0, 0, MUS_MAX_ENVELOPE_POINT_X);
					clamp(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].y, (mused.fourop_prev_vol_env_y - my), 0, vol_env_editor.h);
					
					if(mused.fourop_vol_env_point > 0 && mused.fourop_vol_env_point < mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points - 1)
					{
						clamp(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].x, 0, mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point - 1].x + 1, mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point + 1].x - 1);
					}
					
					if(mused.fourop_vol_env_point == mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points - 1) //last dot
					{
						clamp(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].x, 0, mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point - 1].x + 1, MUS_MAX_ENVELOPE_POINT_X);
					}
				}
				
				if(mused.fourop_vol_env_point == 0)
				{
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].x = 0; //1st point always at 0
				}
			}
			
			mused.fourop_prev_vol_env_x = mx;
			mused.fourop_prev_vol_env_y = my;
		}
	}
	
	else
	{
		mused.fourop_prev_vol_env_x = -1;
		mused.fourop_prev_vol_env_y = -1;
	}
	
	bevelex(domain, &vol_env_editor_init, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	
	SDL_Rect mask_left;
	SDL_Rect mask_right; //hiding dots and lines drawn outside of the envelope editor
	copy_rect(&mask_left, &vol_env_editor_init);
	
	mask_left.w = 1;
	mask_left.x -= 1;
	
	gfx_rect(dest_surface, &mask_left, colors[COLOR_WAVETABLE_BACKGROUND]);
	
	SDL_Rect vol_env_params;
	copy_rect(&vol_env_params, &area);
	
	adjust_rect(&vol_env_params, 2);
	
	vol_env_params.y -= mused.fourop_point_env_editor_scroll;
	vol_env_params.y += TOP_VIEW_H - 4;
	vol_env_params.h = ENV_WINDOW_HEIGHT + 2 * ENV_ED_MARGIN + 10;
	vol_env_params.w = 200 - 2 * ENV_ED_MARGIN;
	vol_env_params.x += area.w - 200 - 4 + 2 * ENV_ED_MARGIN;
	
	bevelex(domain, &vol_env_params, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	
	copy_rect(&mask_right, &vol_env_params);
	
	mask_right.w = 10;
	mask_right.x += vol_env_params.w;
	
	gfx_rect(dest_surface, &mask_right, colors[COLOR_WAVETABLE_BACKGROUND]);
	
	adjust_rect(&vol_env_params, 4);
	
	console_set_clip(mused.console, &vol_env_params);
	
	copy_rect(&r, &vol_env_params);
	r.w = r.w / 2;
	r.h = 10;
	r.y -= 2;
	
	if ((button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "ADD", NULL, NULL, NULL, NULL) & 1) && event->type == SDL_MOUSEBUTTONDOWN && mused.fourop_vol_env_point != -1)
	{
		if(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points < MUS_MAX_ENVELOPE_POINTS)
		{
			if(mused.fourop_vol_env_point == mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points - 1 
			&& mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points - 1].x < MUS_MAX_ENVELOPE_POINT_X) //if last point
			{
				snapshot(S_T_INSTRUMENT);
				
				mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points].x = my_min(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points - 1].x + 10, MUS_MAX_ENVELOPE_POINT_X);
				mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points].y = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points - 1].y;
				
				mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points++;
			}
			
			else if(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points - 1].x < MUS_MAX_ENVELOPE_POINT_X) //add to the center of the line
			{
				snapshot(S_T_INSTRUMENT);
				
				for(int i = MUS_MAX_ENVELOPE_POINTS - 2; i >= mused.fourop_vol_env_point; --i)
				{
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i + 1].x = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i].x;
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i + 1].y = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i].y;
				}
				
				mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points++;
				
				mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point + 1].x = (mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].x + mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point + 2].x) / 2;
				mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point + 1].y = (mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point].y + mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[mused.fourop_vol_env_point + 2].y) / 2;
			}
		}
	}
	
	//update_rect(&vol_env_params, &r);
	r.x += r.w;
	
	if ((button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "DELETE", NULL, NULL, NULL, NULL) & 1) && event->type == SDL_MOUSEBUTTONDOWN && mused.fourop_vol_env_point != -1)
	{
		if(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points > 2)
		{
			if(mused.fourop_vol_env_point == mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points - 1) //if last point
			{
				snapshot(S_T_INSTRUMENT);
				
				mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points--;
				mused.fourop_vol_env_point--;
			}
			
			else
			{
				snapshot(S_T_INSTRUMENT);
				
				for(int i = mused.fourop_vol_env_point; i < mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points - 1; ++i)
				{
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i].x = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i + 1].x;
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i].y = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].volume_envelope[i + 1].y;
				}
				
				mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points--;
			}
			
			if(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_flags)
			{
				if(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_loop_start >= mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points)
				{
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_loop_start--;
				}
				
				if(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_loop_end >= mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points)
				{
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_loop_end--;
				}
				
				if(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_sustain >= mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points)
				{
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_sustain--;
				}
			}
		}
	}
	
	update_rect(&vol_env_params, &r);
	
	r.w = vol_env_params.w / 2 - 2;
	
	fourop_env_flags(event, &r, dest, FOUROP_ENV_ENABLE_VOLUME_ENVELOPE_SUSTAIN, "SUSTAIN", (Uint32*)&inst->ops[mused.selected_operator - 1].vol_env_flags, MUS_ENV_SUSTAIN);
	update_rect(&vol_env_params, &r);
	fourop_env_text(event, &r, dest, FOUROP_ENV_VOLUME_ENVELOPE_SUSTAIN_POINT, "POINT", "%01X", MAKEPTR(inst->ops[mused.selected_operator - 1].vol_env_sustain), 1);
	update_rect(&vol_env_params, &r);
	
	r.w = 45;
	
	fourop_env_flags(event, &r, dest, FOUROP_ENV_ENABLE_VOLUME_ENVELOPE_LOOP, "LOOP", (Uint32*)&inst->ops[mused.selected_operator - 1].vol_env_flags, MUS_ENV_LOOP);
	//update_rect(&vol_env_params, &r);
	
	r.x += r.w + 5;
	r.w = 66;
	
	fourop_env_text(event, &r, dest, FOUROP_ENV_VOLUME_ENVELOPE_LOOP_BEGIN, "BEGIN", "%01X", MAKEPTR(inst->ops[mused.selected_operator - 1].vol_env_loop_start), 1);
	//update_rect(&vol_env_params, &r);
	
	r.x += r.w + 5;
	r.w = 55;
	
	fourop_env_text(event, &r, dest, FOUROP_ENV_VOLUME_ENVELOPE_LOOP_END, "END", "%01X", MAKEPTR(inst->ops[mused.selected_operator - 1].vol_env_loop_end), 1);
	update_rect(&vol_env_params, &r);
	
	SDL_Rect vol_env_horiz_slider_rect;
	copy_rect(&vol_env_horiz_slider_rect, &vol_env_editor);
	
	vol_env_horiz_slider_rect.h = 10;
	vol_env_horiz_slider_rect.y += vol_env_editor.h + ENV_ED_MARGIN;
	vol_env_horiz_slider_rect.x -= ENV_ED_MARGIN;
	vol_env_horiz_slider_rect.w += 2 * ENV_ED_MARGIN;
	
	slider_set_params(&mused.fourop_vol_env_horiz_slider_param, 0, MUS_MAX_ENVELOPE_POINT_X, mused.fourop_vol_env_horiz_scroll, (mused.fourop_vol_env_horiz_scroll + (mused.fourop_vol_env_scale < 0 ? (vol_env_editor.w / 2) : (vol_env_editor.w * mused.fourop_vol_env_scale))), &mused.fourop_vol_env_horiz_scroll, 1, SLIDER_HORIZONTAL, mused.slider_bevel);
	slider(dest_surface, &vol_env_horiz_slider_rect, event, &mused.fourop_vol_env_horiz_slider_param);
	
	gfx_domain_set_clip(domain, NULL);
	
	adjust_rect(&area, -1);
	bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
}

void point_envelope_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	if(!(mused.show_point_envelope_editor)) return;
	if(mused.show_four_op_menu) return;
	
	MusInstrument *inst = &mused.song.instrument[mused.current_instrument];
	
	SDL_Rect area;
	copy_rect(&area, dest);
	
	if(!(mused.show_fm_settings))
	{
		area.y -= 60;
		area.h += 60;
	}
	
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	//bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 1);
	
	gfx_domain_set_clip(domain, &area);
	
	SDL_Rect vol_env_header;
	copy_rect(&vol_env_header, &area);
	
	adjust_rect(&vol_env_header, 2);
	
	vol_env_header.y -= mused.point_env_editor_scroll;
	vol_env_header.h = TOP_VIEW_H - 4;
	
	bevelex(domain, &vol_env_header, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	
	adjust_rect(&vol_env_header, 4);
	
	console_set_clip(mused.console, &vol_env_header);
	
	//font_write_args(&mused.largefont, domain, &area, "test\ntest\ntest\ntest\ntest\ntest\n");
	
	SDL_Rect r;
	
	copy_rect(&r, &vol_env_header);
	r.w = 150;
	r.h = 10;
	r.y -= 2;
	
	env_flags(event, &r, dest, ENV_ENABLE_VOLUME_ENVELOPE, "USE VOLUME ENVELOPE", &inst->flags, MUS_INST_USE_VOLUME_ENVELOPE);
	update_rect(&vol_env_header, &r);
	
	r.w = 95;
	r.x += 10;
	
	env_text(event, &r, dest, ENV_VOLUME_ENVELOPE_FADEOUT, "FADEOUT", "%03X", MAKEPTR(inst->vol_env_fadeout), 3);
	update_rect(&vol_env_header, &r);
	
	r.w = 95;
	
	float vol_zoom_mult = mused.vol_env_scale < 0 ? 2.0 : (1.0 / (float)mused.vol_env_scale);
	
	char fuck_string[10];
	
	sprintf(fuck_string, "%0.2fx", vol_zoom_mult);
	
	if(mused.vol_env_scale < 0)
	{
		env_text(event, &r, dest, ENV_VOLUME_ENVELOPE_SCALE, "ZOOM", "2x", MAKEPTR(vol_zoom_mult), 5);
	}
	
	else
	{
		env_text(event, &r, dest, ENV_VOLUME_ENVELOPE_SCALE, "ZOOM", fuck_string, (void*)&vol_zoom_mult, 5);
	}
	
	update_rect(&vol_env_header, &r);
	
	SDL_Rect vol_env_editor;
	copy_rect(&vol_env_editor, &area);
	
	adjust_rect(&vol_env_editor, 2);
	
	vol_env_editor.y -= mused.point_env_editor_scroll;
	vol_env_editor.y += TOP_VIEW_H - 4;
	vol_env_editor.h = ENV_WINDOW_HEIGHT + 2 * ENV_ED_MARGIN;
	vol_env_editor.w -= 200 - 2 * ENV_ED_MARGIN;
	
	SDL_Rect vol_env_editor_init;
	copy_rect(&vol_env_editor_init, &vol_env_editor);
	
	adjust_rect(&vol_env_editor, ENV_ED_MARGIN);
	
	SDL_Rect vol_env_info;
	
	vol_env_info.y = vol_env_editor.y - 4;
	vol_env_info.x = vol_env_editor.x + vol_env_editor.w - 4 * 9 - 4 - 4;
	vol_env_info.h = 8;
	vol_env_info.w = 4 * 12;
	
	if(mused.vol_env_point != -1)
	{
		font_write_args(&mused.tinyfont, dest_surface, &vol_env_info, "TIME:%0.2fs\n VOL:%02X", (float)mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].x / 100.0, mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].y);
	}

	
	//mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].x
	
	bool should_continue = true;
	
	for(int i = 0; (i < mused.song.num_channels) && should_continue; ++i)
	{
		if(mused.mus.channel[i].instrument != NULL)
		{
			if(mused.mus.channel[i].instrument == inst && (inst->flags & MUS_INST_USE_VOLUME_ENVELOPE))
			{
				if(mused.mus.cyd->channel[i].adsr.envelope > 0 && (mused.mus.cyd->channel[i].flags & CYD_CHN_ENABLE_GATE)) //if custom envelope is playing draw current position
				{
					should_continue = false;
					
					SDL_Rect current_position;
		
					copy_rect(&current_position, &vol_env_editor);
					
					current_position.w = 16;
					current_position.x = vol_env_editor.x + (((mused.mus.cyd->channel[i].adsr.envelope >> 16) - mused.vol_env_horiz_scroll) * vol_zoom_mult) - 7;
					current_position.y -= (ENV_ED_MARGIN - 2);
					current_position.h += 2 * (ENV_ED_MARGIN - 2);
					
					bevelex(domain, &current_position, mused.slider_bevel, BEV_ENV_CURRENT_ENV_POSITION, BEV_F_NORMAL);
					
					gfx_translucent_rect(domain, &current_position, colors[COLOR_WAVETABLE_BACKGROUND] | (Uint32)(255 - (mused.mus.cyd->channel[i].adsr.curr_vol_fadeout_value >> (31 - 8))) << 24);
				}
			}
		}
	}
	
	if(mused.song.instrument[mused.current_instrument].vol_env_flags & MUS_ENV_SUSTAIN) //draw sustain
	{
		SDL_Rect sustain_point;
		
		copy_rect(&sustain_point, &vol_env_editor);
		
		sustain_point.w = 16;
		sustain_point.x = vol_env_editor.x + (mused.song.instrument[mused.current_instrument].volume_envelope[mused.song.instrument[mused.current_instrument].vol_env_sustain].x - mused.vol_env_horiz_scroll) * vol_zoom_mult - 7;
		sustain_point.y -= (ENV_ED_MARGIN - 2);
		sustain_point.h += 2 * (ENV_ED_MARGIN - 2);
		
		bevelex(domain, &sustain_point, mused.slider_bevel, BEV_ENV_SUSTAIN_POINT, BEV_F_NORMAL);
	}
	
	if(mused.song.instrument[mused.current_instrument].vol_env_flags & MUS_ENV_LOOP) //draw loop begin and end
	{
		SDL_Rect loop_begin, loop_end;
		
		copy_rect(&loop_begin, &vol_env_editor);
		copy_rect(&loop_end, &vol_env_editor);
		
		loop_begin.w = 16;
		loop_begin.x = vol_env_editor.x + (mused.song.instrument[mused.current_instrument].volume_envelope[mused.song.instrument[mused.current_instrument].vol_env_loop_start].x - mused.vol_env_horiz_scroll) * vol_zoom_mult;
		loop_begin.y -= (ENV_ED_MARGIN - 2);
		loop_begin.h += 2 * (ENV_ED_MARGIN - 2);
		
		bevelex(domain, &loop_begin, mused.slider_bevel, BEV_ENV_LOOP_START, BEV_F_NORMAL);
		
		loop_end.w = 16;
		loop_end.x = vol_env_editor.x + (mused.song.instrument[mused.current_instrument].volume_envelope[mused.song.instrument[mused.current_instrument].vol_env_loop_end].x - mused.vol_env_horiz_scroll) * vol_zoom_mult - 15;
		loop_end.y -= (ENV_ED_MARGIN - 2);
		loop_end.h += 2 * (ENV_ED_MARGIN - 2);
		
		bevelex(domain, &loop_end, mused.slider_bevel, BEV_ENV_LOOP_END, BEV_F_NORMAL);
	}
	
	for(int i = 0; i < mused.song.instrument[mused.current_instrument].num_vol_points; ++i)
	{
		int x = mused.song.instrument[mused.current_instrument].volume_envelope[i].x;
		int y = mused.song.instrument[mused.current_instrument].volume_envelope[i].y;
		
		if(i > 0)
		{
			int prev_x = mused.song.instrument[mused.current_instrument].volume_envelope[i - 1].x;
			int prev_y = mused.song.instrument[mused.current_instrument].volume_envelope[i - 1].y;
			
			gfx_line(domain, vol_env_editor.x + (prev_x - mused.vol_env_horiz_scroll) * vol_zoom_mult, vol_env_editor.y + (ENV_WINDOW_HEIGHT - prev_y) - 1, vol_env_editor.x + (x - mused.vol_env_horiz_scroll) * vol_zoom_mult, vol_env_editor.y + (ENV_WINDOW_HEIGHT - y) - 1, colors[COLOR_WAVETABLE_SAMPLE]);
		}
	}
	
	for(int i = 0; i < mused.song.instrument[mused.current_instrument].num_vol_points; ++i)
	{
		int x = mused.song.instrument[mused.current_instrument].volume_envelope[i].x;
		int y = mused.song.instrument[mused.current_instrument].volume_envelope[i].y;

		SDL_Rect rr = { vol_env_editor.x + (x - mused.vol_env_horiz_scroll) * vol_zoom_mult - mused.smallfont.w / 2,
			vol_env_editor.y + (ENV_WINDOW_HEIGHT - y) - mused.smallfont.h / 2, mused.smallfont.w, mused.smallfont.h };
		
		font_write(&mused.smallfont, dest_surface, &rr, "\2");

		if (i == mused.vol_env_point)
		{
			SDL_Rect sel;
			copy_rect(&sel, &rr);
			adjust_rect(&sel, -4);
			bevelex(domain, &sel, mused.slider_bevel, BEV_CURSOR, BEV_F_STRETCH_ALL);
		}

		if (event->type == SDL_MOUSEBUTTONDOWN)
		{
			if (check_event(event, &rr, NULL, 0, 0, 0) && vol_env_editor.x + (x - mused.vol_env_horiz_scroll) * vol_zoom_mult >= vol_env_editor.x && vol_env_editor.x + (x - mused.vol_env_horiz_scroll) * vol_zoom_mult <= vol_env_editor.x + vol_env_editor.w)
			{
				mused.vol_env_point = i;
			}
		}
	}
	
	int mx, my;
	
	Uint32 buttons = SDL_GetMouseState(&mx, &my);
	
	mx /= mused.pixel_scale;
	my /= mused.pixel_scale;
	
	if (mused.focus == EDITENVELOPE)
	{
		if (mx >= vol_env_editor.x - 5 && mx <= vol_env_editor.x + vol_env_editor.w + 5 && my >= vol_env_editor.y - 5 && my <= vol_env_editor.y + vol_env_editor.h + 5)
		{
			if (event->type == SDL_MOUSEBUTTONDOWN)
			{
				snapshot(S_T_INSTRUMENT);
			}
		}
	}
	
	if (mused.focus == EDITENVELOPE && (buttons & SDL_BUTTON(1)))
	{
		if (mx >= vol_env_editor.x - 5 && mx <= vol_env_editor.x + vol_env_editor.w + 5 && my >= vol_env_editor.y - 5 && my <= vol_env_editor.y + vol_env_editor.h + 5)
		{
			if (mused.prev_vol_env_x != -1)
			{
				int x = mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].x;
				int y = mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].y;

				SDL_Rect rr = { vol_env_editor.x + (x - mused.vol_env_horiz_scroll) * vol_zoom_mult - mused.smallfont.w / 2,
					vol_env_editor.y + (ENV_WINDOW_HEIGHT - y) - mused.smallfont.h / 2 - 3, mused.smallfont.w, mused.smallfont.h };
					
				adjust_rect(&rr, -4);
				
				int mouse_x = (mused.prev_vol_env_x == -1) ? mx : mused.prev_vol_env_x;
				int mouse_y = (mused.prev_vol_env_y == -1) ? my : mused.prev_vol_env_y;
				
				//debug("self dot x %d, dot x %d left edge %d right edge %d", x, vol_env_editor.x + (x - mused.vol_env_horiz_scroll) * vol_zoom_mult, vol_env_editor.x, vol_env_editor.x + vol_env_editor.w);
				
				if (mouse_x >= rr.x && mouse_x <= rr.x + rr.w && mouse_y >= rr.y && mouse_y <= rr.y + rr.h && vol_env_editor.x + (x - mused.vol_env_horiz_scroll) * vol_zoom_mult >= vol_env_editor.x && vol_env_editor.x + (x - mused.vol_env_horiz_scroll) * vol_zoom_mult <= vol_env_editor.x + vol_env_editor.w)
				{
					mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].x = (mx - vol_env_editor.x) / vol_zoom_mult + mused.vol_env_horiz_scroll;
					
					clamp(mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].x, 0, 0, MUS_MAX_ENVELOPE_POINT_X);
					clamp(mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].y, (mused.prev_vol_env_y - my), 0, vol_env_editor.h);
					
					if(mused.vol_env_point > 0 && mused.vol_env_point < mused.song.instrument[mused.current_instrument].num_vol_points - 1)
					{
						clamp(mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].x, 0, mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point - 1].x + 1, mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point + 1].x - 1);
					}
					
					if(mused.vol_env_point == mused.song.instrument[mused.current_instrument].num_vol_points - 1) //last dot
					{
						clamp(mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].x, 0, mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point - 1].x + 1, MUS_MAX_ENVELOPE_POINT_X);
					}
				}
				
				if(mused.vol_env_point == 0)
				{
					mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].x = 0; //1st point always at 0
				}
			}
			
			mused.prev_vol_env_x = mx;
			mused.prev_vol_env_y = my;
		}
	}
	
	else
	{
		mused.prev_vol_env_x = -1;
		mused.prev_vol_env_y = -1;
	}
	
	bevelex(domain, &vol_env_editor_init, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	
	SDL_Rect mask_left;
	SDL_Rect mask_right; //hiding dots and lines drawn outside of the envelope editor
	copy_rect(&mask_left, &vol_env_editor_init);
	
	mask_left.w = 1;
	mask_left.x -= 1;
	
	gfx_rect(dest_surface, &mask_left, colors[COLOR_WAVETABLE_BACKGROUND]);
	
	SDL_Rect vol_env_params;
	copy_rect(&vol_env_params, &area);
	
	adjust_rect(&vol_env_params, 2);
	
	vol_env_params.y -= mused.point_env_editor_scroll;
	vol_env_params.y += TOP_VIEW_H - 4;
	vol_env_params.h = ENV_WINDOW_HEIGHT + 2 * ENV_ED_MARGIN + 10;
	vol_env_params.w = 200 - 2 * ENV_ED_MARGIN;
	vol_env_params.x += area.w - 200 - 4 + 2 * ENV_ED_MARGIN;
	
	bevelex(domain, &vol_env_params, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	
	copy_rect(&mask_right, &vol_env_params);
	
	mask_right.w = 10;
	mask_right.x += vol_env_params.w;
	
	gfx_rect(dest_surface, &mask_right, colors[COLOR_WAVETABLE_BACKGROUND]);
	
	adjust_rect(&vol_env_params, 4);
	
	console_set_clip(mused.console, &vol_env_params);
	
	copy_rect(&r, &vol_env_params);
	r.w = r.w / 2;
	r.h = 10;
	r.y -= 2;
	
	if ((button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "ADD", NULL, NULL, NULL, NULL) & 1) && event->type == SDL_MOUSEBUTTONDOWN && mused.vol_env_point != -1)
	{
		if(mused.song.instrument[mused.current_instrument].num_vol_points < MUS_MAX_ENVELOPE_POINTS)
		{
			if(mused.vol_env_point == mused.song.instrument[mused.current_instrument].num_vol_points - 1 
			&& mused.song.instrument[mused.current_instrument].volume_envelope[mused.song.instrument[mused.current_instrument].num_vol_points - 1].x < MUS_MAX_ENVELOPE_POINT_X) //if last point
			{
				snapshot(S_T_INSTRUMENT);
				
				mused.song.instrument[mused.current_instrument].volume_envelope[mused.song.instrument[mused.current_instrument].num_vol_points].x = my_min(mused.song.instrument[mused.current_instrument].volume_envelope[mused.song.instrument[mused.current_instrument].num_vol_points - 1].x + 10, MUS_MAX_ENVELOPE_POINT_X);
				mused.song.instrument[mused.current_instrument].volume_envelope[mused.song.instrument[mused.current_instrument].num_vol_points].y = mused.song.instrument[mused.current_instrument].volume_envelope[mused.song.instrument[mused.current_instrument].num_vol_points - 1].y;
				
				mused.song.instrument[mused.current_instrument].num_vol_points++;
			}
			
			else if(mused.song.instrument[mused.current_instrument].volume_envelope[mused.song.instrument[mused.current_instrument].num_vol_points - 1].x < MUS_MAX_ENVELOPE_POINT_X) //add to the center of the line
			{
				snapshot(S_T_INSTRUMENT);
				
				for(int i = MUS_MAX_ENVELOPE_POINTS - 2; i >= mused.vol_env_point; --i)
				{
					mused.song.instrument[mused.current_instrument].volume_envelope[i + 1].x = mused.song.instrument[mused.current_instrument].volume_envelope[i].x;
					mused.song.instrument[mused.current_instrument].volume_envelope[i + 1].y = mused.song.instrument[mused.current_instrument].volume_envelope[i].y;
				}
				
				mused.song.instrument[mused.current_instrument].num_vol_points++;
				
				mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point + 1].x = (mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].x + mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point + 2].x) / 2;
				mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point + 1].y = (mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point].y + mused.song.instrument[mused.current_instrument].volume_envelope[mused.vol_env_point + 2].y) / 2;
			}
		}
	}
	
	//update_rect(&vol_env_params, &r);
	r.x += r.w;
	
	if ((button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "DELETE", NULL, NULL, NULL, NULL) & 1) && event->type == SDL_MOUSEBUTTONDOWN && mused.vol_env_point != -1)
	{
		if(mused.song.instrument[mused.current_instrument].num_vol_points > 2)
		{
			if(mused.vol_env_point == mused.song.instrument[mused.current_instrument].num_vol_points - 1) //if last point
			{
				snapshot(S_T_INSTRUMENT);
				
				mused.song.instrument[mused.current_instrument].num_vol_points--;
				mused.vol_env_point--;
			}
			
			else
			{
				snapshot(S_T_INSTRUMENT);
				
				for(int i = mused.vol_env_point; i < mused.song.instrument[mused.current_instrument].num_vol_points - 1; ++i)
				{
					mused.song.instrument[mused.current_instrument].volume_envelope[i].x = mused.song.instrument[mused.current_instrument].volume_envelope[i + 1].x;
					mused.song.instrument[mused.current_instrument].volume_envelope[i].y = mused.song.instrument[mused.current_instrument].volume_envelope[i + 1].y;
				}
				
				mused.song.instrument[mused.current_instrument].num_vol_points--;
			}
			
			if(mused.song.instrument[mused.current_instrument].vol_env_flags)
			{
				if(mused.song.instrument[mused.current_instrument].vol_env_loop_start >= mused.song.instrument[mused.current_instrument].num_vol_points)
				{
					mused.song.instrument[mused.current_instrument].vol_env_loop_start--;
				}
				
				if(mused.song.instrument[mused.current_instrument].vol_env_loop_end >= mused.song.instrument[mused.current_instrument].num_vol_points)
				{
					mused.song.instrument[mused.current_instrument].vol_env_loop_end--;
				}
				
				if(mused.song.instrument[mused.current_instrument].vol_env_sustain >= mused.song.instrument[mused.current_instrument].num_vol_points)
				{
					mused.song.instrument[mused.current_instrument].vol_env_sustain--;
				}
			}
		}
	}
	
	update_rect(&vol_env_params, &r);
	
	r.w = vol_env_params.w / 2 - 2;
	
	env_flags(event, &r, dest, ENV_ENABLE_VOLUME_ENVELOPE_SUSTAIN, "SUSTAIN", (Uint32*)&inst->vol_env_flags, MUS_ENV_SUSTAIN);
	update_rect(&vol_env_params, &r);
	env_text(event, &r, dest, ENV_VOLUME_ENVELOPE_SUSTAIN_POINT, "POINT", "%01X", MAKEPTR(inst->vol_env_sustain), 1);
	update_rect(&vol_env_params, &r);
	
	r.w = 45;
	
	env_flags(event, &r, dest, ENV_ENABLE_VOLUME_ENVELOPE_LOOP, "LOOP", (Uint32*)&inst->vol_env_flags, MUS_ENV_LOOP);
	//update_rect(&vol_env_params, &r);
	
	r.x += r.w + 5;
	r.w = 66;
	
	env_text(event, &r, dest, ENV_VOLUME_ENVELOPE_LOOP_BEGIN, "BEGIN", "%01X", MAKEPTR(inst->vol_env_loop_start), 1);
	//update_rect(&vol_env_params, &r);
	
	r.x += r.w + 5;
	r.w = 55;
	
	env_text(event, &r, dest, ENV_VOLUME_ENVELOPE_LOOP_END, "END", "%01X", MAKEPTR(inst->vol_env_loop_end), 1);
	update_rect(&vol_env_params, &r);
	
	SDL_Rect vol_env_horiz_slider_rect;
	copy_rect(&vol_env_horiz_slider_rect, &vol_env_editor);
	
	vol_env_horiz_slider_rect.h = 10;
	vol_env_horiz_slider_rect.y += vol_env_editor.h + ENV_ED_MARGIN;
	vol_env_horiz_slider_rect.x -= ENV_ED_MARGIN;
	vol_env_horiz_slider_rect.w += 2 * ENV_ED_MARGIN;
	
	slider_set_params(&mused.vol_env_horiz_slider_param, 0, MUS_MAX_ENVELOPE_POINT_X, mused.vol_env_horiz_scroll, (mused.vol_env_horiz_scroll + (mused.vol_env_scale < 0 ? (vol_env_editor.w / 2) : (vol_env_editor.w * mused.vol_env_scale))), &mused.vol_env_horiz_scroll, 1, SLIDER_HORIZONTAL, mused.slider_bevel);
	slider(dest_surface, &vol_env_horiz_slider_rect, event, &mused.vol_env_horiz_slider_param);
	
	// ====== panning envelope
	
	SDL_Rect pan_env_header;
	copy_rect(&pan_env_header, &vol_env_header);
	
	adjust_rect(&pan_env_header, 2);
	
	//pan_env_header.y -= mused.point_env_editor_scroll;
	
	pan_env_header.y += vol_env_params.h + vol_env_header.h + 11;
	pan_env_header.x -= 6;
	pan_env_header.w += 12;
	pan_env_header.h = TOP_VIEW_H - 4;
	
	bevelex(domain, &pan_env_header, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	
	adjust_rect(&pan_env_header, 4);
	
	console_set_clip(mused.console, &pan_env_header);
	
	//font_write_args(&mused.largefont, domain, &area, "test\ntest\ntest\ntest\ntest\ntest\n");
	
	//SDL_Rect r;
	
	copy_rect(&r, &pan_env_header);
	r.w = 160;
	r.h = 10;
	r.y -= 2;
	
	env_flags(event, &r, dest, ENV_ENABLE_PANNING_ENVELOPE, "USE PANNING ENVELOPE", &inst->flags, MUS_INST_USE_PANNING_ENVELOPE);
	update_rect(&pan_env_header, &r);
	
	r.w = 95;
	r.x += 10;
	
	env_text(event, &r, dest, ENV_PANNING_ENVELOPE_FADEOUT, "FADEOUT", "%03X", MAKEPTR(inst->pan_env_fadeout), 3);
	update_rect(&pan_env_header, &r);
	
	r.w = 95;
	
	float pan_zoom_mult = mused.pan_env_scale < 0 ? 2.0 : (1.0 / (float)mused.pan_env_scale);
	
	//char fuck_string[10];
	
	sprintf(fuck_string, "%0.2fx", pan_zoom_mult);
	
	if(mused.pan_env_scale < 0)
	{
		env_text(event, &r, dest, ENV_PANNING_ENVELOPE_SCALE, "ZOOM", "2x", MAKEPTR(pan_zoom_mult), 5);
	}
	
	else
	{
		env_text(event, &r, dest, ENV_PANNING_ENVELOPE_SCALE, "ZOOM", fuck_string, (void*)&pan_zoom_mult, 5);
	}
	
	update_rect(&pan_env_header, &r);
	
	SDL_Rect pan_env_editor;
	copy_rect(&pan_env_editor, &area);
	
	adjust_rect(&pan_env_editor, 2);
	
	pan_env_editor.y = pan_env_header.y + pan_env_header.h + 4;
	//pan_env_editor.y += TOP_VIEW_H - 4 + vol_env_editor_init.h;
	pan_env_editor.h = ENV_WINDOW_HEIGHT + 2 * ENV_ED_MARGIN;
	pan_env_editor.w -= 200 - 2 * ENV_ED_MARGIN;
	
	SDL_Rect pan_env_editor_init;
	copy_rect(&pan_env_editor_init, &pan_env_editor);
	
	adjust_rect(&pan_env_editor, ENV_ED_MARGIN);
	
	SDL_Rect pan_env_info;
	
	pan_env_info.y = pan_env_editor.y - 4;
	pan_env_info.x = pan_env_editor.x + pan_env_editor.w - 4 * 9 - 4 - 4;
	pan_env_info.h = 8;
	pan_env_info.w = 4 * 12;
	
	if(mused.pan_env_point != -1)
	{
		if(mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].y == 0x40) //center
		{
			font_write_args(&mused.tinyfont, dest_surface, &pan_env_info, "TIME:%0.2fs\n PAN:\xfa\xf9", (float)mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].x / 100.0);
		}
		
		if(mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].y < 0x40) //left
		{
			font_write_args(&mused.tinyfont, dest_surface, &pan_env_info, "TIME:%0.2fs\n PAN:\xf9%02X", (float)mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].x / 100.0, 0x40 - mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].y);
		}
		
		if(mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].y > 0x40) //right
		{
			font_write_args(&mused.tinyfont, dest_surface, &pan_env_info, "TIME:%0.2fs\n PAN:%02X\xfa", (float)mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].x / 100.0, mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].y - 0x40);
		}
	}
	
	SDL_Rect panning_stuff;
	
	panning_stuff.x = pan_env_editor.x - 5;
	panning_stuff.y = pan_env_editor.y - 5;
	panning_stuff.w = panning_stuff.h = 8;
	
	font_write_args(&mused.largefont, dest_surface, &panning_stuff, "R");
	
	panning_stuff.y += pan_env_editor.h + 3;
	
	font_write_args(&mused.largefont, dest_surface, &panning_stuff, "L");
	
	panning_stuff.y = pan_env_editor.y + pan_env_editor.h / 2 - 1;
	panning_stuff.w = pan_env_editor.w + 2 * ENV_ED_MARGIN + 3;
	panning_stuff.x -= ENV_ED_MARGIN;
	panning_stuff.h = 1;
	
	Uint32 color = (colors[COLOR_WAVETABLE_SAMPLE] & 255) / 3 + ((((colors[COLOR_WAVETABLE_SAMPLE] >> 8) & 255) / 3) << 8) + ((((colors[COLOR_WAVETABLE_SAMPLE] >> 16) & 255) / 3) << 16);
	
	gfx_rect(domain, &panning_stuff, color);
	
	should_continue = true;
	
	for(int i = 0; (i < mused.song.num_channels) && should_continue; ++i)
	{
		if(mused.mus.channel[i].instrument != NULL)
		{
			if(mused.mus.channel[i].instrument == inst && (inst->flags & MUS_INST_USE_PANNING_ENVELOPE))
			{
				if(mused.mus.cyd->channel[i].adsr.pan_envelope > 0 && (mused.mus.cyd->channel[i].flags & CYD_CHN_ENABLE_GATE)) //if custom envelope is playing draw current position
				{
					should_continue = false;
					
					SDL_Rect current_position;
		
					copy_rect(&current_position, &pan_env_editor);
					
					current_position.w = 16;
					current_position.x = pan_env_editor.x + (((mused.mus.cyd->channel[i].adsr.pan_envelope >> 16) - mused.pan_env_horiz_scroll) * pan_zoom_mult) - 7;
					current_position.y -= (ENV_ED_MARGIN - 2);
					current_position.h += 2 * (ENV_ED_MARGIN - 2);
					
					bevelex(domain, &current_position, mused.slider_bevel, BEV_ENV_CURRENT_ENV_POSITION, BEV_F_NORMAL);
					
					gfx_translucent_rect(domain, &current_position, colors[COLOR_WAVETABLE_BACKGROUND] | (Uint32)(255 - (mused.mus.cyd->channel[i].adsr.curr_pan_fadeout_value >> (31 - 8))) << 24);
				}
			}
		}
	}
	
	if(mused.song.instrument[mused.current_instrument].pan_env_flags & MUS_ENV_SUSTAIN) //draw sustain
	{
		SDL_Rect sustain_point;
		
		copy_rect(&sustain_point, &pan_env_editor);
		
		sustain_point.w = 16;
		sustain_point.x = pan_env_editor.x + (mused.song.instrument[mused.current_instrument].panning_envelope[mused.song.instrument[mused.current_instrument].pan_env_sustain].x - mused.pan_env_horiz_scroll) * pan_zoom_mult - 7;
		sustain_point.y -= (ENV_ED_MARGIN - 2);
		sustain_point.h += 2 * (ENV_ED_MARGIN - 2);
		
		bevelex(domain, &sustain_point, mused.slider_bevel, BEV_ENV_SUSTAIN_POINT, BEV_F_NORMAL);
	}
	
	if(mused.song.instrument[mused.current_instrument].pan_env_flags & MUS_ENV_LOOP) //draw loop begin and end
	{
		SDL_Rect loop_begin, loop_end;
		
		copy_rect(&loop_begin, &pan_env_editor);
		copy_rect(&loop_end, &pan_env_editor);
		
		loop_begin.w = 16;
		loop_begin.x = pan_env_editor.x + (mused.song.instrument[mused.current_instrument].panning_envelope[mused.song.instrument[mused.current_instrument].pan_env_loop_start].x - mused.pan_env_horiz_scroll) * pan_zoom_mult;
		loop_begin.y -= (ENV_ED_MARGIN - 2);
		loop_begin.h += 2 * (ENV_ED_MARGIN - 2);
		
		bevelex(domain, &loop_begin, mused.slider_bevel, BEV_ENV_LOOP_START, BEV_F_NORMAL);
		
		loop_end.w = 16;
		loop_end.x = pan_env_editor.x + (mused.song.instrument[mused.current_instrument].panning_envelope[mused.song.instrument[mused.current_instrument].pan_env_loop_end].x - mused.pan_env_horiz_scroll) * pan_zoom_mult - 15;
		loop_end.y -= (ENV_ED_MARGIN - 2);
		loop_end.h += 2 * (ENV_ED_MARGIN - 2);
		
		bevelex(domain, &loop_end, mused.slider_bevel, BEV_ENV_LOOP_END, BEV_F_NORMAL);
	}
	
	for(int i = 0; i < mused.song.instrument[mused.current_instrument].num_pan_points; ++i)
	{
		int x = mused.song.instrument[mused.current_instrument].panning_envelope[i].x;
		int y = mused.song.instrument[mused.current_instrument].panning_envelope[i].y;
		
		if(i > 0)
		{
			int prev_x = mused.song.instrument[mused.current_instrument].panning_envelope[i - 1].x;
			int prev_y = mused.song.instrument[mused.current_instrument].panning_envelope[i - 1].y;
			
			gfx_line(domain, pan_env_editor.x + (prev_x - mused.pan_env_horiz_scroll) * pan_zoom_mult, pan_env_editor.y + (ENV_WINDOW_HEIGHT - prev_y) - 1, pan_env_editor.x + (x - mused.pan_env_horiz_scroll) * pan_zoom_mult, pan_env_editor.y + (ENV_WINDOW_HEIGHT - y) - 1, colors[COLOR_WAVETABLE_SAMPLE]);
		}
	}
	
	for(int i = 0; i < mused.song.instrument[mused.current_instrument].num_pan_points; ++i)
	{
		int x = mused.song.instrument[mused.current_instrument].panning_envelope[i].x;
		int y = mused.song.instrument[mused.current_instrument].panning_envelope[i].y;

		SDL_Rect rr = { pan_env_editor.x + (x - mused.pan_env_horiz_scroll) * pan_zoom_mult - mused.smallfont.w / 2,
			pan_env_editor.y + (ENV_WINDOW_HEIGHT - y) - mused.smallfont.h / 2, mused.smallfont.w, mused.smallfont.h };
		
		font_write(&mused.smallfont, dest_surface, &rr, "\2");

		if (i == mused.pan_env_point)
		{
			SDL_Rect sel;
			copy_rect(&sel, &rr);
			adjust_rect(&sel, -4);
			bevelex(domain, &sel, mused.slider_bevel, BEV_CURSOR, BEV_F_STRETCH_ALL);
		}

		if (event->type == SDL_MOUSEBUTTONDOWN)
		{
			if (check_event(event, &rr, NULL, 0, 0, 0) && pan_env_editor.x + (x - mused.pan_env_horiz_scroll) * pan_zoom_mult >= pan_env_editor.x && pan_env_editor.x + (x - mused.pan_env_horiz_scroll) * pan_zoom_mult <= pan_env_editor.x + pan_env_editor.w)
			{
				mused.pan_env_point = i;
			}
		}
	}
	
	//int mx, my;
	
	//buttons = SDL_GetMouseState(&mx, &my);
	
	//mx /= mused.pixel_scale;
	//my /= mused.pixel_scale;
	
	if (mused.focus == EDITENVELOPE)
	{
		if (mx >= pan_env_editor.x - 5 && mx <= pan_env_editor.x + pan_env_editor.w + 5 && my >= pan_env_editor.y - 5 && my <= pan_env_editor.y + pan_env_editor.h + 5)
		{
			if (event->type == SDL_MOUSEBUTTONDOWN)
			{
				snapshot(S_T_INSTRUMENT);
			}
		}
	}
	
	if (mused.focus == EDITENVELOPE && (buttons & SDL_BUTTON(1)))
	{
		if (mx >= pan_env_editor.x - 5 && mx <= pan_env_editor.x + pan_env_editor.w + 5 && my >= pan_env_editor.y - 5 && my <= pan_env_editor.y + pan_env_editor.h + 5)
		{
			if (mused.prev_pan_env_x != -1)
			{
				if (event->type == SDL_MOUSEBUTTONUP)
				{
					snapshot(S_T_INSTRUMENT);
				}
				
				int x = mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].x;
				int y = mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].y;

				SDL_Rect rr = { pan_env_editor.x + (x - mused.pan_env_horiz_scroll) * pan_zoom_mult - mused.smallfont.w / 2,
					pan_env_editor.y + (ENV_WINDOW_HEIGHT - y) - mused.smallfont.h / 2 - 3, mused.smallfont.w, mused.smallfont.h };
					
				adjust_rect(&rr, -4);
				
				int mouse_x = (mused.prev_pan_env_x == -1) ? mx : mused.prev_pan_env_x;
				int mouse_y = (mused.prev_pan_env_y == -1) ? my : mused.prev_pan_env_y;
				
				//debug("self dot x %d, dot x %d left edge %d right edge %d", x, vol_env_editor.x + (x - mused.vol_env_horiz_scroll) * vol_zoom_mult, vol_env_editor.x, vol_env_editor.x + vol_env_editor.w);
				
				if (mouse_x >= rr.x && mouse_x <= rr.x + rr.w && mouse_y >= rr.y && mouse_y <= rr.y + rr.h && pan_env_editor.x + (x - mused.pan_env_horiz_scroll) * pan_zoom_mult >= pan_env_editor.x && pan_env_editor.x + (x - mused.pan_env_horiz_scroll) * pan_zoom_mult <= pan_env_editor.x + pan_env_editor.w)
				{
					mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].x = (mx - pan_env_editor.x) / pan_zoom_mult + mused.pan_env_horiz_scroll;
					
					clamp(mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].x, 0, 0, MUS_MAX_ENVELOPE_POINT_X);
					clamp(mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].y, (mused.prev_pan_env_y - my), 0, pan_env_editor.h);
					
					if(mused.pan_env_point > 0 && mused.pan_env_point < mused.song.instrument[mused.current_instrument].num_pan_points - 1)
					{
						clamp(mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].x, 0, mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point - 1].x + 1, mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point + 1].x - 1);
					}
					
					if(mused.pan_env_point == mused.song.instrument[mused.current_instrument].num_pan_points - 1) //last dot
					{
						clamp(mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].x, 0, mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point - 1].x + 1, MUS_MAX_ENVELOPE_POINT_X);
					}
				}
				
				if(mused.pan_env_point == 0)
				{
					mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].x = 0; //1st point always at 0
				}
			}

			mused.prev_pan_env_x = mx;
			mused.prev_pan_env_y = my;
		}
	}
	
	else
	{
		mused.prev_pan_env_x = -1;
		mused.prev_pan_env_y = -1;
	}
	
	bevelex(domain, &pan_env_editor_init, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	
	//SDL_Rect mask_left;
	//SDL_Rect mask_right; //hiding dots and lines drawn outside of the envelope editor
	copy_rect(&mask_left, &pan_env_editor_init);
	
	mask_left.w = 1;
	mask_left.x -= 1;
	
	gfx_rect(dest_surface, &mask_left, colors[COLOR_WAVETABLE_BACKGROUND]);
	
	
	SDL_Rect pan_env_params;
	copy_rect(&pan_env_params, &area);
	
	adjust_rect(&pan_env_params, 2);
	
	pan_env_params.y = pan_env_editor_init.y;
	//pan_env_params.y += TOP_VIEW_H - 4;
	pan_env_params.h = ENV_WINDOW_HEIGHT + 2 * ENV_ED_MARGIN + 10;
	pan_env_params.w = 200 - 2 * ENV_ED_MARGIN;
	pan_env_params.x += area.w - 200 - 4 + 2 * ENV_ED_MARGIN;
	
	bevelex(domain, &pan_env_params, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	
	copy_rect(&mask_right, &pan_env_params);
	
	mask_right.w = 10;
	mask_right.x += pan_env_params.w;
	
	gfx_rect(dest_surface, &mask_right, colors[COLOR_WAVETABLE_BACKGROUND]);
	
	adjust_rect(&pan_env_params, 4);
	
	console_set_clip(mused.console, &vol_env_params);
	
	copy_rect(&r, &pan_env_params);
	r.w = r.w / 2;
	r.h = 10;
	r.y -= 2;
	
	if ((button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "ADD", NULL, NULL, NULL, NULL) & 1) && event->type == SDL_MOUSEBUTTONDOWN && mused.pan_env_point != -1)
	{
		if(mused.song.instrument[mused.current_instrument].num_pan_points < MUS_MAX_ENVELOPE_POINTS)
		{
			if(mused.pan_env_point == mused.song.instrument[mused.current_instrument].num_pan_points - 1 
			&& mused.song.instrument[mused.current_instrument].panning_envelope[mused.song.instrument[mused.current_instrument].num_pan_points - 1].x < MUS_MAX_ENVELOPE_POINT_X) //if last point
			{
				snapshot(S_T_INSTRUMENT);
				
				mused.song.instrument[mused.current_instrument].panning_envelope[mused.song.instrument[mused.current_instrument].num_pan_points].x = my_min(mused.song.instrument[mused.current_instrument].panning_envelope[mused.song.instrument[mused.current_instrument].num_pan_points - 1].x + 10, MUS_MAX_ENVELOPE_POINT_X);
				mused.song.instrument[mused.current_instrument].panning_envelope[mused.song.instrument[mused.current_instrument].num_pan_points].y = mused.song.instrument[mused.current_instrument].panning_envelope[mused.song.instrument[mused.current_instrument].num_pan_points - 1].y;
				
				mused.song.instrument[mused.current_instrument].num_pan_points++;
			}
			
			else if(mused.song.instrument[mused.current_instrument].panning_envelope[mused.song.instrument[mused.current_instrument].num_pan_points - 1].x < MUS_MAX_ENVELOPE_POINT_X) //add to the center of the line
			{
				snapshot(S_T_INSTRUMENT);
				
				for(int i = MUS_MAX_ENVELOPE_POINTS - 2; i >= mused.pan_env_point; --i)
				{
					mused.song.instrument[mused.current_instrument].panning_envelope[i + 1].x = mused.song.instrument[mused.current_instrument].panning_envelope[i].x;
					mused.song.instrument[mused.current_instrument].panning_envelope[i + 1].y = mused.song.instrument[mused.current_instrument].panning_envelope[i].y;
				}
				
				mused.song.instrument[mused.current_instrument].num_pan_points++;
				
				mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point + 1].x = (mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].x + mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point + 2].x) / 2;
				mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point + 1].y = (mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point].y + mused.song.instrument[mused.current_instrument].panning_envelope[mused.pan_env_point + 2].y) / 2;
			}
		}
	}
	
	//update_rect(&pan_env_params, &r);
	r.x += r.w;
	
	if ((button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "DELETE", NULL, NULL, NULL, NULL) & 1) && event->type == SDL_MOUSEBUTTONDOWN && mused.pan_env_point != -1)
	{
		if(mused.song.instrument[mused.current_instrument].num_pan_points > 2)
		{
			if(mused.pan_env_point == mused.song.instrument[mused.current_instrument].num_pan_points - 1) //if last point
			{
				snapshot(S_T_INSTRUMENT);
				
				mused.song.instrument[mused.current_instrument].num_pan_points--;
				mused.pan_env_point--;
			}
			
			else
			{
				snapshot(S_T_INSTRUMENT);
				
				for(int i = mused.pan_env_point; i < mused.song.instrument[mused.current_instrument].num_pan_points - 1; ++i)
				{
					mused.song.instrument[mused.current_instrument].panning_envelope[i].x = mused.song.instrument[mused.current_instrument].panning_envelope[i + 1].x;
					mused.song.instrument[mused.current_instrument].panning_envelope[i].y = mused.song.instrument[mused.current_instrument].panning_envelope[i + 1].y;
				}
				
				mused.song.instrument[mused.current_instrument].num_pan_points--;
			}
			
			if(mused.song.instrument[mused.current_instrument].pan_env_flags)
			{
				if(mused.song.instrument[mused.current_instrument].pan_env_loop_start >= mused.song.instrument[mused.current_instrument].num_pan_points)
				{
					mused.song.instrument[mused.current_instrument].pan_env_loop_start--;
				}
				
				if(mused.song.instrument[mused.current_instrument].pan_env_loop_end >= mused.song.instrument[mused.current_instrument].num_pan_points)
				{
					mused.song.instrument[mused.current_instrument].pan_env_loop_end--;
				}
				
				if(mused.song.instrument[mused.current_instrument].pan_env_sustain >= mused.song.instrument[mused.current_instrument].num_pan_points)
				{
					mused.song.instrument[mused.current_instrument].pan_env_sustain--;
				}
			}
		}
	}
	
	update_rect(&pan_env_params, &r);
	
	r.w = pan_env_params.w / 2 - 2;
	
	env_flags(event, &r, dest, ENV_ENABLE_PANNING_ENVELOPE_SUSTAIN, "SUSTAIN", (Uint32*)&inst->pan_env_flags, MUS_ENV_SUSTAIN);
	update_rect(&pan_env_params, &r);
	env_text(event, &r, dest, ENV_PANNING_ENVELOPE_SUSTAIN_POINT, "POINT", "%01X", MAKEPTR(inst->pan_env_sustain), 1);
	update_rect(&pan_env_params, &r);
	
	r.w = 45;
	
	env_flags(event, &r, dest, ENV_ENABLE_PANNING_ENVELOPE_LOOP, "LOOP", (Uint32*)&inst->pan_env_flags, MUS_ENV_LOOP);
	//update_rect(&pan_env_params, &r);
	
	r.x += r.w + 5;
	r.w = 66;
	
	env_text(event, &r, dest, ENV_PANNING_ENVELOPE_LOOP_BEGIN, "BEGIN", "%01X", MAKEPTR(inst->pan_env_loop_start), 1);
	//update_rect(&pan_env_params, &r);
	
	r.x += r.w + 5;
	r.w = 55;
	
	env_text(event, &r, dest, ENV_PANNING_ENVELOPE_LOOP_END, "END", "%01X", MAKEPTR(inst->pan_env_loop_end), 1);
	update_rect(&pan_env_params, &r);
	
	SDL_Rect pan_env_horiz_slider_rect;
	copy_rect(&pan_env_horiz_slider_rect, &pan_env_editor);
	
	pan_env_horiz_slider_rect.h = 10;
	pan_env_horiz_slider_rect.y += pan_env_editor.h + ENV_ED_MARGIN;
	pan_env_horiz_slider_rect.x -= ENV_ED_MARGIN;
	pan_env_horiz_slider_rect.w += 2 * ENV_ED_MARGIN;
	
	slider_set_params(&mused.pan_env_horiz_slider_param, 0, MUS_MAX_ENVELOPE_POINT_X, mused.pan_env_horiz_scroll, (mused.pan_env_horiz_scroll + (mused.pan_env_scale < 0 ? (pan_env_editor.w / 2) : (pan_env_editor.w * mused.pan_env_scale))), &mused.pan_env_horiz_scroll, 1, SLIDER_HORIZONTAL, mused.slider_bevel);
	slider(dest_surface, &pan_env_horiz_slider_rect, event, &mused.pan_env_horiz_slider_param);
	
	int total_height = vol_env_editor_init.h * 2 + 11 * 2 + 16 * 2;
	
	slider_set_params(&mused.point_env_slider_param, 0, total_height, mused.point_env_editor_scroll, area.h + mused.point_env_editor_scroll, &mused.point_env_editor_scroll, 1, SLIDER_VERTICAL, mused.slider_bevel);
	
	gfx_domain_set_clip(domain, NULL);
	
	adjust_rect(&area, -1);
	bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
}