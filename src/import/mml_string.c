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

#include "mml_string.h"
#include "stdlib.h"

extern Mused mused;
extern GfxDomain *domain;

const char* chip_types[3] = 
{
	"OPL",
	"OPM",
	"OPN",
};

const Uint8 convert_ops_indices[8][4] = //klystrack and OPN/OPM alg schemes have different ops indices, we need to account for that
{
//ops:    1  2  3  4    <- ops in OPN/OPM

		{ 4, 3, 2, 1 }, //alg 0 <- ops in klystrack
		{ 4, 3, 2, 1 }, //alg 1 ...
		{ 4, 3, 2, 1 }, //alg 2
		{ 3, 2, 4, 1 }, //alg 3
		{ 4, 3, 2, 1 }, //alg 4
		{ 4, 3, 2, 1 }, //alg 5
		{ 4, 3, 2, 1 }, //alg 6
		{ 4, 3, 2, 1 }, //alg 7
};

static int draw_box_mml(GfxDomain *dest, GfxSurface *gfx, const Font *font, const SDL_Event *event, const char *msg, int buttons, int *selected)
{
	int w = 0, max_w = 200, h = font->h;
	
	for (const char *c = msg; *c; ++c)
	{
		w += font->w;
		max_w = my_max(max_w, w + 16);
		if (*c == '\n')
		{
			w = 0;
			h += font->h;
		}
	}
	
	h += 14 * MML_BUTTON_TYPES - 16;
	
	SDL_Rect area = { dest->screen_w / 2 - max_w / 2, dest->screen_h / 2 - h / 2 - 8, max_w, h + 16 + 16 + 4 };
	
	bevel(dest, &area, gfx, BEV_MENU);
	SDL_Rect content, pos;
	copy_rect(&content, &area);
	adjust_rect(&content, 8);
	copy_rect(&pos, &content);
	
	font_write(font, dest, &pos, msg);
	update_rect(&content, &pos);
	
	int b = 0;
	for (int i = 0; i < MML_BUTTON_TYPES; ++i)
		if (buttons & (1 << i)) ++b;
		
	*selected = (*selected + b) % b;
	
	//pos.w = 50;
	pos.w = content.w;
	pos.h = 14;
	pos.x = content.x;
	pos.y -= 14 * MML_BUTTON_TYPES;
	
	int r = 0;
	static const char *label[] = { "PMD", "FMP", "FMP7", "VOPM (OPM)", "NRTDRV", "MXDRV", "MMLDRV", "MUCOM88" };
	int idx = 0;
	
	for (int i = 0; i < MML_BUTTON_TYPES; ++i)
	{
		if (buttons & (1 << i))
		{
			int p = button_text_event(dest, event, &pos, gfx, font, BEV_BUTTON, BEV_BUTTON_ACTIVE, label[i], NULL, 0, 0, 0);
			
			if (idx == *selected)
			{	
				if (event->type == SDL_KEYDOWN && (event->key.keysym.sym == SDLK_SPACE || event->key.keysym.sym == SDLK_RETURN))
					p = 1;
			
				bevel(dest, &pos, gfx, BEV_CURSOR);
			}
			
			pos.y += 14;
			
			//update_rect(&content, &pos);
			if (p & 1) r = (1 << i);
			++idx;
		}
	}
	
	return r;
}

int msgbox_mml(GfxDomain *domain, GfxSurface *gfx, const Font *font, const char *msg, int buttons)
{
	set_repeat_timer(NULL);
	
	int selected = 0;
	
	SDL_StopTextInput();
	
	while (1)
	{
		SDL_Event e = { 0 };
		int got_event = 0;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				case SDL_WINDOWEVENT:
				{
					switch (e.window.event) 
					{
						case SDL_WINDOWEVENT_RESIZED:
						{
							debug("SDL_WINDOWEVENT_RESIZED %dx%d", e.window.data1, e.window.data2);

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
						
						return 0;
						
						break;
						
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
							return (1 << selected);
						break;
						
						case SDLK_LEFT:
						--selected;
						break;
						
						case SDLK_RIGHT:
						++selected;
						break;
						
						default: 
						break;
					}
				}
				break;
			
				case SDL_USEREVENT:
					e.type = SDL_MOUSEBUTTONDOWN;
				break;
				
				case SDL_MOUSEMOTION:
					gfx_convert_mouse_coordinates(domain, &e.motion.x, &e.motion.y);
					gfx_convert_mouse_coordinates(domain, &e.motion.xrel, &e.motion.yrel);
				break;
				
				case SDL_MOUSEBUTTONDOWN:
					gfx_convert_mouse_coordinates(domain, &e.button.x, &e.button.y);
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
			int r = draw_box_mml(domain, gfx, font, &e, msg, buttons, &selected);
			gfx_domain_flip(domain);
			set_repeat_timer(NULL);
			
			if (r) 
			{
				return r;
			}
		}
		
		else
		{
			SDL_Delay(5);
		}
	}
}

void process_mml_string(MusInstrument* inst, int dialect, char* mml_string)
{
	switch(dialect)
	{
		case BTN_PMD:
		{
			/* OPN: */
			
			//@ Instrument number ALG FB
			//AR DR SR RR SL TL KS ML DT AMS
			//AR DR SR RR SL TL KS ML DT AMS
			//AR DR SR RR SL TL KS ML DT AMS
			//AR DR SR RR SL TL KS ML DT AMS
			
			/* OPM: */
			
			//@ Instrument number ALG FB
			//AR DR SR RR SL TL KS ML DT DT2 AMS
			//AR DR SR RR SL TL KS ML DT DT2 AMS
			//AR DR SR RR SL TL KS ML DT DT2 AMS
			//AR DR SR RR SL TL KS ML DT DT2 AMS
			
			/* OPL: (?) */
			
			//@ Instrument number ALG FB
			//AR DR RR SL TL KSL ML KSR EGT VIB AM
			//AR DR RR SL TL KSL ML KSR EGT VIB AM
			
			/*Range	Format 1&2	ALG	— 0–7
			FB	— 0–7
			AR	— 0–31
			DR	— 0–31
			SR	— 0–31
			RR	— 0–15
			SL	— 0–15
			TL	— 0–127
			KS	— 0–3
			ML	— 0–15
			DT	— -3–3 or 0–7
			AMS	— 0–1
			Format 2	DT2	— 0–3
			Format 3	ALG	— 0–1
			FB	— 0–7
			AR	— 0–15
			DR	— 0–15
			RR	— 0–15
			SL	— 0–15
			TL	— 0–63
			KSL	— 0–3
			ML	— 0–15
			KSR	— 0–1
			EGT	— 0–1
			VIB	— 0–1
			AM	— 0–1*/
			
			//EXAMPLES:
			
			/*@  0  4  5  =falsyn?
				31  0  0  0  0  22  0  2  3   0
				18 10  0  6  0   0  0  8  3   0
				31  0  0  0  0  23  0  4 -3   0
				18 10  0  6  0   0  0  4 -3   0*/
			
			/*; NM AG FB  Falcom Synth
			@  0  4  5  =falsynOPM?
			;   AR DR SR RR SL  TL KS ML DT DT2 AMS
				31  0  0  0  0  22  0  2  3   0   0
				18 10  0  6  0   0  0  8  3   0   0
				31  0  0  0  0  23  0  4 -3   0   0
				18 10  0  6  0   0  0  4 -3   0   0*/
			
			/*; NM AG FB  E.Bass
			@  2  0  5  =E.Bass
			;   AR DR RR SL TL KSL ML KSR EGT VIB AM
				11  5  2  2 29   0  0   0   0   1  0
				12  8  6  1  0   0  1   1   1   1  0*/
				
			// ";" seems to define a comment string, and =[inst_name] seems to define instrument name
			
			char* current_line;
			char* lines[6] = { NULL }; //4 lines of main params + "header" with alg, fb and inst num
			//we are skipping lines that start with ";" since these are comments
			
			char* params[4][20] = { NULL }; //individual params
			
			Uint8 lines_counter = 0;
			Uint16 passes = 0;
			
			char* current_param; //parsed
			char* mml_string_copy = (char*)calloc(1, strlen(mml_string) + 1);
			strcpy(mml_string_copy, mml_string);
			
			debug("Selected PMD dialect");
			
			const char delimiters_lines[] = "\n\r";
			const char delimiters[] = "=\t @";
			
			bool instrument_started = false;
			
			//current_line = strtok(mml_string, delimiters_lines);
			
			while(current_line != NULL)
			{
				current_line = strtok(passes > 0 ? NULL : mml_string, delimiters_lines);
				passes++;
				
				if(current_line)
				{
					if(strchr(current_line, '@') == NULL && !(instrument_started)) //skipping all strings before start of instrument
					{
						debug("Skipping string");
						goto skip;
					}
					
					else
					{
						instrument_started = true;
					}
					
					if(strchr(current_line, ';') == NULL && lines_counter < 5)
					{
						lines[lines_counter] = current_line;
						lines_counter++;
						
						debug("Parsed line: \"%s\"", lines[lines_counter - 1]);
					}
					
					else if(lines_counter < 6 && strchr(current_line, ';') != NULL)
					{
						debug("Parsed line (comment): \"%s\"", current_line);
					}
					
					if(lines_counter == 5) goto parse_params;
				}
				
				skip:;
			}
			
			parse_params:;
			
			Uint8 param_counter = 0;
			Uint8 param_lines = 0; //if 4 then it's OPN/OPM, if 2 then OPL
			
			Uint8 alg = 0;
			Uint8 feedback = 0;
			
			current_param = strtok(lines[0], delimiters); //inst number, discarded here
			
			if(lines[0])
			{
				current_param = strtok(NULL, delimiters); //algorithm
				
				if(current_param)
				{
					alg = atoi(current_param);
					debug("Algorithm: \"%s\", %d", current_param, alg);
				}
				
				current_param = strtok(NULL, delimiters); //feedback
				
				if(current_param)
				{
					feedback = atoi(current_param);
					debug("Feedback: \"%s\", %d", current_param, feedback);
				}
				
				current_param = strtok(NULL, delimiters); //instrument name
				
				if(current_param && strlen(current_param) < MUS_INSTRUMENT_NAME_LEN - 1)
				{
					strcpy(inst->name, current_param);
					
					debug("instrument name: \"%s\"", inst->name);
				}
			}
			
			for(int i = 0; i < 4 && lines[i + 1]; ++i) //parsing lines
			{
				param_counter = 0;
				
				if(lines[i + 1])
				{
					debug("Parsing params from line: \"%s\"", lines[i + 1]);
					
					do
					{
						current_param = strtok(param_counter == 0 ? lines[i + 1] : NULL, delimiters);
						
						if(current_param)
						{
							params[i][param_counter] = current_param;
							
							debug("Parsed param: \"%s\"", current_param);
							
							param_counter++;
						}
						
					} while(current_param);
					
					param_lines++;
				}
			}
			
			debug("Parsed %d param lines", param_lines);
			
			if(param_lines < 2)
			{
				msgbox(domain, mused.slider_bevel, &mused.largefont, "No FM parameters lines found!", MB_OK);
				goto end;
			}
			
			Uint8 params_in_line = 0;
			
			do
			{
				current_param = params[1][params_in_line];
				
				if(current_param)
				{
					params_in_line++;
				}
				
			} while(current_param);
			
			debug("Params in one line: %d", params_in_line);
			
			Uint8 chip_type = param_lines == 2 ? YAMAHA_CHIP_OPL : (params_in_line == 10 ? YAMAHA_CHIP_OPN : YAMAHA_CHIP_OPM);
			
			debug("Chip type: %s", chip_types[chip_type]);
			
			if(chip_type == YAMAHA_CHIP_OPN || chip_type == YAMAHA_CHIP_OPM) //finally parsing the params!
			{
				inst->fm_freq_LUT = 1;
				
				inst->alg = reinterpret_yamaha_params(alg, YAMAHA_PARAM_CON, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
				inst->ops[convert_ops_indices[alg][0] - 1].feedback = reinterpret_yamaha_params(feedback, YAMAHA_PARAM_FL, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
				
				for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
				{
					inst->ops[convert_ops_indices[alg][i] - 1].adsr.a = reinterpret_yamaha_params(atoi(params[i][0]), YAMAHA_PARAM_AR, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
					inst->ops[convert_ops_indices[alg][i] - 1].adsr.d = reinterpret_yamaha_params(atoi(params[i][1]), YAMAHA_PARAM_DR, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
					inst->ops[convert_ops_indices[alg][i] - 1].adsr.sr = reinterpret_yamaha_params(atoi(params[i][2]), YAMAHA_PARAM_SR, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
					inst->ops[convert_ops_indices[alg][i] - 1].adsr.r = reinterpret_yamaha_params(atoi(params[i][3]), YAMAHA_PARAM_RR, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
					inst->ops[convert_ops_indices[alg][i] - 1].adsr.s = reinterpret_yamaha_params(atoi(params[i][4]), YAMAHA_PARAM_SL, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
					
					inst->ops[convert_ops_indices[alg][i] - 1].volume = reinterpret_yamaha_params(atoi(params[i][5]), YAMAHA_PARAM_TL, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
					
					//skip key scaling rn
					//inst->ops[convert_ops_indices[alg][i] - 1].env_ksl_level = reinterpret_yamaha_params(atoi(params[i][6]), YAMAHA_PARAM_KS, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
					
					/*if(inst->ops[convert_ops_indices[alg][i] - 1].env_ksl_level > 0)
					{
						inst->ops[convert_ops_indices[alg][i] - 1].cydflags |= CYD_FM_OP_ENABLE_ENVELOPE_KEY_SCALING;
					}*/
					
					inst->ops[convert_ops_indices[alg][i] - 1].harmonic = reinterpret_yamaha_params(atoi(params[i][7]), YAMAHA_PARAM_MUL, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
					inst->ops[convert_ops_indices[alg][i] - 1].detune = reinterpret_yamaha_params(atoi(params[i][8]), YAMAHA_PARAM_DT1, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
					
					if(chip_type == YAMAHA_CHIP_OPM)
					{
						inst->ops[convert_ops_indices[alg][i] - 1].coarse_detune = reinterpret_yamaha_params(atoi(params[i][9]), YAMAHA_PARAM_DT2, mused.song.song_rate, &mused.cyd, chip_type, BTN_PMD);
						//AMS
					}
					
					else
					{
						//AMS
					}
				}
				
				/*switch(inst->alg) //correcting TL of output ops so they are loud as they should be
				{
					case 1:
					case 2:
					case 3:
					{
						inst->ops[0].volume = 127 - atoi(params[3][5]);
						break;
					}
					
					case 6:
					{
						inst->ops[0].volume = 127 - atoi(params[3][5]);
						inst->ops[2].volume = 127 - atoi(params[1][5]);
						break;
					}
					
					case 9:
					case 11:
					{
						inst->ops[0].volume = 127 - atoi(params[3][5]);
						inst->ops[1].volume = 127 - atoi(params[2][5]);
						inst->ops[2].volume = 127 - atoi(params[1][5]);
						break;
					}
					
					case 13:
					{
						inst->ops[0].volume = 127 - atoi(params[3][5]);
						inst->ops[1].volume = 127 - atoi(params[2][5]);
						inst->ops[2].volume = 127 - atoi(params[1][5]);
						inst->ops[3].volume = 127 - atoi(params[0][5]);
						break;
					}
				}*/
			}
			
			end:;
			
			free(mml_string_copy);
			
			break;
		}
		
		case BTN_FMP:
		{
			debug("Selected FMP dialect");
			
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Only PMD dialect supported in this version!", MB_OK);
			
			break;
		}
		
		case BTN_FMP7:
		{
			debug("Selected FMP7 dialect");
			
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Only PMD dialect supported in this version!", MB_OK);
			
			break;
		}
		
		case BTN_VOPM:
		{
			debug("Selected VOPM dialect");
			
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Only PMD dialect supported in this version!", MB_OK);
			
			break;
		}
		
		case BTN_NRTDRV:
		{
			debug("Selected NRTDRV dialect");
			
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Only PMD dialect supported in this version!", MB_OK);
			
			break;
		}
		
		case BTN_MXDRV:
		{
			debug("Selected MXDRV dialect");
			
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Only PMD dialect supported in this version!", MB_OK);
			
			break;
		}
		
		case BTN_MMLDRV:
		{
			debug("Selected MMLDRV dialect");
			
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Only PMD dialect supported in this version!", MB_OK);
			
			break;
		}
		
		case BTN_MUCOM88:
		{
			debug("Selected MUCOM88 dialect");
			
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Only PMD dialect supported in this version!", MB_OK);
			
			break;
		}
		
		default: break;
	}
}

void import_mml_fm_instrument_string(MusInstrument* inst)
{
	debug("Import MML string:");
	
	if (confirm(domain, mused.slider_bevel, &mused.largefont, "Import MML string to current instrument?\n  It will overwrite current instrument!"))
	{
		char* mml_string = SDL_GetClipboardText();
		debug("got MML string: \"%s\"", mml_string);
		
		if(strcmp(mml_string, "") == 0)
		{
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Clipboard is empty!", MB_OK);
			SDL_free(mml_string);
			
			goto end;
		}
		
		int dialect = msgbox_mml(domain, mused.slider_bevel, &mused.largefont, "Select MML dialect:", 0xFFFF);
		
		if(dialect)
		{
			snapshot(S_T_INSTRUMENT);
			
			mus_get_empty_instrument(inst);
			
			inst->cydflags = CYD_CHN_ENABLE_FM | CYD_CHN_ENABLE_KEY_SYNC;
			inst->fm_flags = CYD_FM_ENABLE_4OP | CYD_FM_FOUROP_USE_MAIN_INST_PROG;
			
			for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
			{
				inst->ops[i].flags = MUS_FM_OP_SAVE_LFO_SETTINGS | MUS_FM_OP_RELATIVE_VOLUME;
				inst->ops[i].cydflags = CYD_FM_OP_ENABLE_EXPONENTIAL_VOLUME | CYD_FM_OP_ENABLE_KEY_SYNC | CYD_FM_OP_ENABLE_SINE;
			}
			
			process_mml_string(inst, dialect, mml_string);
		}
		
		//do stuff
		
		SDL_free(mml_string);
	}
	
	end:;
	
	mused.import_mml_string = false;
}