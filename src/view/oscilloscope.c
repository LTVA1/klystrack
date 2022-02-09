#include "oscilloscope.h"
#include <signal.h>

extern Mused mused;

void update_oscillscope_view(GfxDomain *dest, const SDL_Rect* area)
{
	if(mused.output_buffer_counter >= OSC_SIZE * 8)
	{
		mused.output_buffer_counter = 0;
	}
	
	for(int x = 0; x < area->h * 0.5; x++) //drawing black lines every two pixels so bevel is partially hidden when it is under oscilloscope
	{
		gfx_line(domain, area->x, area->y + 2 * x, area->x + area->w - 1, area->y + 2 * x, colors[COLOR_WAVETABLE_BACKGROUND]);
	}
	
	Sint32 sample, last_sample, scaled_sample;
	
	for(int i = OSC_SIZE * 2 + 10; i < OSC_SIZE * 4; i++)
	{
		if((mused.output_buffer[i + 1] > TRIGGER_LEVEL) && (mused.output_buffer[i] >= TRIGGER_LEVEL) && (mused.output_buffer[i - 1] <= TRIGGER_LEVEL) && (mused.output_buffer[i - 2] < TRIGGER_LEVEL)) //&& (abs(mused.output_buffer[i] - mused.output_buffer[i - 1]) < 1000))
		{
			//here comes the part with triggering
			
			//if(mused.output_buffer[i] != 0)
			//{
				//debug("Trigger values: [i-2]: %d, [i-1]: %d, [i]: %d, [i+1]: %d, [i+2]: %d, [i+3]: %d, [i+4]: %d, i: %d", mused.output_buffer[i - 2], mused.output_buffer[i - 1], mused.output_buffer[i], mused.output_buffer[i + 1], mused.output_buffer[i + 2], mused.output_buffer[i + 3], mused.output_buffer[i + 4], i);
			//}
			
			for (int x = i - area->w; x < area->w + i; ++x)
			{
				if(!(x & 1))
				{
					last_sample = scaled_sample;
					sample = mused.output_buffer[x];
				}
					
				if(sample > OSC_MAX_CLAMP)
				{
					sample = OSC_MAX_CLAMP;
				}
							
				if(sample < -OSC_MAX_CLAMP)
				{
					sample = -OSC_MAX_CLAMP;
				}
							
				if(last_sample > OSC_MAX_CLAMP)
				{
					last_sample = OSC_MAX_CLAMP;
				}
							
				if(last_sample < -OSC_MAX_CLAMP)
				{
					last_sample = -OSC_MAX_CLAMP;
				}
					
				if(!(x & 1))
				{
					scaled_sample = (sample * OSC_SIZE) / 32768;
					
					if(x != i - area->w && x != i - area->w + 1)
					{
						gfx_line(domain, area->x + (x - i + area->w) / 2 - 1, area->h / 2 + area->y + last_sample, area->x + (x - i + area->w) / 2, area->h / 2 + area->y + scaled_sample, colors[COLOR_WAVETABLE_SAMPLE]);
					}
				}
			}
				
			return;
		}
	}
	
	/* Below is a dirty hack. This debug would not actually do anything because mused.output_buffer_counter can be anything from 0 to 8191.
	It is done because for some reason if I compile the code with -O2 or -O3 flags part of the function that is below would not execute
	even if it is supposed to without this debug thing. Very strange, actually. */
	
	if(mused.output_buffer_counter == -1)
	{
		debug("Trigger values:");
	}
	
	for (int x = 0; x < area->w; ++x)
	{
		last_sample = scaled_sample;
		sample = (mused.output_buffer[2 * x] + mused.output_buffer[2 * x + 1]) / 2;
		
		if(sample > OSC_MAX_CLAMP)
		{
			sample = OSC_MAX_CLAMP;
		}
				
		if(sample < -OSC_MAX_CLAMP)
		{
			sample = -OSC_MAX_CLAMP;
		}
				
		if(last_sample > OSC_MAX_CLAMP)
		{
			last_sample = OSC_MAX_CLAMP;
		}
				
		if(last_sample < -OSC_MAX_CLAMP)
		{
			last_sample = -OSC_MAX_CLAMP;
		}
			
		scaled_sample = (sample * OSC_SIZE) / 32768;
		
		if(x != 0)
		{
			gfx_line(domain, area->x + x - 1, area->h / 2 + area->y + last_sample, area->x + x, area->h / 2 + area->y + scaled_sample, colors[COLOR_WAVETABLE_SAMPLE]);
		}
	}
}