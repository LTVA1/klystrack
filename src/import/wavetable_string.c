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

#include "wavetable_string.h"
#include "../view.h"
#include "stdlib.h"

extern Mused mused;
extern GfxDomain *domain;

bool quit;
bool generate_macro;

#define WINDOW_HEIGHT 55
#define WINDOW_WIDTH 250

draw_bit_depth_select_window(Uint8* bit_depth, SDL_Event* event)
{
	SDL_Rect area = { domain->screen_w / 2 - WINDOW_WIDTH / 2, domain->screen_h / 2 - WINDOW_HEIGHT / 2, WINDOW_WIDTH, WINDOW_HEIGHT };
	
	bevel(domain, &area, mused.slider_bevel, BEV_MENU);
	
	adjust_rect(&area, 8);
	
	SDL_Rect r;
	copy_rect(&r, &area);
	
	r.h = 10;
	
	*bit_depth += generic_field(event, &r, -1, -1, "SELECT WAVETABLE BIT DEPTH:", "%3d", MAKEPTR(*bit_depth), 3);
	r.y += 12;
	generic_flags(event, &r, 0, 0, "GENERATE LOOP-THROUGH MACRO", &generate_macro, 1);
	
	r.h = 12;
	r.w = 40;
	r.y = area.y + area.h - r.h;
	
	r.x = area.x + area.w - r.w;
	
	int should_quit = button_text_event(domain, event, &r, mused.slider_bevel, &mused.buttonfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, "OK", NULL, MAKEPTR(1), NULL, NULL);
	
	if(should_quit & 1)
	{
		quit = true;
	}
}

Uint8 bit_depth_select_menu_view()
{
	Uint8 bit_depth = 4; //most common chiptune wavetable bit depth
	quit = false;
	generate_macro = false;
	
	while (!quit)
	{
		SDL_Event e = { 0 };
		int got_event = 0;
		
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				case SDL_QUIT:
				
				set_repeat_timer(NULL);
				SDL_PushEvent(&e);
				
				return bit_depth;
				
				break;
				
				case SDL_WINDOWEVENT:
				{
					switch (e.window.event) 
					{
						case SDL_WINDOWEVENT_RESIZED:
						{
							domain->screen_w = my_max(320, e.window.data1 / domain->scale);
							domain->screen_h = my_max(240, e.window.data2 / domain->scale);
							
							if (!(mused.flags & FULLSCREEN))
							{
								mused.window_w = domain->screen_w * domain->scale;
								mused.window_h = domain->screen_h * domain->scale;
							}
							
							gfx_domain_update(domain, false);
						}
						break;
					}
					break;
				}
				
				case SDL_KEYDOWN:
				{
					switch (e.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						
						set_repeat_timer(NULL);
						return bit_depth;
						
						break;
						
						default: break;
					}
				}
				break;
			
				case SDL_USEREVENT:
					e.type = SDL_MOUSEBUTTONDOWN;
				break;
				
				case SDL_MOUSEMOTION:
					if (domain)
					{
						e.motion.xrel /= domain->scale;
						e.motion.yrel /= domain->scale;
						e.button.x /= domain->scale;
						e.button.y /= domain->scale;
					}
				break;
				
				case SDL_MOUSEBUTTONDOWN:
					if (domain)
					{
						e.button.x /= domain->scale;
						e.button.y /= domain->scale;
					}
				break;
				
				case SDL_MOUSEBUTTONUP:
				{
					if (e.button.button == SDL_BUTTON_LEFT)
						mouse_released(&e);
				}
				break;
			}
			
			if (e.type != SDL_MOUSEMOTION || (e.motion.state)) ++got_event;
			
			// Process mouse click events immediately, and batch mouse motion events
			// (process the last one) to fix lag with high poll rate mice on Linux.
			//fix from here https://github.com/kometbomb/klystrack/pull/300
			if (should_process_mouse(&e))
				break;
		}
		
		if (got_event || gfx_domain_is_next_frame(domain))
		{
			draw_bit_depth_select_window(&bit_depth, &e);
			gfx_domain_flip(domain);
		}
		else
			SDL_Delay(5);
	}
	
	return bit_depth;
}

void import_wavetable_string(MusInstrument* inst)
{
	if (confirm(domain, mused.slider_bevel, &mused.largefont, "  Import wavetable string to current instrument?\nIt will overwrite current instrument local samples!"))
	{
		char* wave_string = SDL_GetClipboardText();
		
		debug("got wavetable string: \"%s\"", wave_string);
		
		if(strcmp(wave_string, "") == 0)
		{
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Clipboard is empty!", MB_OK);
			
			goto end;
		}
		
		Uint8 bit_depth = bit_depth_select_menu_view();
		
		debug("selected bit depth: %d", bit_depth);
		
		char* wave_string_copy = (char*)calloc(1, strlen(wave_string) + 1);
		strcpy(wave_string_copy, wave_string);
		
		const char delimiters_lines[] = "\n\r;";
		const char delimiters[] = ", \t";
		
		Uint16 passes = 0;
		Uint16 wavetables = 0;
		Uint16 wavetable_length = 0;
		
		char* current_line = 1;
		
		char** lines = (char**)calloc(1, sizeof(char*));
		
		Uint16** wavetable_arrays = (Uint16**)calloc(1, sizeof(Uint16*));
		wavetable_arrays[0] = NULL;
		
		Uint16* wavetable_lengths = NULL;
		Sint16* wave_data = NULL;
		
		while(current_line != NULL)
		{
			current_line = strtok(passes > 0 ? NULL : wave_string_copy, delimiters_lines);
			passes++;
			
			if(current_line != NULL)
			{
				if(strcmp(current_line, "") != 0)
				{
					wavetables++;
					
					lines = (char**)realloc(lines, wavetables * sizeof(char*));
					lines[wavetables - 1] = current_line;
					
					wavetable_arrays = (Uint16**)realloc(wavetable_arrays, wavetables * sizeof(Uint16*));
					wavetable_arrays[wavetables - 1] = NULL;
				}
			}
		}
		
		if(wavetables > MUS_MAX_INST_SAMPLES)
		{
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Too many wavetables!", MB_OK);
			
			goto end;
		}
		
		if(wavetables == 0)
		{
			msgbox(domain, mused.slider_bevel, &mused.largefont, "No wavetables found!", MB_OK);
			
			goto end;
		}
		
		debug("wavetables in string: %d", wavetables);
		
		wavetable_lengths = (Uint16*)calloc(1, sizeof(Uint16) * wavetables);
		
		for(int i = 0; i < wavetables; i++)
		{
			char* current_number = 1;
			
			Uint16 number_processed = 0;
			
			wavetable_arrays[i] = (Uint16*)calloc(1, sizeof(Uint16));
			
			Uint16 wavetable_position = 0;
			
			while(current_number != NULL)
			{
				current_number = strtok(number_processed > 0 ? NULL : lines[i], delimiters);
				number_processed++;
				
				if(current_number != NULL)
				{
					if(strcmp(current_number, "") != 0)
					{
						wavetable_position++;
						
						wavetable_arrays[i] = (Uint16*)realloc(wavetable_arrays[i], wavetable_position * sizeof(Uint16));
						
						wavetable_arrays[i][wavetable_position - 1] = atoi(current_number);
					}
				}
			}
			
			wavetable_length = wavetable_position;
			
			wavetable_lengths[i] = wavetable_length;
		}
		
		for(int i = 0; i < wavetables; i++)
		{
			if(wavetable_length != wavetable_lengths[i])
			{
				msgbox(domain, mused.slider_bevel, &mused.largefont, "Wavetables have different length!", MB_OK);
			
				goto end;
			}
		}
		
		debug("wavetable length %d", wavetable_length);
		
		mus_free_inst_samples(inst);
		
		inst->local_samples = (CydWavetableEntry**)calloc(1, wavetables * sizeof(CydWavetableEntry*));
		inst->local_sample_names = (char**)calloc(1, wavetables * sizeof(char*));
		
		inst->num_local_samples = wavetables;
		
		wave_data = calloc(1, sizeof(Sint16) * wavetable_length);
		
		for(int i = 0; i < wavetables; i++) //finally process the wavetables
		{
			inst->local_samples[i] = (CydWavetableEntry*)calloc(1, sizeof(CydWavetableEntry));
			cyd_wave_entry_init(inst->local_samples[i], NULL, 0, 0, 0, 0, 0);
			
			inst->local_sample_names[i] = (char*)calloc(1, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
			memset(inst->local_sample_names[i], 0, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
			
			for(int j = 0; j < wavetable_length; j++)
			{
				wave_data[j] = (Sint64)wavetable_arrays[i][j] * 65536 / (1 << bit_depth) - 32768;
			}
			
			cyd_wave_entry_init(inst->local_samples[i], wave_data, wavetable_length, CYD_WAVE_TYPE_SINT16, 1, 1, 1);
			
			inst->local_samples[i]->base_note = inst->base_note << 8 + inst->finetune;
			inst->local_samples[i]->flags |= CYD_WAVE_LOOP | CYD_WAVE_NO_INTERPOLATION;
			
			inst->local_samples[i]->loop_begin = 0;
			inst->local_samples[i]->loop_end = wavetable_length;
			
			inst->local_samples[i]->sample_rate = get_freq(inst->base_note << 8 + inst->finetune) / 1024 * wavetable_length;
		}
		
		if(generate_macro)
		{
			int cycle_through_macro = -1;
			
			for(int i = 0; i < inst->num_macros; i++)
			{
				if(is_empty_program(inst->program[i]))
				{
					cycle_through_macro = i;
				}
			}
			
			if(cycle_through_macro == -1) //all programs are occupied, check if we can create a new one
			{
				if(inst->num_macros == MUS_MAX_MACROS_INST)
				{
					msgbox(domain, mused.slider_bevel, &mused.largefont, "Too many macros already occupied,\ncan't create a new one for cycling!", MB_OK);
			
					goto end;
				}
				
				inst->num_macros++; //we can so create new macro
				
				cycle_through_macro = inst->num_macros - 1;
				
				inst->program[inst->num_macros - 1] = calloc(1, MUS_PROG_LEN * sizeof(Uint16));
				inst->program_unite_bits[inst->num_macros - 1] = calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
				
				memset(inst->program_unite_bits[inst->num_macros - 1], 0, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
				
				for(int i = 0; i < MUS_PROG_LEN; i++)
				{
					inst->program[inst->num_macros - 1][i] = MUS_FX_NOP;
				}
			}
			
			for(int i = 0; i < wavetables; i++)
			{
				inst->program[cycle_through_macro][i] = MUS_FX_SET_LOCAL_SAMPLE | (i & 0xff);
			}
			
			if(wavetables < MUS_PROG_LEN)
			{
				inst->program[cycle_through_macro][wavetables] = MUS_FX_JUMP; //jump to beginning
			}
		}
		
		invalidate_wavetable_view();
		
		inst->flags |= MUS_INST_USE_LOCAL_SAMPLES;
		
		for(int i = 0; i < wavetables; i++)
		{
			if(wavetable_arrays[i])
			{
				free(wavetable_arrays[i]);
			}
		}
		
		end:;
		
		if(wave_data)
		{
			free(wave_data);
		}
		
		if(lines)
		{
			free(lines);
		}
		
		if(wavetable_lengths)
		{
			free(wavetable_lengths);
		}
		
		SDL_free(wave_string);
		
		if(wave_string_copy)
		{
			free(wave_string_copy);
		}
		
		if(wavetable_arrays)
		{
			free(wavetable_arrays);
		}
	}
}