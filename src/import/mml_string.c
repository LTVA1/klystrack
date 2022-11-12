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

#include "mml_string.h"

extern Mused mused;
extern GfxDomain *domain;

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
			@  0  4  5  =falsyn?
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
			
			debug("Selected PMD dialect");
			break;
		}
		
		case BTN_FMP:
		{
			debug("Selected FMP dialect");
			break;
		}
		
		case BTN_FMP7:
		{
			debug("Selected FMP7 dialect");
			break;
		}
		
		case BTN_VOPM:
		{
			debug("Selected VOPM dialect");
			break;
		}
		
		case BTN_NRTDRV:
		{
			debug("Selected NRTDRV dialect");
			break;
		}
		
		case BTN_MXDRV:
		{
			debug("Selected MXDRV dialect");
			break;
		}
		
		case BTN_MMLDRV:
		{
			debug("Selected MMLDRV dialect");
			break;
		}
		
		case BTN_MUCOM88:
		{
			debug("Selected MUCOM88 dialect");
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