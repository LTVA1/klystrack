/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2022 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

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

#include "../view.h"
#include "../event.h"
#include "../mused.h"
#include "../action.h"
#include "../diskop.h"
#include "gui/mouse.h"
#include "gui/dialog.h"
#include "gui/bevel.h"
#include "../theme.h"
#include "../mybevdefs.h"
#include "snd/freqs.h"
#include <stdbool.h>
#include "sequence.h"

#include "../command.h"

#define HEADER_HEIGHT 12 //was 12

const struct { bool margin; int w; int id; } pattern_params[] =
{
	{false, 3, PED_NOTE},
	{true, 1, PED_INSTRUMENT1},
	{false, 1, PED_INSTRUMENT2},
	{true, 1, PED_VOLUME1},
	{false, 1, PED_VOLUME2},
	{true, 1, PED_LEGATO},
	{false, 1, PED_SLIDE},
	{false, 1, PED_VIB},
	{false, 1, PED_TREM}, //wasn't there
	{true, 1, PED_COMMAND1},
	{false, 1, PED_COMMAND2},
	{false, 1, PED_COMMAND3},
	{false, 1, PED_COMMAND4},
	
	{true, 1, PED_COMMAND21}, //wasn't there
	{false, 1, PED_COMMAND22},
	{false, 1, PED_COMMAND23},
	{false, 1, PED_COMMAND24},
	{true, 1, PED_COMMAND31},
	{false, 1, PED_COMMAND32},
	{false, 1, PED_COMMAND33},
	{false, 1, PED_COMMAND34},
	{true, 1, PED_COMMAND41},
	{false, 1, PED_COMMAND42},
	{false, 1, PED_COMMAND43},
	{false, 1, PED_COMMAND44},
	{true, 1, PED_COMMAND51},
	{false, 1, PED_COMMAND52},
	{false, 1, PED_COMMAND53},
	{false, 1, PED_COMMAND54},
	{true, 1, PED_COMMAND61},
	{false, 1, PED_COMMAND62},
	{false, 1, PED_COMMAND63},
	{false, 1, PED_COMMAND64},
	{true, 1, PED_COMMAND71},
	{false, 1, PED_COMMAND72},
	{false, 1, PED_COMMAND73},
	{false, 1, PED_COMMAND74},
	{true, 1, PED_COMMAND81},
	{false, 1, PED_COMMAND82},
	{false, 1, PED_COMMAND83},
	{false, 1, PED_COMMAND84},
};

#define selrow(sel, nor) ((current_patternstep() == i) ? (sel) : (nor))
#define diszero(e, c) ((!(e)) ? mix_colors(c, colors[COLOR_PATTERN_EMPTY_DATA]) : c)

extern GfxDomain* domain;

void pattern_view_header(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, int channel)
{
	bevelex(dest_surface, dest, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	
	SDL_Rect area;
	copy_rect(&area, dest);
	adjust_rect(&area, 3);
	font_write_args(&mused.smallfont, dest_surface, &area, "%02d", channel);
	
	SDL_Rect mute;
	
	const int destw = 160;
	
	{	
		copy_rect(&mute, dest);
		mute.w = dest->h;
		
		if (!(mused.flags & COMPACT_VIEW) && !(mused.flags & EXPAND_ONLY_CURRENT_TRACK))//&& (!(mused.flags & EXPAND_ONLY_CURRENT_TRACK)))
		{
			mute.x = dest->x + destw - mute.w - 16; //was mute.x = dest->x + dest->w - mute.w; //mute.x = dest->x + dest->w - mute.w - 16;

			SDL_Rect expand;
			SDL_Rect collapse;
			
			copy_rect(&expand, dest);
			copy_rect(&collapse, dest);
			expand.w = 8;
			collapse.w = 8;
			
			expand.x = dest->x + destw - mute.w + 4; //expand.x = dest->x + dest->w - mute.w + 4;
			collapse.x = dest->x + destw - mute.w - 4; //collapse.x = dest->x + dest->w - mute.w - 4;
			
			void *action = expand_command;
			
			if(mused.song.pattern[current_pattern_for_channel(channel)].command_columns != MUS_MAX_COMMANDS - 1)
			{
				button_event(dest_surface, event, &expand, mused.slider_bevel,
					BEV_BUTTON, 
					BEV_BUTTON_ACTIVE, 
					DECAL_EXPAND, action, MAKEPTR(channel), 0, 0);
			}
				
			if(mused.song.pattern[current_pattern_for_channel(channel)].command_columns > 0)
			{
				action = hide_command;
				
				button_event(dest_surface, event, &collapse, mused.slider_bevel,
					BEV_BUTTON, 
					BEV_BUTTON_ACTIVE, 
					DECAL_COLLAPSE, action, MAKEPTR(channel), 0, 0);
			}
		}
		
		else
		{
			mute.x = dest->x + dest->w - mute.w; //mute.x = dest->x + dest->w - mute.w;
		}
	
		void *action = enable_channel;
                
        if (SDL_GetModState() & KMOD_SHIFT) action = solo_channel;
        
		button_event(dest_surface, event, &mute, mused.slider_bevel, 
				(mused.mus.channel[channel].flags & MUS_CHN_DISABLED) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, 
				(mused.mus.channel[channel].flags & MUS_CHN_DISABLED) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, 
				(mused.mus.channel[channel].flags & MUS_CHN_DISABLED) ? DECAL_AUDIO_DISABLED : DECAL_AUDIO_ENABLED, action, MAKEPTR(channel), 0, 0);
	}	
	
	if (!(mused.flags & COMPACT_VIEW) && (!(mused.flags & EXPAND_ONLY_CURRENT_TRACK) || channel == mused.current_sequencetrack))
	{
		SDL_Rect vol;
		copy_rect(&vol, &mute);
		
		const int pan_w = 42;

		vol.x -= vol.w + 3 + 14;
		vol.x -= pan_w * 2 + 3 + 12;
		vol.w = pan_w;
		vol.h -= 1;
		vol.y += 1;
		
		int d;
		int _current_pattern = current_pattern_for_channel(channel);
		
		if (_current_pattern != -1) 
		{
			if ((d = generic_field(event, &vol, 96, channel, "", "", NULL, 1)))
			{
				snapshot_cascade(S_T_SONGINFO, 96, channel);
				mused.song.pattern[_current_pattern].color = my_max(0, my_min(15, (int)mused.song.pattern[_current_pattern].color + d));
			}
			
			{
				SDL_Rect bar;
				copy_rect(&bar, &vol);
				bar.w = 8;
				bar.h = 8;
				bar.x += vol.w - 24 - 1;
				bar.y += 1;
				
				gfx_rect(dest_surface, &bar, pattern_color[mused.song.pattern[_current_pattern].color]);
			}
		}
				
		vol.x += vol.w + 1;
		
		char tmp[4] = "\xfa\xf9";
		
		bool is_max_pan = mused.song.default_panning[channel] == 127;
		if (mused.song.default_panning[channel])
			snprintf(tmp, sizeof(tmp), "%c%X", mused.song.default_panning[channel] < 0 ? '\xf9' : '\xfa', is_max_pan ? 0xf : (mused.song.default_panning[channel] == -128 ? 0xf : ((abs((int)mused.song.default_panning[channel]) >> 3) & 0xf)));
		
		if ((d = generic_field(event, &vol, 97, channel, "P", "%s", tmp, 2)))
		{
			snapshot_cascade(S_T_SONGINFO, 97, channel);
			mused.song.default_panning[channel] = my_max(-128, my_min(127, (int)mused.song.default_panning[channel] + d * (is_max_pan ? 7 : 8)));
			if (abs(mused.song.default_panning[channel]) < 8)
				mused.song.default_panning[channel] = 0;
		}
		
		//debug("%d", mused.song.default_panning[channel]);
		
		vol.x += vol.w + 1;
		
		if ((d = generic_field(event, &vol, 98, channel, "V", "%02X", MAKEPTR(mused.song.default_volume[channel]), 2)))
		{
			snapshot_cascade(S_T_SONGINFO, 98, channel);
			mused.song.default_volume[channel] = my_max(0, my_min(MAX_VOLUME, (int)mused.song.default_volume[channel] + d));
		}
	}
}

static void pattern_view_registers_map(GfxDomain *dest_surface, const SDL_Rect *reg_map)
{
	gfx_domain_set_clip(dest_surface, reg_map);
	console_clear(mused.console);
	
	console_set_color(mused.console, colors[COLOR_WAVETABLE_SAMPLE]);
	
	Uint16 current_registers_row = 0;
	Uint8 current_registers[8];
	
	font_write_args(&mused.tinyfont, dest_surface, reg_map, "CYD chip registers:");
	
	const int spacer = 3;
	
	int ch_reg_map_h = 0;
	int flag = 0;
	
	for(int i = 0; i < mused.song.num_channels; ++i)
	{	
		if((mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_4OP) && flag == 0)
		{
			ch_reg_map_h = 40; //80
			flag = 1;
		}
		
		else if(flag == 0)
		{
			ch_reg_map_h = 40;
		}
	}
	
	int cols = 0;
	int i1 = 0;
	
	const int col_w = 162;
	
	for(int i = 0; i < mused.song.num_channels; ++i)
	{
		current_registers_row = 0x200 * i;
		
		SDL_Rect row = { reg_map->x + cols * col_w, reg_map->y + 6 + spacer + ch_reg_map_h * i1, 200, 6 };
		
		for(int i = 0; i < 8; ++i)
		{
			current_registers[i] = 0;
		}
		
		current_registers[0] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_KEY_SYNC) ? 1 : 0) << 7);
		current_registers[0] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_GATE) ? 1 : 0) << 6);
		current_registers[0] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_PULSE) ? 1 : 0) << 5);
		current_registers[0] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_TRIANGLE) ? 1 : 0) << 4);
		current_registers[0] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_SAW) ? 1 : 0) << 3);
		current_registers[0] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_NOISE) ? 1 : 0) << 2);
		current_registers[0] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_METAL) ? 1 : 0) << 1);
		current_registers[0] |= ((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_LFSR) ? 1 : 0);
		
		current_registers[1] |= ((mused.cyd.channel[i].lfsr_type & 0xf) << 4);
		current_registers[1] |= ((mused.cyd.channel[i].pw & 0xf00) >> 8);
		
		current_registers[2] |= mused.cyd.channel[i].pw & 0xff;
		
		current_registers[3] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_FIXED_NOISE_PITCH) ? 1 : 0) << 7);
		current_registers[3] |= mused.mus.channel[i].noise_note & 0x7f;
		
		current_registers[4] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_WAVE) ? 1 : 0) << 7);
		current_registers[4] |= (((mused.cyd.channel[i].musflags & MUS_INST_LOCK_NOTE) ? 1 : 0) << 6);
		current_registers[4] |= (((mused.cyd.channel[i].musflags & MUS_INST_WAVE_LOCK_NOTE) ? 1 : 0) << 5);
		current_registers[4] |= (((mused.cyd.channel[i].flags & CYD_CHN_WAVE_OVERRIDE_ENV) ? 1 : 0) << 4);
		current_registers[4] |= (((mused.cyd.channel[i].musflags & MUS_INST_RELATIVE_VOLUME) ? 1 : 0) << 3);
		current_registers[4] |= mused.cyd.channel[i].mixmode & 0x7;
		
		current_registers[5] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_FILTER) ? 1 : 0) << 7);
		current_registers[5] |= ((mused.cyd.channel[i].flttype & 0x7) << 4);
		current_registers[5] |= ((mused.mus.song_track[i].filter_cutoff & 0xf00) >> 8);
		
		current_registers[6] |= mused.mus.song_track[i].filter_cutoff & 0xff;
		
		current_registers[7] |= ((mused.cyd.channel[i].flt_slope & 0x7) << 5);
		current_registers[7] |= ((mused.mus.song_track[i].filter_resonance & 0xf) << 1);
		current_registers[7] |= ((mused.cyd.channel[i].musflags & MUS_INST_QUARTER_FREQ) ? 1 : 0);
		
		font_write_args(&mused.tinyfont, dest_surface, &row, "#%04X: #%02X #%02X #%02X #%02X #%02X #%02X #%02X #%02X", current_registers_row, current_registers[0], current_registers[1], 
		current_registers[2], current_registers[3], current_registers[4], current_registers[5], current_registers[6], current_registers[7]);
		
		current_registers_row += 8;
		
		SDL_Rect row1 = { reg_map->x + cols * col_w, reg_map->y + 12 + spacer + ch_reg_map_h * i1, 200, 6 };
		
		for(int i = 0; i < 8; ++i)
		{
			current_registers[i] = 0;
		}
		
		current_registers[0] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_1_BIT_NOISE) ? 1 : 0) << 7);
		current_registers[0] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_EXPONENTIAL_VOLUME) ? 1 : 0) << 6);
		current_registers[0] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_EXPONENTIAL_ATTACK) ? 1 : 0) << 5);
		current_registers[0] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_EXPONENTIAL_DECAY) ? 1 : 0) << 4);
		current_registers[0] |= ((mused.cyd.channel[i].true_freq / 16) >> 16) & 0xf;
		
		current_registers[1] |= (((mused.cyd.channel[i].true_freq / 16) & 0xff00) >> 8);
		
		current_registers[2] |= (mused.cyd.channel[i].true_freq / 16) & 0xff;
		
		for(int k = 0; k < CYD_WAVE_MAX_ENTRIES; ++k)
		{
			if(mused.cyd.channel[i].wave_entry == &mused.cyd.wavetable_entries[k])
			{
				current_registers[3] |= k;
			}
		}
		
		current_registers[4] |= (mused.cyd.channel[i].subosc[0].wave.start_offset >> 8);
		
		current_registers[5] |= mused.cyd.channel[i].subosc[0].wave.start_offset & 0xff;
		
		current_registers[6] |= (mused.cyd.channel[i].subosc[0].wave.end_offset >> 8);
		
		current_registers[7] |= mused.cyd.channel[i].subosc[0].wave.end_offset & 0xff;
		
		font_write_args(&mused.tinyfont, dest_surface, &row1, "#%04X: #%02X #%02X #%02X #%02X #%02X #%02X #%02X #%02X", current_registers_row, current_registers[0], current_registers[1], 
		current_registers[2], current_registers[3], current_registers[4], current_registers[5], current_registers[6], current_registers[7]);
		
		current_registers_row += 8;
		
		SDL_Rect row2 = { reg_map->x + cols * col_w, reg_map->y + 18 + spacer + ch_reg_map_h * i1, 200, 6 };
		
		for(int i = 0; i < 8; ++i)
		{
			current_registers[i] = 0;
		}
		
		current_registers[0] |= my_min(0xff, mused.cyd.channel[i].adsr.volume) * (mused.cyd.channel[i].curr_tremolo + 512) / 512;
		
		current_registers[1] |= mused.cyd.channel[i].panning;
		
		current_registers[2] |= ((mused.cyd.channel[i].adsr.a & 0x3f) << 2);
		current_registers[2] |= ((mused.cyd.channel[i].adsr.d & 0x3f) >> 6);
		
		current_registers[3] |= ((mused.cyd.channel[i].adsr.d & 0xf) << 4);
		current_registers[3] |= ((mused.cyd.channel[i].adsr.s & 0x1f) >> 1);
		
		current_registers[4] |= ((mused.cyd.channel[i].adsr.s & 0x1) << 7);
		current_registers[4] |= ((mused.cyd.channel[i].adsr.r & 0x3f) << 1);
		current_registers[4] |= ((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_ENVELOPE_KEY_SCALING) ? 1 : 0);
		
		current_registers[5] |= mused.cyd.channel[i].env_ksl_level;
		
		current_registers[6] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_VOLUME_KEY_SCALING) ? 1 : 0) << 7);
		current_registers[6] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_YM_ENV) ? 1 : 0) << 6);
		current_registers[6] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_AY8930_BUZZ_MODE) ? 1 : 0) << 5);
		current_registers[6] |= ((mused.cyd.channel[i].ym_env_shape & 0x3) << 3);
		current_registers[6] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_RING_MODULATION) ? 1 : 0) << 2);
		current_registers[6] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_SYNC) ? 1 : 0) << 1);
		current_registers[6] |= ((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_FX) ? 1 : 0);
		
		current_registers[7] |= mused.cyd.channel[i].vol_ksl_level;
		
		font_write_args(&mused.tinyfont, dest_surface, &row2, "#%04X: #%02X #%02X #%02X #%02X #%02X #%02X #%02X #%02X", current_registers_row, current_registers[0], current_registers[1], 
		current_registers[2], current_registers[3], current_registers[4], current_registers[5], current_registers[6], current_registers[7]);
		
		current_registers_row += 8;
		
		SDL_Rect row3 = { reg_map->x + cols * col_w, reg_map->y + 24 + spacer + ch_reg_map_h * i1, 200, 6 };
		
		for(int i = 0; i < 8; ++i)
		{
			current_registers[i] = 0;
		}
		
		current_registers[0] |= ((mused.cyd.channel[i].subosc[0].buzz_detune_freq / 16) >> 8);
		
		current_registers[1] |= (mused.cyd.channel[i].subosc[0].buzz_detune_freq / 16) & 0xff;
		
		current_registers[2] |= mused.cyd.channel[i].ring_mod;
		
		current_registers[3] |= mused.cyd.channel[i].sync_source;
		
		current_registers[4] |= ((mused.cyd.channel[i].fx_bus & 0x3f) << 2);
		current_registers[4] |= (((mused.cyd.channel[i].musflags & MUS_INST_MULTIOSC) ? 1 : 0) << 1);
		current_registers[4] |= ((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_FM) ? 1 : 0);
		
		current_registers[5] |= mused.mus.song_track[i].extarp1;
		current_registers[6] |= mused.mus.song_track[i].extarp2;
		
		current_registers[7] |= (((mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_WAVE) ? 1 : 0) << 7);
		current_registers[7] |= (((mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_EXPONENTIAL_VOLUME) ? 1 : 0) << 6);
		current_registers[7] |= (((mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_EXPONENTIAL_ATTACK) ? 1 : 0) << 5);
		current_registers[7] |= (((mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_EXPONENTIAL_DECAY) ? 1 : 0) << 4);
		
		if(mused.cyd.channel[i].flags & CYD_CHN_ENABLE_FM)
		{
			current_registers[7] |= ((mused.cyd.channel[i].true_freq / 16 + (((mused.cyd.channel[i].fm.fm_base_note - mused.cyd.channel[i].fm.fm_carrier_base_note) << 8) + mused.cyd.channel[i].fm.fm_finetune + mused.cyd.channel[i].fm.fm_vib == 0 ? 0 : get_freq(((mused.cyd.channel[i].fm.fm_base_note - mused.cyd.channel[i].fm.fm_carrier_base_note) << 8) + mused.cyd.channel[i].fm.fm_finetune + mused.cyd.channel[i].fm.fm_vib) / 16 - get_freq(0) / 16)) >> 16) & 0xf;
		}
		
		font_write_args(&mused.tinyfont, dest_surface, &row3, "#%04X: #%02X #%02X #%02X #%02X #%02X #%02X #%02X #%02X", current_registers_row, current_registers[0], current_registers[1], 
		current_registers[2], current_registers[3], current_registers[4], current_registers[5], current_registers[6], current_registers[7]);
		
		current_registers_row += 8;
		
		SDL_Rect row4 = { reg_map->x + cols * col_w, reg_map->y + 30 + spacer + ch_reg_map_h * i1, 200, 6 };
		
		for(int i = 0; i < 8; ++i)
		{
			current_registers[i] = 0;
		}
		
		if(mused.cyd.channel[i].flags & CYD_CHN_ENABLE_FM)
		{
			current_registers[0] |= (((mused.cyd.channel[i].true_freq / 16 + (((mused.cyd.channel[i].fm.fm_base_note - mused.cyd.channel[i].fm.fm_carrier_base_note) << 8) + mused.cyd.channel[i].fm.fm_finetune + mused.cyd.channel[i].fm.fm_vib == 0 ? 0 : get_freq(((mused.cyd.channel[i].fm.fm_base_note - mused.cyd.channel[i].fm.fm_carrier_base_note) << 8) + mused.cyd.channel[i].fm.fm_finetune + mused.cyd.channel[i].fm.fm_vib) / 16 - get_freq(0) / 16)) & 0xff00) >> 8);
			
			current_registers[1] |= (mused.cyd.channel[i].true_freq / 16 + (((mused.cyd.channel[i].fm.fm_base_note - mused.cyd.channel[i].fm.fm_carrier_base_note) << 8) + mused.cyd.channel[i].fm.fm_finetune + mused.cyd.channel[i].fm.fm_vib == 0 ? 0 : get_freq(((mused.cyd.channel[i].fm.fm_base_note - mused.cyd.channel[i].fm.fm_carrier_base_note) << 8) + mused.cyd.channel[i].fm.fm_finetune + mused.cyd.channel[i].fm.fm_vib) / 16 - get_freq(0) / 16)) & 0xff;
		}
		
		for(int k = 0; k < CYD_WAVE_MAX_ENTRIES; ++k)
		{
			if(mused.cyd.channel[i].fm.wave_entry == &mused.cyd.wavetable_entries[k])
			{
				current_registers[2] |= k;
			}
		}
		
		current_registers[3] |= (mused.cyd.channel[i].fm.wave.start_offset >> 8);
		
		current_registers[4] |= mused.cyd.channel[i].fm.wave.start_offset & 0xff;
		
		current_registers[5] |= (mused.cyd.channel[i].fm.wave.end_offset >> 8);
		
		current_registers[6] |= mused.cyd.channel[i].fm.wave.end_offset & 0xff;
		
		current_registers[7] |= (((mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_ADDITIVE) ? 1 : 0) << 7);
		current_registers[7] |= (((mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_VOLUME_KEY_SCALING) ? 1 : 0) << 6);
		current_registers[7] |= (((mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_ENVELOPE_KEY_SCALING) ? 1 : 0) << 5);
		current_registers[7] |= ((mused.cyd.channel[i].fm.fm_freq_LUT & 0x3) << 3);
		current_registers[7] |= mused.cyd.channel[i].fm.feedback & 0x7;
		
		font_write_args(&mused.tinyfont, dest_surface, &row4, "#%04X: #%02X #%02X #%02X #%02X #%02X #%02X #%02X #%02X", current_registers_row, current_registers[0], current_registers[1], 
		current_registers[2], current_registers[3], current_registers[4], current_registers[5], current_registers[6], current_registers[7]);
		
		current_registers_row += 8;
		
		SDL_Rect row5 = { reg_map->x + cols * col_w, reg_map->y + 36 + spacer + ch_reg_map_h * i1, 200, 6 };
		
		for(int i = 0; i < 8; ++i)
		{
			current_registers[i] = 0;
		}
		
		current_registers[0] |= mused.cyd.channel[i].fm.adsr.volume * (mused.cyd.channel[i].fm.fm_curr_tremolo + 512) / 512;
		
		current_registers[1] |= mused.cyd.channel[i].fm.fm_vol_ksl_level;
		
		current_registers[2] |= mused.cyd.channel[i].fm.fm_env_ksl_level;
		
		current_registers[3] |= mused.cyd.channel[i].fm.harmonic;
		
		current_registers[4] |= ((mused.cyd.channel[i].fm.adsr.a & 0x3f) << 2);
		current_registers[4] |= ((mused.cyd.channel[i].fm.adsr.d & 0x3f) >> 6);
		
		current_registers[5] |= ((mused.cyd.channel[i].fm.adsr.d & 0xf) << 4);
		current_registers[5] |= ((mused.cyd.channel[i].fm.adsr.s & 0x1f) >> 1);
		
		current_registers[6] |= ((mused.cyd.channel[i].fm.adsr.s & 0x1) << 7);
		current_registers[6] |= ((mused.cyd.channel[i].fm.adsr.r & 0x3f) << 1);
		
		current_registers[6] |= mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_GATE;
		
		current_registers[7] |= (((mused.cyd.channel[i].flags & CYD_CHN_ENABLE_EXPONENTIAL_RELEASE) ? 1 : 0) << 7);
		current_registers[7] |= (((mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_EXPONENTIAL_RELEASE) ? 1 : 0) << 6);
		
		current_registers[7] |= (((mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_4OP) ? 1 : 0) << 5);
		current_registers[7] |= ((mused.cyd.channel[i].fm.alg & 0xF) << 4);
		current_registers[7] |= ((mused.cyd.channel[i].fm.flags & CYD_FM_ENABLE_3CH_EXP_MODE) ? 1 : 0);
		
		font_write_args(&mused.tinyfont, dest_surface, &row5, "#%04X: #%02X #%02X #%02X #%02X #%02X #%02X #%02X #%02X", current_registers_row, current_registers[0], current_registers[1], 
		current_registers[2], current_registers[3], current_registers[4], current_registers[5], current_registers[6], current_registers[7]);
		
		if(reg_map->y + 36 + spacer + ch_reg_map_h * i1 + 80 > domain->screen_h)
		{
			i1 = 0;
			cols++;
		}
		
		else
		{
			i1++;
		}
	}
	
	//mused.mus.channel.note
	//mused.cyd.channel->flags
	
	for(int i = 0; i < mused.song.num_channels; ++i)
	{
		//SDL_Rect row = { reg_map->x, reg_map->y + 6 * i, 200, 6 };
		//font_write_args(&mused.tinyfont, dest_surface, &row, "CYD chip registers:");
	}
}


void pattern_view_inner(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event)
{
	font_set_color(&mused.tinyfont, colors[COLOR_SMALL_TEXT]);
	
	gfx_domain_set_clip(dest_surface, dest);
	
	const int height = 8;
	const int top = mused.pattern_position - dest->h / height / 2;
	const int bottom = top + dest->h / height;
	SDL_Rect row;
	copy_rect(&row, dest);
	
	adjust_rect(&row, 1);
	gfx_rect(dest_surface, &row, colors[COLOR_OF_BACKGROUND]);
	
	row.y = (bottom - top) / 2 * height + row.y + 2 + HEADER_HEIGHT;
	row.h = height + 1;
	
	bevelex(dest_surface, &row, mused.slider_bevel, BEV_SELECTED_PATTERN_ROW, BEV_F_STRETCH_ALL);
	
	slider_set_params(&mused.pattern_slider_param, 0, mused.song.song_length - 1, mused.pattern_position, mused.pattern_position, &mused.pattern_position, 1, SLIDER_VERTICAL, mused.slider_bevel);
	
	const int char_width = mused.largefont.w;
	int w = 2 * char_width + 4 + 4;
	int narrow_w = 2 * char_width + 4 + 4;
	const int SPACER = 4;
	
	for(int i = 0; i < mused.song.num_channels; i++)
	{
		mused.widths[i][0] = 2 * char_width + 4 + 4; //width for every channel
		mused.widths[i][1] = 2 * char_width + 4 + 4; //narrow width for every channel
		
		for (int param = PED_NOTE; param < PED_PARAMS; ++param)
		{
			if(param < PED_COMMAND21)
			{
				if (param == PED_NOTE || viscol(param))
					mused.widths[i][0] += pattern_params[param].w * char_width;
					
				if (param == PED_NOTE)
					mused.widths[i][1] += pattern_params[param].w * char_width;
					
				if (pattern_params[param].margin && param < PED_PARAMS - 1 && viscol(param + 1))
				{
					mused.widths[i][0] += SPACER;
					
					if (param == PED_NOTE)
						mused.widths[i][1] += SPACER;
				}
			}
				
			else if(current_pattern_for_channel(i) != -1)
			{
				if((param - PED_COMMAND21) < mused.song.pattern[current_pattern_for_channel(i)].command_columns * 4)
				{
					if (param == PED_NOTE || viscol(param))
						mused.widths[i][0] += pattern_params[param].w * char_width;
					
					if (param == PED_NOTE)
						mused.widths[i][1] += pattern_params[param].w * char_width;
						
					if (pattern_params[param].margin && param < PED_PARAMS - 1 && viscol(param + 1))
					{
						mused.widths[i][0] += SPACER;
							
						if (param == PED_NOTE)
							mused.widths[i][1] += SPACER;
					}
				}
			}
		}
		
		if (!(mused.flags & EXPAND_ONLY_CURRENT_TRACK))
			mused.widths[i][1] = mused.widths[i][0];
	}
	
	for (int param = PED_NOTE; param < PED_PARAMS; ++param)
	{
		if(param < PED_COMMAND21)
		{
			if (param == PED_NOTE || viscol(param))
				w += pattern_params[param].w * char_width;
				
			if (param == PED_NOTE)
				narrow_w += pattern_params[param].w * char_width;
				
			if (pattern_params[param].margin && param < PED_PARAMS - 1 && viscol(param + 1))
			{
				w += SPACER;
				
				if (param == PED_NOTE)
					narrow_w += SPACER;
			}
		}
			
		else
		{
			if((param - PED_COMMAND21) < mused.song.pattern[current_pattern_for_channel(0)].command_columns * 4)
			{
				if (param == PED_NOTE || viscol(param))
					w += pattern_params[param].w * char_width;
				
				if (param == PED_NOTE)
					narrow_w += pattern_params[param].w * char_width;
					
				if (pattern_params[param].margin && param < PED_PARAMS - 1 && viscol(param + 1))
				{
					w += SPACER;
						
					if (param == PED_NOTE)
					{
						narrow_w += SPACER;
					}
				}
			}
		}
	}
		
	if (!(mused.flags & EXPAND_ONLY_CURRENT_TRACK))
	{
		narrow_w = w;
	}
	
	int last_visible = mused.pattern_horiz_position;
	int sum_width = 0;
	
	for(int i = mused.pattern_horiz_position; sum_width <= dest->w; i++, last_visible++)
	{
		sum_width += (i == mused.current_sequencetrack) ? mused.widths[i][0] : mused.widths[i][1];
	}
	
	//                                                                                                                 my_min(mused.song.num_channels, last_visible - 1) - 1
	slider_set_params(&mused.pattern_horiz_slider_param, 0, mused.song.num_channels - 1, mused.pattern_horiz_position, my_min(mused.song.num_channels, last_visible - 1) - 1, &mused.pattern_horiz_position, 1, SLIDER_HORIZONTAL, mused.slider_bevel); //slider_set_params(&mused.pattern_horiz_slider_param, 0, mused.song.num_channels - 1, mused.pattern_horiz_position, my_min(mused.song.num_channels, mused.pattern_horiz_position + 1 + (dest->w - w) / narrow_w) - 1, &mused.pattern_horiz_position, 1, SLIDER_HORIZONTAL, mused.slider_bevel);
	
	int x = 0;
	
	int registers_map_x_offset = 0;
	int registers_map_y_offset = 0;
	
	int pixel_offset = 0;
	
	if((mused.flags & SONG_PLAYING) && (mused.flags2 & SMOOTH_SCROLL) && (mused.flags & FOLLOW_PLAY_POSITION))
	{
		pixel_offset = (Uint32)mused.mus.song_counter * (Uint32)8 / (((Uint32)mused.mus.song_position & (Uint32)1) ? (Uint32)mused.song.song_speed2 : (Uint32)mused.song.song_speed) - ((Uint32)mused.draw_passes_since_song_start == (Uint32)0 ? (Uint32)0 : ((Uint32)mused.draw_passes_since_song_start < (Uint32)4 ? (Uint32)mused.draw_passes_since_song_start : (Uint32)4));
		
		if(mused.draw_passes_since_song_start < 5) mused.draw_passes_since_song_start++;
	}
	
	if (mused.focus == EDITPATTERN)
	{
		int x = 2 + 2 * char_width + SPACER;
		for (int param = 0; param < mused.current_patternx; ++param)
		{
			if (viscol(param)) 
			{
				x += (param > 0 && pattern_params[param].margin ? SPACER : 0) + pattern_params[param].w * char_width;
			}
		}
		
		if (pattern_params[mused.current_patternx].margin) x += SPACER;
		
		int cursor_width = 0;
		int q = 0;
		
		while(dest->w <= cursor_width + mused.widths[mused.pattern_horiz_position + q][1])
		{
			cursor_width += mused.widths[mused.pattern_horiz_position + q][1];
			q++;
		}
		
		if (mused.current_sequencetrack >= mused.pattern_horiz_position && mused.current_sequencetrack <= my_min(mused.song.num_channels, mused.pattern_horiz_position + 1 + dest->w - cursor_width) - 1) //if (mused.current_sequencetrack >= mused.pattern_horiz_position && mused.current_sequencetrack <= my_min(mused.song.num_channels, mused.pattern_horiz_position + 1 + (dest->w - w) / narrow_w) - 1)
		{
			int narrow_width = 0;
			
			for(int i = mused.pattern_horiz_position; i < mused.current_sequencetrack; ++i)
			{
				narrow_width += mused.widths[i][1];
			}
			
			SDL_Rect cursor = { 1 + dest->x + narrow_width + x, row.y, pattern_params[mused.current_patternx].w * char_width, row.h}; //SDL_Rect cursor = { 1 + dest->x + narrow_w * (mused.current_sequencetrack - mused.pattern_horiz_position) + x, row.y, pattern_params[mused.current_patternx].w * char_width, row.h};
			
			//debug("draws from here %d", mused.pattern_horiz_position);
			
			adjust_rect(&cursor, -2);
			set_cursor(&cursor);
		}
		
		if (mused.selection.start != mused.selection.end)
		{
			if (mused.selection.start <= bottom && mused.selection.end >= top)
			{
				int narrow_width2 = 0;
				
				for(int i = mused.pattern_horiz_position; i < mused.current_sequencetrack; ++i)
				{
					narrow_width2 += mused.widths[i][1];
				}
				
				if((mused.selection.patternx_start - 12) > mused.song.pattern[current_pattern_for_channel(mused.current_sequencetrack)].command_columns * 4)
				{
					mused.selection.patternx_start = 12 + mused.song.pattern[current_pattern_for_channel(mused.current_sequencetrack)].command_columns * 4;
				}
				
				if((mused.selection.patternx_end - 12) > mused.song.pattern[current_pattern_for_channel(mused.current_sequencetrack)].command_columns * 4)
				{
					mused.selection.patternx_end = 12 + mused.song.pattern[current_pattern_for_channel(mused.current_sequencetrack)].command_columns * 4;
				}
				
				Uint16 start_shift = (mused.selection.patternx_start > 0 ? (8 * 3 + SPACER) : 0) + (mused.selection.patternx_start > 1 ? (8) : 0) + (mused.selection.patternx_start > 2 ? (8 + SPACER) : 0) + (mused.selection.patternx_start > 3 ? (8) : 0) + (mused.selection.patternx_start > 4 ? (8 + SPACER) : 0) + (mused.selection.patternx_start > 5 ? (8) : 0) + (mused.selection.patternx_start > 6 ? (8) : 0) + (mused.selection.patternx_start > 7 ? (8) : 0) + (mused.selection.patternx_start > 8 ? (8 + SPACER) : 0);
				
				for(int com = 0; com < MUS_MAX_COMMANDS; ++com)
				{
					if(mused.selection.patternx_start > 9 + 4 * com)
					{
						start_shift += 8;
					}
					
					if(mused.selection.patternx_start > 10 + 4 * com)
					{
						start_shift += 8;
					}
					
					if(mused.selection.patternx_start > 11 + 4 * com)
					{
						start_shift += 8;
					}
					
					if(mused.selection.patternx_start > 12 + 4 * com)
					{
						start_shift += 8 + SPACER;
					}
				}
				
				Uint16 end_shift = (mused.selection.patternx_end < 8 ? (8) : 0) + (mused.selection.patternx_end < 7 ? (8) : 0) + (mused.selection.patternx_end < 6 ? (8) : 0) + (mused.selection.patternx_end < 5 ? (8 + SPACER) : 0) + (mused.selection.patternx_end < 4 ? (8) : 0) + (mused.selection.patternx_end < 3 ? (8 + SPACER) : 0) + (mused.selection.patternx_end < 2 ? (8) : 0) + (mused.selection.patternx_end < 1 ? (8 + SPACER) : 0);
				
				for(int com = MUS_MAX_COMMANDS - 1; com >= 0; --com)
				{
					if(mused.selection.patternx_end < 9 + 4 * com)
					{
						end_shift += 8 + SPACER;
					}
					
					if(mused.selection.patternx_end < 10 + 4 * com)
					{
						end_shift += 8;
					}
					
					if(mused.selection.patternx_end < 11 + 4 * com)
					{
						end_shift += 8;
					}
					
					if(mused.selection.patternx_end < 12 + 4 * com)
					{
						end_shift += 8;
					}
				}
				
				end_shift -= (MUS_MAX_COMMANDS - mused.song.pattern[current_pattern_for_channel(mused.current_sequencetrack)].command_columns - 1) * (8 * 4 + SPACER);
				
				if (!(viscol(PED_INSTRUMENT1)) && mused.selection.patternx_end <= PED_INSTRUMENT2)
				{
					end_shift -= SPACER + 8 * 2;
				}
				
				if (!(viscol(PED_VOLUME1)) && mused.selection.patternx_end <= PED_VOLUME2)
				{
					end_shift -= SPACER + 8 * 2;
				}
				
				if (!(viscol(PED_LEGATO)) && mused.selection.patternx_end <= PED_TREM)
				{
					end_shift -= SPACER + 8 * 4;
				}
				
				if (!(viscol(PED_COMMAND1)) && mused.selection.patternx_end <= PED_COMMAND4)
				{
					end_shift -= SPACER + 8 * 4;
				}
				
				SDL_Rect selection = { dest->x + narrow_width2 + 2 * char_width + SPACER + start_shift, 
					row.y + height * (mused.selection.start - mused.pattern_position) - pixel_offset, mused.widths[mused.current_sequencetrack][0] - (2 * char_width + SPACER) - start_shift - end_shift, height * (mused.selection.end - mused.selection.start)}; //SDL_Rect selection = { dest->x + narrow_w * (mused.current_sequencetrack - mused.pattern_horiz_position) + 2 * char_width + SPACER, row.y + height * (mused.selection.start - mused.pattern_position), w - (2 * char_width + SPACER), height * (mused.selection.end - mused.selection.start)};
					
				adjust_rect(&selection, -3);
				selection.h += 2;
				bevel(dest_surface, &selection, mused.slider_bevel, BEV_SELECTION);
			}
		}
	}
	
	for (int channel = mused.pattern_horiz_position; channel < mused.song.num_channels && x < dest->w; x += ((channel == mused.current_sequencetrack) ? mused.widths[channel][0] : mused.widths[channel][1]), ++channel) //for (int channel = mused.pattern_horiz_position; channel < mused.song.num_channels && x < dest->w; x += ((channel == mused.current_sequencetrack) ? w : narrow_w), ++channel)
	{
		const MusSeqPattern *sp = &mused.song.sequence[channel][0];

		SDL_Rect track;
		copy_rect(&track, dest);
		track.w = ((channel == mused.current_sequencetrack) ? mused.widths[channel][0] : mused.widths[channel][1]) + 2; //track.w = ((channel == mused.current_sequencetrack) ? w : narrow_w) + 2;
		track.x += x;
		
		gfx_domain_set_clip(dest_surface, NULL);
		
		SDL_Rect header;
		copy_rect(&header, &track);
		header.h = HEADER_HEIGHT;
		header.w -= 2;
		
		//pattern_view_header(dest_surface, &header, event, channel);
		
		track.h -= HEADER_HEIGHT;
		track.y += HEADER_HEIGHT + 1;
		
		//bevelex(dest_surface, &track, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL|BEV_F_DISABLE_CENTER);
		adjust_rect(&track, 3);
		
		track.y -= 8;
		track.h += 8;
		
		for (int i = 0; i < mused.song.num_sequences[channel]; ++i, ++sp)
		{
			if (sp->position >= bottom) break;
			
			int len = mused.song.pattern[sp->pattern].num_steps;
			
			if (sp->position + len <= top) continue;
			
			if (i < mused.song.num_sequences[channel] - 1)
			{
				len = my_min(len, (sp + 1)->position - sp->position);
			}
			
			SDL_Rect pat = { track.x, (sp->position - top) * height + track.y - pixel_offset + 8, ((channel == mused.current_sequencetrack) ? mused.widths[channel][0] : mused.widths[channel][1]), len * height }; //SDL_Rect pat = { track.x, (sp->position - top) * height + track.y, ((channel == mused.current_sequencetrack) ? w : narrow_w), len * height };
			
			//track.y -= 8;
			//track.h += 8;
			
			SDL_Rect text;
			copy_rect(&text, &pat);
			clip_rect(&pat, &track);
			
			gfx_domain_set_clip(dest_surface, &pat);
			
			for (int step = 0; step < len; ++step, text.y += height)
			{
				if (text.y < pat.y) continue;
				if (sp->position + step >= bottom) break;
				
				MusStep *s = &mused.song.pattern[sp->pattern].step[step];
				
				SDL_Rect pos;
				copy_rect(&pos, &text);
				pos.h = height;
				
				static const struct { Uint32 bar, beat, normal; } coltab[] =
				{
					{COLOR_PATTERN_BAR, COLOR_PATTERN_BEAT, COLOR_PATTERN_NORMAL},
					{COLOR_PATTERN_INSTRUMENT_BAR, COLOR_PATTERN_INSTRUMENT_BEAT, COLOR_PATTERN_INSTRUMENT},
					{COLOR_PATTERN_INSTRUMENT_BAR, COLOR_PATTERN_INSTRUMENT_BEAT, COLOR_PATTERN_INSTRUMENT},
					{COLOR_PATTERN_VOLUME_BAR, COLOR_PATTERN_VOLUME_BEAT, COLOR_PATTERN_VOLUME},
					{COLOR_PATTERN_VOLUME_BAR, COLOR_PATTERN_VOLUME_BEAT, COLOR_PATTERN_VOLUME},
					{COLOR_PATTERN_CTRL_BAR, COLOR_PATTERN_CTRL_BEAT, COLOR_PATTERN_CTRL},
					{COLOR_PATTERN_CTRL_BAR, COLOR_PATTERN_CTRL_BEAT, COLOR_PATTERN_CTRL},
					{COLOR_PATTERN_CTRL_BAR, COLOR_PATTERN_CTRL_BEAT, COLOR_PATTERN_CTRL},
					{COLOR_PATTERN_CTRL_BAR, COLOR_PATTERN_CTRL_BEAT, COLOR_PATTERN_CTRL}, //wasn't there
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND}, //wasn't there
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND},
					{COLOR_PATTERN_COMMAND_BAR, COLOR_PATTERN_COMMAND_BEAT, COLOR_PATTERN_COMMAND}

				};
				
				if (step == 0)
				{
					console_set_color(mused.console, colors[COLOR_PATTERN_SEQ_NUMBER]);
					font_write_args(&mused.console->font, dest_surface, &pos, "%02X", sp->pattern);
				}
				
				else
				{
					console_set_color(mused.console, timesig(step, colors[COLOR_PATTERN_BAR], colors[COLOR_PATTERN_BEAT], colors[COLOR_PATTERN_NORMAL]));
					SDL_Rect cpos;
					copy_rect(&cpos, &pos);
					
					cpos.y += (mused.console->font.h - mused.tinyfont.h) / 2;
					cpos.x += (mused.console->font.w * 2 - mused.tinyfont.w * 2) / 2 - 1;
					
					if (SHOW_DECIMALS & mused.flags)
						font_write_args(&mused.tinyfont, dest_surface, &cpos, "%02d\n", (step + 100) % 100); // so we don't get negative numbers
					else
						font_write_args(&mused.tinyfont, dest_surface, &cpos, "%02X\n", step & 0xff);
				}
				
				pos.x += 2 * char_width + SPACER;
				
				for (int param = PED_NOTE; (param - PED_COMMAND21) < mused.song.pattern[sp->pattern].command_columns * 4; ++param) //(param - PED_COMMAND21) < mused.song.pattern[sp->pattern].command_columns * 4
				{
					if (param != PED_NOTE && !viscol(param)) continue;
				
					pos.w = pattern_params[param].w * char_width;
					
					if (pattern_params[param].margin)
						pos.x += SPACER;
						
					Uint32 color = 0;
					
					if (sp->position + step != mused.pattern_position)	
						color = timesig(step, colors[coltab[param].bar], colors[coltab[param].beat], colors[coltab[param].normal]);
					else
						console_set_color(mused.console, colors[COLOR_PATTERN_SELECTED]);
				
					switch (param)
					{
						case PED_NOTE:
							{
								char note[5];
								
								switch(s->note)
								{
									case MUS_NOTE_CUT:
									{
										snprintf(note, sizeof(note), "%s", "OFF"); break;
									}
									
									case MUS_NOTE_RELEASE:
									{
										snprintf(note, sizeof(note), "%s", "\x08\x09\x0b"); break;
									}
									
									case MUS_NOTE_MACRO_RELEASE:
									{
										snprintf(note, sizeof(note), "%s", "M\x08\x0b"); break;
									}
									
									case MUS_NOTE_RELEASE_WITHOUT_MACRO:
									{
										snprintf(note, sizeof(note), "%s", "N\x08\x0b"); break;
									}
									
									case MUS_NOTE_NONE:
									{
										snprintf(note, sizeof(note), "%s", "---"); break;
									}
									
									default:
									{
										snprintf(note, sizeof(note), "%s", notename(s->note)); break;
									}
								}
								
								if (sp->position + step != mused.pattern_position)
									console_set_color(mused.console, diszero(mused.song.pattern[sp->pattern].step[step].note != MUS_NOTE_NONE, color));
									
								font_write(&mused.console->font, dest_surface, &pos, note);
							}
							break;
						
						case PED_INSTRUMENT1:
						case PED_INSTRUMENT2:
						
							if (sp->position + step != mused.pattern_position)
								console_set_color(mused.console, diszero(s->instrument != MUS_NOTE_NO_INSTRUMENT, color));
						
							if (s->instrument != MUS_NOTE_NO_INSTRUMENT)
								font_write_args(&mused.console->font, dest_surface, &pos, "%X", (s->instrument >> (4 - (param - PED_INSTRUMENT1) * 4)) & 0xf);
							else
								font_write(&mused.console->font, dest_surface, &pos, "-");
							break;
							
						case PED_VOLUME1:
						case PED_VOLUME2:
						
							if (sp->position + step != mused.pattern_position)
								console_set_color(mused.console, diszero(s->volume != MUS_NOTE_NO_VOLUME, color));
						
							if (s->volume != MUS_NOTE_NO_VOLUME)
							{
								if (param == PED_VOLUME1 && s->volume > MAX_VOLUME)
								{
									char c;
									
									switch (s->volume & 0xf0)
									{
										default:
											c = '-';
											break;
											
										case MUS_NOTE_VOLUME_SET_PAN:
											c = 'P';
											break;
											
										case MUS_NOTE_VOLUME_PAN_LEFT:
											c = 0xf9;
											break;
											
										case MUS_NOTE_VOLUME_PAN_RIGHT:
											c = 0xfa;
											break;
											
										case MUS_NOTE_VOLUME_FADE_UP:
											c = 0xfb;
											break;
											
										case MUS_NOTE_VOLUME_FADE_DN:
											c = 0xfc;
											break;
									}
									
									font_write_args(&mused.console->font, dest_surface, &pos, "%c", c);
								}
								
								else
								{
									font_write_args(&mused.console->font, dest_surface, &pos, "%X", (s->volume >> (4 - (param - PED_VOLUME1) * 4)) & 0xf);
								}
							}
							else
								font_write(&mused.console->font, dest_surface, &pos, "-");
							break;
							
						case PED_LEGATO:
						case PED_SLIDE:
						case PED_VIB:
						case PED_TREM:
						
							if (sp->position + step != mused.pattern_position)
								console_set_color(mused.console, diszero((s->ctrl & (1 << (param - PED_LEGATO))), color)); //was console_set_color(mused.console, diszero((s->ctrl & (1 << (param - PED_LEGATO))), color));
						
							font_write_args(&mused.console->font, dest_surface, &pos, "%c", (s->ctrl & (1 << (param - PED_LEGATO))) ? "LSVT"[param - PED_LEGATO] : '-'); //was font_write_args(&mused.console->font, dest_surface, &pos, "%c", (s->ctrl & (1 << (param - PED_LEGATO))) ? "LSV"[param - PED_LEGATO] : '-');
							break;
							
						case PED_COMMAND1:
						case PED_COMMAND2:
						case PED_COMMAND3:
						case PED_COMMAND4:
						
							if (sp->position + step != mused.pattern_position)
							{
								console_set_color(mused.console, diszero(s->command[0] != 0, color));
							}
							
							if(mused.flags2 & HIGHLIGHT_COMMANDS)
							{
								if((~(get_instruction_mask(s->command[0])) & (0xf << ((PED_COMMAND4 - param) * 4))) && s->command[0] != 0 && is_valid_command(s->command[0]))
								{
									Uint32 highlight_color = ((color & 0xff) * 1 / 2) + (((((color >> 8) & 0xff) * 1 / 2) & 0xff) << 8) + 0xff0000;//my_min(0xff0000, (((((color >> 16) & 0xff) * 7 / 4) & 0xff) << 16));
									
									if((sp->position + step) == mused.pattern_position)
									{
										highlight_color = 0x00ee00;
									}
									
									console_set_color(mused.console, diszero(s->command[0] != 0, highlight_color));
								}
							}
							
							if ((mused.flags & HIDE_ZEROS) && s->command[0] == 0)
								font_write_args(&mused.console->font, dest_surface, &pos, "-");
							else
								font_write_args(&mused.console->font, dest_surface, &pos, "%X", (s->command[0] >> (12 - (param - PED_COMMAND1) * 4)) & 0xf);
							break;
							
						case PED_COMMAND21: //wasn't there
						case PED_COMMAND22:
						case PED_COMMAND23:
						case PED_COMMAND24:
						
							if (sp->position + step != mused.pattern_position)
								console_set_color(mused.console, diszero(s->command[1] != 0, color));
							
							if(mused.flags2 & HIGHLIGHT_COMMANDS)
							{
								if((~(get_instruction_mask(s->command[1])) & (0xf << ((PED_COMMAND24 - param) * 4))) && s->command[1] != 0 && is_valid_command(s->command[1]))
								{
									Uint32 highlight_color = ((color & 0xff) * 1 / 2) + (((((color >> 8) & 0xff) * 1 / 2) & 0xff) << 8) + 0xff0000;//my_min(0xff0000, (((((color >> 16) & 0xff) * 7 / 4) & 0xff) << 16));
									
									if((sp->position + step) == mused.pattern_position)
									{
										highlight_color = 0x00ee00;
									}
									
									console_set_color(mused.console, diszero(s->command[1] != 0, highlight_color));
								}
							}
							
							if ((mused.flags & HIDE_ZEROS) && s->command[1] == 0)
								font_write_args(&mused.console->font, dest_surface, &pos, "-");
							else
								font_write_args(&mused.console->font, dest_surface, &pos, "%X", (s->command[1] >> (12 - (param - PED_COMMAND21) * 4)) & 0xf);
							break;
							
						case PED_COMMAND31: //wasn't there
						case PED_COMMAND32:
						case PED_COMMAND33:
						case PED_COMMAND34:
						
							if (sp->position + step != mused.pattern_position)
								console_set_color(mused.console, diszero(s->command[2] != 0, color));
							
							if(mused.flags2 & HIGHLIGHT_COMMANDS)
							{
								if((~(get_instruction_mask(s->command[2])) & (0xf << ((PED_COMMAND34 - param) * 4))) && s->command[2] != 0 && is_valid_command(s->command[2]))
								{
									Uint32 highlight_color = ((color & 0xff) * 1 / 2) + (((((color >> 8) & 0xff) * 1 / 2) & 0xff) << 8) + 0xff0000;//my_min(0xff0000, (((((color >> 16) & 0xff) * 7 / 4) & 0xff) << 16));
									
									if((sp->position + step) == mused.pattern_position)
									{
										highlight_color = 0x00ee00;
									}
									
									console_set_color(mused.console, diszero(s->command[2] != 0, highlight_color));
								}
							}
							
							if ((mused.flags & HIDE_ZEROS) && s->command[2] == 0)
								font_write_args(&mused.console->font, dest_surface, &pos, "-");
							else
								font_write_args(&mused.console->font, dest_surface, &pos, "%X", (s->command[2] >> (12 - (param - PED_COMMAND31) * 4)) & 0xf);
							break;
							
						case PED_COMMAND41: //wasn't there
						case PED_COMMAND42:
						case PED_COMMAND43:
						case PED_COMMAND44:
						
							if (sp->position + step != mused.pattern_position)
								console_set_color(mused.console, diszero(s->command[3] != 0, color));
							
							if(mused.flags2 & HIGHLIGHT_COMMANDS)
							{
								if((~(get_instruction_mask(s->command[3])) & (0xf << ((PED_COMMAND44 - param) * 4))) && s->command[3] != 0 && is_valid_command(s->command[3]))
								{
									Uint32 highlight_color = ((color & 0xff) * 1 / 2) + (((((color >> 8) & 0xff) * 1 / 2) & 0xff) << 8) + 0xff0000;//my_min(0xff0000, (((((color >> 16) & 0xff) * 7 / 4) & 0xff) << 16));
									
									if((sp->position + step) == mused.pattern_position)
									{
										highlight_color = 0x00ee00;
									}
									
									console_set_color(mused.console, diszero(s->command[3] != 0, highlight_color));
								}
							}
							
							if ((mused.flags & HIDE_ZEROS) && s->command[3] == 0)
								font_write_args(&mused.console->font, dest_surface, &pos, "-");
							else
								font_write_args(&mused.console->font, dest_surface, &pos, "%X", (s->command[3] >> (12 - (param - PED_COMMAND41) * 4)) & 0xf);
							break;
							
						case PED_COMMAND51: //wasn't there
						case PED_COMMAND52:
						case PED_COMMAND53:
						case PED_COMMAND54:
						
							if (sp->position + step != mused.pattern_position)
								console_set_color(mused.console, diszero(s->command[4] != 0, color));
							
							if(mused.flags2 & HIGHLIGHT_COMMANDS)
							{
								if((~(get_instruction_mask(s->command[4])) & (0xf << ((PED_COMMAND54 - param) * 4))) && s->command[4] != 0 && is_valid_command(s->command[4]))
								{
									Uint32 highlight_color = ((color & 0xff) * 1 / 2) + (((((color >> 8) & 0xff) * 1 / 2) & 0xff) << 8) + 0xff0000;//my_min(0xff0000, (((((color >> 16) & 0xff) * 7 / 4) & 0xff) << 16));
									
									if((sp->position + step) == mused.pattern_position)
									{
										highlight_color = 0x00ee00;
									}
									
									console_set_color(mused.console, diszero(s->command[4] != 0, highlight_color));
								}
							}
							
							if ((mused.flags & HIDE_ZEROS) && s->command[4] == 0)
								font_write_args(&mused.console->font, dest_surface, &pos, "-");
							else
								font_write_args(&mused.console->font, dest_surface, &pos, "%X", (s->command[4] >> (12 - (param - PED_COMMAND51) * 4)) & 0xf);
							break;
							
						case PED_COMMAND61: //wasn't there
						case PED_COMMAND62:
						case PED_COMMAND63:
						case PED_COMMAND64:
						
							if (sp->position + step != mused.pattern_position)
								console_set_color(mused.console, diszero(s->command[5] != 0, color));
							
							if(mused.flags2 & HIGHLIGHT_COMMANDS)
							{
								if((~(get_instruction_mask(s->command[5])) & (0xf << ((PED_COMMAND64 - param) * 4))) && s->command[5] != 0 && is_valid_command(s->command[5]))
								{
									Uint32 highlight_color = ((color & 0xff) * 1 / 2) + (((((color >> 8) & 0xff) * 1 / 2) & 0xff) << 8) + 0xff0000;//my_min(0xff0000, (((((color >> 16) & 0xff) * 7 / 4) & 0xff) << 16));
									
									if((sp->position + step) == mused.pattern_position)
									{
										highlight_color = 0x00ee00;
									}
									
									console_set_color(mused.console, diszero(s->command[5] != 0, highlight_color));
								}
							}
							
							if ((mused.flags & HIDE_ZEROS) && s->command[5] == 0)
								font_write_args(&mused.console->font, dest_surface, &pos, "-");
							else
								font_write_args(&mused.console->font, dest_surface, &pos, "%X", (s->command[5] >> (12 - (param - PED_COMMAND61) * 4)) & 0xf);
							break;
							
						case PED_COMMAND71: //wasn't there
						case PED_COMMAND72:
						case PED_COMMAND73:
						case PED_COMMAND74:
						
							if (sp->position + step != mused.pattern_position)
								console_set_color(mused.console, diszero(s->command[6] != 0, color));
							
							if(mused.flags2 & HIGHLIGHT_COMMANDS)
							{
								if((~(get_instruction_mask(s->command[6])) & (0xf << ((PED_COMMAND74 - param) * 4))) && s->command[6] != 0 && is_valid_command(s->command[6]))
								{
									Uint32 highlight_color = ((color & 0xff) * 1 / 2) + (((((color >> 8) & 0xff) * 1 / 2) & 0xff) << 8) + 0xff0000;//my_min(0xff0000, (((((color >> 16) & 0xff) * 7 / 4) & 0xff) << 16));
									
									if((sp->position + step) == mused.pattern_position)
									{
										highlight_color = 0x00ee00;
									}
									
									console_set_color(mused.console, diszero(s->command[6] != 0, highlight_color));
								}
							}
							
							if ((mused.flags & HIDE_ZEROS) && s->command[6] == 0)
								font_write_args(&mused.console->font, dest_surface, &pos, "-");
							else
								font_write_args(&mused.console->font, dest_surface, &pos, "%X", (s->command[6] >> (12 - (param - PED_COMMAND71) * 4)) & 0xf);
							break;
							
						case PED_COMMAND81: //wasn't there
						case PED_COMMAND82:
						case PED_COMMAND83:
						case PED_COMMAND84:
						
							if (sp->position + step != mused.pattern_position)
								console_set_color(mused.console, diszero(s->command[7] != 0, color));
							
							if(mused.flags2 & HIGHLIGHT_COMMANDS)
							{
								if((~(get_instruction_mask(s->command[7])) & (0xf << ((PED_COMMAND84 - param) * 4))) && s->command[7] != 0 && is_valid_command(s->command[7]))
								{
									Uint32 highlight_color = ((color & 0xff) * 1 / 2) + (((((color >> 8) & 0xff) * 1 / 2) & 0xff) << 8) + 0xff0000;//my_min(0xff0000, (((((color >> 16) & 0xff) * 7 / 4) & 0xff) << 16));
									
									if((sp->position + step) == mused.pattern_position)
									{
										highlight_color = 0x00ee00;
									}
									
									console_set_color(mused.console, diszero(s->command[7] != 0, highlight_color));
								}
							}
							
							if ((mused.flags & HIDE_ZEROS) && s->command[7] == 0)
								font_write_args(&mused.console->font, dest_surface, &pos, "-");
							else
								font_write_args(&mused.console->font, dest_surface, &pos, "%X", (s->command[7] >> (12 - (param - PED_COMMAND81) * 4)) & 0xf);
							break;
							
						default: break;
					}
					
					SDL_Rect tmp;
					copy_rect(&tmp, &pos);
					clip_rect(&tmp, &track);
					
					if(tmp.y < track.y + track.h - 18) //so rows drawn under the slider don't result in unwanted hits
					{
						if (tmp.y - header.y > HEADER_HEIGHT + 3 && mused.frames_since_menu_close > 5)
						{
							if(mused.flags2 & DRAG_SELECT_PATTERN)
							{
								if(mused.selection.drag_selection)
								{
									int mouse_x, mouse_y;
									
									Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
									gfx_convert_mouse_coordinates(domain, &mouse_x, &mouse_y);
									
									if ((mouse_x >= tmp.x - SPACER) && (mouse_y >= tmp.y) && (mouse_x <= tmp.x + tmp.w + SPACER / 2) && (mouse_y < tmp.y + tmp.h))
									{
										if(mused.selection.prev_start != (sp->position + step) || mused.selection.prev_patternx_start != param)
										{
											if(mused.selection.channel == channel)
											{
												mused.selection.start = mused.selection.prev_start;
												mused.selection.end = sp->position + step;
												
												mused.selection.patternx_start = mused.selection.prev_patternx_start;
												mused.selection.patternx_end = param;
											}
											
											if(mused.selection.start > mused.selection.end)
											{
												int temp = mused.selection.start;
												mused.selection.start = mused.selection.end;
												mused.selection.end = temp;
											}
											
											if(mused.selection.patternx_start > mused.selection.patternx_end)
											{
												int temp = mused.selection.patternx_start;
												mused.selection.patternx_start = mused.selection.patternx_end;
												mused.selection.patternx_end = temp;
											}
										}
									}
									
									if(event->type == SDL_MOUSEBUTTONUP)
									{
										mused.selection.drag_selection = false;
										
										if(mused.selection.start != mused.selection.end)
										{
											mused.jump_in_pattern = false;
										}
									}
								}
								
								if(check_event(event, &tmp, NULL, NULL, NULL, NULL))
								{
									if(!(mused.selection.drag_selection) && (mused.selection.start == mused.selection.end))
									{
										mused.selection.prev_start = sp->position + step;
										mused.selection.prev_patternx_start = param;
										mused.selection.drag_selection = true;
										
										mused.selection.channel = channel;
									}
									
									if(!(mused.selection.drag_selection) && (mused.selection.start != mused.selection.end))
									{
										mused.selection.drag_selection = false;
										mused.jump_in_pattern = true;
										mused.selection.start = mused.selection.end = mused.selection.patternx_start = mused.selection.patternx_end = -1;
									}
								}
							}
						}
						
						//if (sp && event->type == SDL_MOUSEBUTTONDOWN && tmp.y - header.y > HEADER_HEIGHT + 3) //so we do not process the topmost row which is hidden under header and is there for smooth scroll (so rows disappear upper then pattern edge where they are still visible)
						if(mused.flags2 & DRAG_SELECT_PATTERN)
						{
							if (sp && event->type == SDL_MOUSEBUTTONUP && tmp.y - header.y > HEADER_HEIGHT + 3 && mused.frames_since_menu_close > 5)
							{
								if(mused.jump_in_pattern)
								{
									check_event_mousebuttonup(event, &tmp, select_pattern_param, MAKEPTR(param), MAKEPTR(sp->position + step), MAKEPTR(channel)); //change focus row and column on mouse button down (preparing for mouse drag selection)
								}
								
								if(check_event_mousebuttonup(event, &tmp, NULL, NULL, NULL, NULL))
								{
									mused.jump_in_pattern = true;
								}
								
								set_repeat_timer(NULL); // ugh
							}
						}
						
						else
						{
							if (sp && event->type == SDL_MOUSEBUTTONDOWN && tmp.y - header.y > HEADER_HEIGHT + 3)
							{
								check_event(event, &tmp, select_pattern_param, MAKEPTR(param), MAKEPTR(sp->position + step), MAKEPTR(channel));
								
								set_repeat_timer(NULL); // ugh
							}
						}
					}
					
					pos.x += pos.w;
					
					if (channel != mused.current_sequencetrack && (mused.flags & EXPAND_ONLY_CURRENT_TRACK))
						break;
				}
			}
			
			if ((mused.flags & SONG_PLAYING) && !(mused.flags & FOLLOW_PLAY_POSITION))
			{
				gfx_domain_set_clip(dest_surface, &track);
				SDL_Rect pos = { track.x, (mused.stat_song_position - top) * height + track.y - 1, ((channel == mused.current_sequencetrack) ? w : narrow_w), 2 };
				bevel(dest_surface, &pos, mused.slider_bevel, BEV_SEQUENCE_PLAY_POS);
			}
		}
		
		SDL_Rect muted_mask = { 0 };
		copy_rect(&muted_mask, &track);
		
		muted_mask.x -= 1;
		muted_mask.w += 2;
		
		gfx_domain_set_clip(dest_surface, &muted_mask);
		
		if(mused.mus.channel[channel].flags & MUS_CHN_DISABLED)
		{
			gfx_translucent_rect(domain, &muted_mask, 128 << 24);
		}
		
		SDL_Rect mask;
		
		copy_rect(&mask, &header);
		mask.h += 2;
		mask.w -= 1;
		mask.x += 1;
		gfx_domain_set_clip(dest_surface, &mask);
		gfx_rect(dest_surface, &mask, colors[COLOR_OF_BACKGROUND]);
		gfx_domain_set_clip(dest_surface, &header);
		
		track.x -= 3;
		track.y += HEADER_HEIGHT - 7;
		track.h += 6;
		track.w += 6;
		gfx_domain_set_clip(dest_surface, &track);
		bevelex(dest_surface, &track, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL|BEV_F_DISABLE_CENTER);
		adjust_rect(&track, 3);
		
		gfx_domain_set_clip(dest_surface, &header);
		pattern_view_header(dest_surface, &header, event, channel);
		
		if ((mused.flags & SONG_PLAYING) && !(mused.flags & DISABLE_VU_METERS))
		{
			gfx_domain_set_clip(dest_surface, &track);
			const int ah = dest->h + dest->y - row.y;
			const int w = mused.vu_meter->surface->w;
			const int h = my_min(mused.vu_meter->surface->h, mused.vis.cyd_env[channel] * ah / MAX_VOLUME);
			SDL_Rect r = { track.x + track.w / 2 - w / 2 , row.y - h, w, h };
			SDL_Rect sr = { 0, mused.vu_meter->surface->h - h, mused.vu_meter->surface->w, h };
			gfx_blit(mused.vu_meter, &sr, dest_surface, &r);
		}
		
		//oscilloscope per track
		if(mused.flags2 & SHOW_OSCILLOSCOPES_PATTERN_EDITOR)
		{
			gfx_domain_set_clip(dest_surface, &track);
			
			SDL_Rect area;
			copy_rect(&area, &track);
			
			//area.y += 15;
			area.h = my_min(track.w * 2 / 3, 104);
			area.w = my_min(track.w, 104 * 3 / 2);
			
			bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
			adjust_rect(&area, 2);

			int *pointer = &mused.channels_output_buffer_counters[channel];
			
			update_oscillscope_view(dest_surface, &area, mused.channels_output_buffers[channel], area.w / 8, pointer, false, (bool)(mused.flags2 & SHOW_OSCILLOSCOPE_MIDLINES));
			//void update_oscillscope_view(GfxDomain *dest, const SDL_Rect* area, int* sound_buffer, int size, int* buffer_counter, bool is_translucent, bool show_midlines);
		}
		
		if(mused.flags2 & SHOW_REGISTERS_MAP)
		{
			registers_map_x_offset = track.x + track.w;
			registers_map_y_offset = track.y;
		}
	}
	
	//debug("big big loop finished");
	
	SDL_Rect pat;
	copy_rect(&pat, dest);
	adjust_rect(&pat, 2);
	gfx_domain_set_clip(dest_surface, &pat);
	
	gfx_domain_set_clip(dest_surface, NULL);
	
	if (mused.focus == EDITSEQUENCE)
	{
		// hack to display cursor both in sequence editor and here
		
		int x = 0;
		int w = char_width * 2;
		
		if (mused.flags & EDIT_SEQUENCE_DIGITS)
		{
			x = mused.sequence_digit * char_width;
			w = char_width;
		}
		
		int narrow_width1 = 0;
			
		for(int i = mused.pattern_horiz_position; i < mused.current_sequencetrack; ++i)
		{
			narrow_width1 += mused.widths[i][1];
		}
		
		SDL_Rect cursor = { 3 + dest->x + narrow_width1 + x, row.y, w, row.h}; //SDL_Rect cursor = { 3 + dest->x + narrow_w * (mused.current_sequencetrack - mused.pattern_horiz_position) + x, row.y, w, row.h};
		adjust_rect(&cursor, -2);
		bevelex(dest_surface, &cursor, mused.slider_bevel, (mused.flags & EDIT_MODE) ? BEV_EDIT_CURSOR : BEV_CURSOR, BEV_F_STRETCH_ALL);
	}
	
	// ach
	
	if (event->type == SDL_MOUSEWHEEL && mused.focus == EDITPATTERN)
	{
		if (event->wheel.y > 0)
		{
			mused.pattern_position -= 1;
		}
		
		else
		{
			mused.pattern_position += 1;
		}
		
		mused.pattern_position = my_max(0, my_min(mused.song.song_length - 1, mused.pattern_position));
	}
	
	if(mused.flags2 & SHOW_REGISTERS_MAP)
	{
		SDL_Rect reg_map;
		copy_rect(&reg_map, dest);
		
		reg_map.x = registers_map_x_offset + 16;
		reg_map.y = registers_map_y_offset - 8;
		
		reg_map.h = 3000;
		reg_map.w = 2000;
		
		//debug("x %d y %d", reg_map.x, reg_map.y);
		
		pattern_view_registers_map(dest_surface, &reg_map);
	}
	
	font_set_color(&mused.tinyfont, colors[COLOR_MAIN_TEXT]);
}

static void pattern_view_stepcounter(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event)
{
	SDL_Rect content;
	copy_rect(&content, dest);
	
	SDL_Rect header, frame, compact;
	
	content.y += HEADER_HEIGHT;
	content.h -= HEADER_HEIGHT;
	copy_rect(&frame, &content);
	
	int pixel_offset = 0;
	
	if((mused.flags & SONG_PLAYING) && (mused.flags2 & SMOOTH_SCROLL) && (mused.flags & FOLLOW_PLAY_POSITION))
	{
		pixel_offset = (Uint32)mused.mus.song_counter * (Uint32)8 / (((Uint32)mused.mus.song_position & (Uint32)1) ? (Uint32)mused.song.song_speed2 : (Uint32)mused.song.song_speed) - ((Uint32)mused.draw_passes_since_song_start == (Uint32)0 ? (Uint32)0 : ((Uint32)mused.draw_passes_since_song_start < (Uint32)4 ? (Uint32)mused.draw_passes_since_song_start : (Uint32)4));
	}
	
	content.y -= pixel_offset;
	content.y -= 8;
	content.h += 8;
	
	adjust_rect(&content, 2);
	console_set_clip(mused.console, &content);
	console_clear(mused.console);
	
	adjust_rect(&content, 2);
	console_set_clip(mused.console, &content);
	console_set_background(mused.console, 0);
	
	int start = mused.pattern_position - dest->h / mused.console->font.h / 2;
	
	int y = 0;
	
	for (int row = start - 1; y < content.h; ++row, y += mused.console->font.h)
	{
		if (mused.pattern_position == row)
		{
			SDL_Rect row = { content.x - 2, content.y + y - 1 + pixel_offset, content.w + 4, mused.console->font.h + 1};
			bevelex(dest_surface, &row, mused.slider_bevel, BEV_SELECTED_PATTERN_ROW, BEV_F_STRETCH_ALL);
		}
	}
	
	y = 0;
	
	for (int row = start - 1; y < content.h; ++row, y += mused.console->font.h)
	{
		if (row < 0 || row >= mused.song.song_length)
		{
			console_set_color(mused.console, colors[COLOR_PATTERN_DISABLED]);
		}
		
		else
		{
			console_set_color(mused.console, ((row == mused.pattern_position) ? colors[COLOR_PATTERN_SELECTED] : timesig(row, colors[COLOR_PATTERN_BAR], colors[COLOR_PATTERN_BEAT], colors[COLOR_PATTERN_NORMAL])));
		}
		
		if (mused.flags & SHOW_DECIMALS)
			console_write_args(mused.console, "%04d\n", (row + 10000) % 10000); // so we don't get negative numbers
		else //console_write_args(mused.console, "%03d\n", (row + 1000) % 1000);
			console_write_args(mused.console, "%04X\n", row & 0xffff); //console_write_args(mused.console, "%03X\n", row & 0xfff);
	}
	
	copy_rect(&header, dest);
	header.h = HEADER_HEIGHT;
	header.w -= 2;
	
	copy_rect(&compact, &header);
	adjust_rect(&compact, 2);
	
	compact.w /= 2;
	
	bevelex(dest_surface, &header, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	
	button_event(dest_surface, event, &compact, mused.slider_bevel, 
                !(mused.flags & COMPACT_VIEW) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, 
                !(mused.flags & COMPACT_VIEW) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, 
                (mused.flags & COMPACT_VIEW) ? DECAL_COMPACT_SELETED : DECAL_COMPACT, flip_bit_action, &mused.flags, MAKEPTR(COMPACT_VIEW), 0);
	
	compact.x += compact.w;
	
	button_event(dest_surface, event, &compact, mused.slider_bevel, 
                !(mused.flags & EXPAND_ONLY_CURRENT_TRACK) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, 
                !(mused.flags & EXPAND_ONLY_CURRENT_TRACK) ? BEV_BUTTON : BEV_BUTTON_ACTIVE, 
                (mused.flags & EXPAND_ONLY_CURRENT_TRACK) ? DECAL_FOCUS_SELETED : DECAL_FOCUS, flip_bit_action, &mused.flags, MAKEPTR(EXPAND_ONLY_CURRENT_TRACK), 0);
	
	bevelex(dest_surface, &frame, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL|BEV_F_DISABLE_CENTER);
	
	gfx_domain_set_clip(dest_surface, NULL);
}


void pattern_view2(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect pat, pos;
	copy_rect(&pat, dest);
	copy_rect(&pos, dest);
		
	pat.w -= 38;
	pat.x += 38;
	
	/*pat.w -= 30;
	pat.x += 30;*/
	
	pos.w = 40; //pos.w = 32;
		
	pattern_view_stepcounter(dest_surface, &pos, event);
	pattern_view_inner(dest_surface, &pat, event);
	
	gfx_domain_set_clip(dest_surface, NULL);
	SDL_Rect scrollbar = { dest->x, dest->y + dest->h - SCROLLBAR, dest->w, SCROLLBAR };
	
	slider(dest_surface, &scrollbar, event, &mused.pattern_horiz_slider_param); 
}