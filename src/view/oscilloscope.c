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

#include "oscilloscope.h"
#include <signal.h>

extern Mused mused;

void update_oscillscope_view(GfxDomain *dest, const SDL_Rect* area, int* sound_buffer, int size, int* buffer_counter, bool is_translucent, bool show_midlines)
{
	if(*buffer_counter >= (size == OSC_SIZE ? size * 8 : size * 25))
	{
		*buffer_counter = 0;
	}
	
	if(is_translucent)
	{
		for(int x = 0; x < area->h / 2; x++) //drawing black lines every two pixels so bevel is partially hidden when it is under oscilloscope
		{
			gfx_line(domain, area->x, area->y + 2 * x, area->x + area->w - 1, area->y + 2 * x, colors[COLOR_WAVETABLE_BACKGROUND]);
		}
	}
	
	else
	{
		gfx_rect(dest, area, colors[COLOR_WAVETABLE_BACKGROUND]);
	}

	Sint32 sample, last_sample, scaled_sample;
	
	for(int i = (size == OSC_SIZE ? size * 2 : size * 16) + 10; i < (size == OSC_SIZE ? size * 8 : size * 25); i++)
	{
		if((sound_buffer[i + 1] > TRIGGER_LEVEL) && (sound_buffer[i] >= TRIGGER_LEVEL) && (sound_buffer[i - 1] <= TRIGGER_LEVEL) && (sound_buffer[i - 2] < TRIGGER_LEVEL)) //&& (abs(mused.output_buffer[i] - mused.output_buffer[i - 1]) < 1000))
		{
			for (int x = i - (size == OSC_SIZE ? area->w : 2 * area->w); x < (size == OSC_SIZE ? area->w : 2 * area->w) + i; ++x)
			{
				if(!(x & (size == OSC_SIZE ? 1 : 3)))
				{
					last_sample = scaled_sample;
					sample = sound_buffer[x];
				}
					
				if(!(x & (size == OSC_SIZE ? 1 : 3))) //(size == OSC_SIZE ? 2 : 4) (size == OSC_SIZE ? area->w : 2 * area->w) (size == OSC_SIZE ? 1 : 3)
				{
					scaled_sample = sample * size / (OSC_SIZE * 150);
					
					if(x != i - (size == OSC_SIZE ? area->w : 2 * area->w) && x != i - (size == OSC_SIZE ? area->w : 2 * area->w) + 1 && (size == OSC_SIZE ? 1 : (x != i - 2 * area->w + 2)) && (size == OSC_SIZE ? 1 : (x != i - 2 * area->w + 3)))
					{
						gfx_line(domain, area->x + (x - i + (size == OSC_SIZE ? 1 : 2) * area->w) / (size == OSC_SIZE ? 2 : 4) - 1, area->h / 2 + area->y - my_min(my_max(last_sample, area->h / (-2)), area->h / 2), area->x + (x - i + (size == OSC_SIZE ? 1 : 2) * area->w) / (size == OSC_SIZE ? 2 : 4), area->h / 2 + area->y - my_min(my_max(scaled_sample, area->h / (-2)), area->h / 2), colors[COLOR_WAVETABLE_SAMPLE]);
					}
				}
			}
			
			if(show_midlines)
			{
				for(int y = 0; y < area->h / 2; ++y) //vertical midline
				{
					gfx_line(domain, area->x + area->w / 2, area->y + 2 * y, area->x + area->w / 2, area->y + 2 * y, colors[COLOR_PATTERN_CTRL]);
				}
			
				for(int x = 0; x < area->w / 2; ++x) //horizontal midline
				{
					gfx_line(domain, area->x + 2 * x, area->y + area->h / 2, area->x + 2 * x, area->y + area->h / 2, colors[COLOR_PATTERN_CTRL]);
				}
			}
				
			return;
		}
	}
	
	/* Below is a dirty hack. This debug would not actually do anything because mused.output_buffer_counter can be anything from 0 to 8191.
	It is done because for some reason if I compile the code with -O2 or -O3 flags part of the function that is below would not execute
	even if it is supposed to without this debug thing. Very strange, actually. */
	
	if(*buffer_counter == -1)
	{
		debug("Trigger values:");
	}
	
	for (int x = 0; x < area->w; ++x)
	{
		last_sample = scaled_sample;
		sample = (sound_buffer[2 * x] + sound_buffer[2 * x + 1]) / 2;
		
		scaled_sample = sample * size / (OSC_SIZE * 150);
		
		if(x != 0)
		{
			gfx_line(domain, area->x + x - 1, area->h / 2 + area->y - my_min(my_max(last_sample, area->h / (-2)), area->h / 2), area->x + x, area->h / 2 + area->y - my_min(my_max(scaled_sample, area->h / (-2)), area->h / 2), colors[COLOR_WAVETABLE_SAMPLE]);
		}
	}
	
	if(show_midlines)
	{
		for(int y = 0; y < area->h / 2; ++y) //vertical midline
		{
			gfx_line(domain, area->x + area->w / 2, area->y + 2 * y, area->x + area->w / 2, area->y + 2 * y, colors[COLOR_PATTERN_CTRL]);
		}
	
		for(int x = 0; x < area->w / 2; ++x) //horizontal midline
		{
			gfx_line(domain, area->x + 2 * x, area->y + area->h / 2, area->x + 2 * x, area->y + area->h / 2, colors[COLOR_PATTERN_CTRL]);
		}
	}
}