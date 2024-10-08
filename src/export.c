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

#include "export.h"
#include "gui/bevel.h"
#include "snd/cyd.h"
#include "macros.h"
#include "mused.h"
#include "gfx/gfx.h"
#include "gui/view.h"
#include "mybevdefs.h"
#include "gfx/font.h"
#include "theme.h"
#include <string.h>
#include "wavewriter.h"

extern GfxDomain *domain;

bool export_wav(MusSong *song, CydWavetableEntry * entry, FILE *f, int channel)
{
	bool success = false;
	
	/*MusEngine mus;
	CydEngine cyd;*/
	
	MusEngine* mus = (MusEngine*)calloc(1, sizeof(MusEngine));
	CydEngine* cyd = (CydEngine*)calloc(1, sizeof(CydEngine));
	
	if(mus == NULL || cyd == NULL)
	{
		return success;
	}
	
	cyd_init(cyd, 44100, song->num_channels); //cyd_init(&cyd, 100000, 64);
	cyd->flags |= CYD_SINGLE_THREAD;
	mus_init_engine(mus, cyd);
	mus->volume = song->master_volume;
	mus_set_fx(mus, song);
	CydWavetableEntry * prev_entry = cyd->wavetable_entries; // save entries so they can be free'd
	cyd->wavetable_entries = entry;
	cyd_set_callback(cyd, mus_advance_tick, mus, song->song_rate);
	mus_set_song(mus, song, 0);
	//song->flags |= MUS_NO_REPEAT;
	
	if (channel >= 0)
	{
		// if channel is positive then only export that channel (mute other chans)
		
		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
			mus->channel[i].flags |= MUS_CHN_DISABLED;
		
		mus->channel[channel].flags &= ~MUS_CHN_DISABLED;
	}
	
	else
	{
		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
			mus->channel[i].flags &= ~MUS_CHN_DISABLED;
	}
	
	const int channels = 2;
	Sint16 buffer[2000 * channels]; //was Sint16 buffer[2000 * channels];
	
	int last_percentage = -1;
	
	WaveWriter *ww = ww_create(f, cyd->sample_rate, 2);

	/*int array[2] = { 1, 2 }; 
	int value = 69; 
	array[1] = 0; 
	debug("%d should be 69 and %d is song position, array[0] = %d, array[1] = %d", value, mus.song_position, array[0], array[1]);*/

	for (;;)
	{
		memset(buffer, 0, sizeof(buffer)); // Zero the input to cyd
		
		//debug("Successful memset");
		
		cyd_output_buffer_stereo(cyd, (Uint8*)buffer, sizeof(buffer));
		
		//debug("Successful cyd_output_buffer_stereo"); //wasn't there
		
		if (cyd->samples_output > 0)
			ww_write(ww, buffer, cyd->samples_output);
		
		//debug("Successful ww_write and song pos is %d", mus.song_position);
		
		if (mus->song_position >= song->song_length)
		{
			debug("finished");
			//mus.song_position = -1;
			break;
		}
		
		if (song->song_length != 0)
		{
			SDL_Rect area = {domain->screen_w / 2 - 140, domain->screen_h / 2 - 24, 280, 48};
			int percentage = (mus->song_position + (channel == -1 ? 0 : (channel * song->song_length))) * (area.w - (8 + 2) * 2) / (song->song_length * (channel == -1 ? 1 : song->num_channels));
			
			if (percentage > last_percentage)
			{
				last_percentage = percentage;
				
				bevel(domain, &area, mused.slider_bevel, BEV_MENU);
				
				adjust_rect(&area, 8);
				area.h = 16;
				
				bevel(domain, &area, mused.slider_bevel, BEV_FIELD);
				
				adjust_rect(&area, 2);
				
				int t = area.w;
				area.w = area.w * percentage / area.w;
				
				gfx_rect(domain, &area, colors[COLOR_PROGRESS_BAR]);
				
				area.y += 16 + 4 + 4;
				area.w = t;
				
				font_write_args(&mused.smallfont, domain, &area, "Exporting... Press ESC to abort.");
				
				SDL_Event e;
				
				while (SDL_PollEvent(&e))
				{
					if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
					{
						goto abort;
					}
				}
				
				gfx_domain_flip(domain);
			}
		}
	}
	
	success = true;
	
abort:;
	
	ww_finish(ww);
	
	cyd->wavetable_entries = prev_entry;
	
	cyd_deinit(cyd);
	
	song->flags &= ~MUS_NO_REPEAT;
	
	free(cyd);
	free(mus);
	
	return success;
}

bool export_wav_hires(MusSong *song, CydWavetableEntry * entry, FILE *f, int channel)
{
	bool success = false;
	
	/*MusEngine mus;
	CydEngine cyd;*/
	
	MusEngine* mus = (MusEngine*)calloc(1, sizeof(MusEngine));
	CydEngine* cyd = (CydEngine*)calloc(1, sizeof(CydEngine));
	
	if(mus == NULL || cyd == NULL)
	{
		return success;
	}
	
	cyd_init(cyd, 384000, song->num_channels); //cyd_init(&cyd, 100000, 64);
	
	cyd->flags |= CYD_SINGLE_THREAD;
	mus_init_engine(mus, cyd);
	mus->volume = song->master_volume;
	
	mus_set_fx(mus, song);
	
	CydWavetableEntry * prev_entry = cyd->wavetable_entries; // save entries so they can be free'd
	cyd->wavetable_entries = entry;
	cyd_set_callback(cyd, mus_advance_tick, mus, song->song_rate);
	
	mus_set_song(mus, song, 0);
	//song->flags |= MUS_NO_REPEAT;
	
	if (channel >= 0)
	{
		// if channel is positive then only export that channel (mute other chans)
		
		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
			mus->channel[i].flags |= MUS_CHN_DISABLED;
		
		mus->channel[channel].flags &= ~MUS_CHN_DISABLED;
	}
	
	else
	{
		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
			mus->channel[i].flags &= ~MUS_CHN_DISABLED;
	}
	
	const int channels = 2;
	Sint16 buffer[2000 * channels]; //was Sint16 buffer[2000 * channels];
	
	int last_percentage = -1;
	
	WaveWriter *ww = ww_create(f, cyd->sample_rate, 2);

	/*int array[2] = { 1, 2 }; 
	int value = 69; 
	array[1] = 0; 
	debug("%d should be 69 and %d is song position, array[0] = %d, array[1] = %d", value, mus.song_position, array[0], array[1]);*/

	for (;;)
	{
		//debug("before memset");
		
		memset(buffer, 0, sizeof(buffer)); // Zero the input to cyd
		
		//debug("Successful memset");
		
		cyd_output_buffer_stereo(cyd, (Uint8*)buffer, sizeof(buffer));
		
		//debug("Successful cyd_output_buffer_stereo"); //wasn't there
		
		if (cyd->samples_output > 0)
			ww_write(ww, buffer, cyd->samples_output);
		
		//debug("Successful ww_write and song pos is %d", mus.song_position);
		
		if (mus->song_position >= song->song_length)
		{
			debug("finished");
			//mus.song_position = -1;
			break;
		}
		
		if (song->song_length != 0)
		{
			SDL_Rect area = {domain->screen_w / 2 - 140, domain->screen_h / 2 - 24, 280, 48};
			int percentage = (mus->song_position + (channel == -1 ? 0 : (channel * song->song_length))) * (area.w - (8 + 2) * 2) / (song->song_length * (channel == -1 ? 1 : song->num_channels));
			
			if (percentage > last_percentage)
			{
				last_percentage = percentage;
				
				bevel(domain, &area, mused.slider_bevel, BEV_MENU);
				
				adjust_rect(&area, 8);
				area.h = 16;
				
				bevel(domain, &area, mused.slider_bevel, BEV_FIELD);
				
				adjust_rect(&area, 2);
				
				int t = area.w;
				area.w = area.w * percentage / area.w;
				
				gfx_rect(domain, &area, colors[COLOR_PROGRESS_BAR]);
				
				area.y += 16 + 4 + 4;
				area.w = t;
				
				font_write_args(&mused.smallfont, domain, &area, "Exporting... Press ESC to abort.");
				
				SDL_Event e;
				
				while (SDL_PollEvent(&e))
				{
					if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
					{
						goto abort;
					}
				}
				
				gfx_domain_flip(domain);
			}
		}
	}
	
	success = true;
	
abort:;
	
	ww_finish(ww);
	
	cyd->wavetable_entries = prev_entry;
	
	cyd_deinit(cyd);
	
	song->flags &= ~MUS_NO_REPEAT;
	
	free(cyd);
	free(mus);
	
	return success;
}