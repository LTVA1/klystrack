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

#include "credits.h"

#include "../../klystron/src/gui/filebox.h"
#include "../../klystron/src/gui/msgbox.h"
#include "../../klystron/src/gui/bevel.h"
#include "../../klystron/src/gui/bevdefs.h"
#include "../../klystron/src/gui/dialog.h"
#include "../../klystron/src/gfx/gfx.h"
#include "../../klystron/src/gui/view.h"
#include "../../klystron/src/gui/mouse.h"
#include "../../klystron/src/gui/toolutil.h"
#include "../../klystron/src/gui/slider.h"
#include "../mused.h"
#include "../action.h"
#include "../theme.h"
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

extern Mused mused;
extern GfxDomain* domain;

#define G 0.5 /* sort of gravitational constant lol */

void show_credits(void *unused0, void *unused1, void *unused2)
{
	bool modified = mused.modified;
	
	char filename[5000] = {0};
	//char* song_name = (char*)&mused.song.title;
	char* song_name = malloc(strlen((char*)&mused.song.title) + 2);
	
	strcpy(song_name, (char*)&mused.song.title);
	
	for(int i = 0; i <= strlen((char*)&mused.song.title); i++)
	{
		if(song_name[i] == '\\' || song_name[i] == '/' || song_name[i] == ':' || song_name[i] == '*' || song_name[i] == '?' || song_name[i] == '\"' || song_name[i] == '<' || song_name[i] == '>' || song_name[i] == '|')
		{
			song_name[i] = '_';
		}
	}

	snprintf(filename, sizeof(filename) - 1, "%s.credits_backup.kt.autosave", strcmp(song_name, "") == 0 ? "[untitled_song]" : song_name);
	
	free(song_name);
	
	SDL_RWops* rw = SDL_RWFromFile(filename, "wb");
	
	if(rw)
	{
		save_song(rw, false);
		SDL_RWclose(rw);
	}
	
	else
	{
		goto end;
	}
	
	FILE* f_bgm = fopen("examples/songs/n00bstar-examples/highscore.kt", "rb");
	
	if(f_bgm)
	{
		open_song(f_bgm);
		fclose(f_bgm);
		
		play(0, 0, 0);
		
		mused.mus.volume = 0x30;
		mused.cyd.mus_volume = 0x30;
	}
	
	//debug("sizeof dot %d", sizeof(Dot));
	
	Sint32 text_scroll_position = domain->screen_h + 10;
	
	Uint16 total_dots = NUM_DOTS - 1;
	Sint16 selected_dot = -1;
	Sint16 deleted_dot = -1;
	
	Uint8 frames_deletion = 0;
	
	SDL_Event e = { 0 };
	int got_event = 0;
	
	SDL_Rect textrect = { 0, 0, 1000, 500 };
	SDL_Rect area = { 0, 0, 0, 0 };
	
	SDL_Rect plasma_pixel = { 0, 0, 15, 15 };
	
	SDL_Rect dot_rect = { 0, 0, 3, 3 };
	
	bool quit = false;
	
	Uint32 timeout = SDL_GetTicks() + TIMEOUT; //20 fps
	
	Uint32 frames = 0;
	
	Uint32* grey = (Uint32*)malloc(256 * sizeof(Uint32));
	
	Dot* dots = (Dot*)malloc(NUM_DOTS * 8 * sizeof(Dot));
	
	for(int i = 0; i < NUM_DOTS; ++i)
	{
		dots[i].x = 10 + (rand() % (domain->screen_w - 10));
		dots[i].y = 10 + (rand() % (domain->screen_h - 10));
		dots[i].vx = ((rand() & 1) ? -1 : 1) * (10 + (double)(rand() % 10)) / 120.0;
		dots[i].vy = ((rand() & 1) ? -1 : 1) * (10 + (double)(rand() % 10)) / 120.0;
	}
	
	for(int step = 0; step < NUM_PREV_COORDS; ++step)
	{
		for(int i = 0; i < NUM_DOTS; ++i)
		{
			dots[i].x = my_min(my_max(dots[i].x + dots[i].vx, 0), domain->screen_w);
			dots[i].y = my_min(my_max(dots[i].y + dots[i].vy, 0), domain->screen_h);
			
			if(dots[i].x == domain->screen_w || dots[i].x == 0)
			{
				dots[i].vx *= -1;
			}
			
			if(dots[i].y == domain->screen_h || dots[i].y == 0)
			{
				dots[i].vy *= -1;
			}
			
			dots[i].prev_coords[NUM_PREV_COORDS - 1 - step].x = dots[i].x;
			dots[i].prev_coords[NUM_PREV_COORDS - 1 - step].y = dots[i].y;
		}
	}
	
	double f = 0;

	for(int x = 0; x < 256; x++)
	{
		grey[x] = 47 - ceil((sin(3.14 * 2 * x / 127) + 1) * 23) + 10;
		//int r = 255 - ceil((sin(3.14 * 2 * x / 255) + 1) * 127);
		//grey[x] = (r + (ceil((sin(3.14 * 2 * x / 127.0) + 1) * 64)) + (255 - r)) / 12;
		grey[x] = grey[x] | (grey[x] << 8) | (grey[x] << 16);
		//g[x] = ceil((sin(3.14 * 2 * x / 127.0) + 1) * 64);
		//b[x] = 255 - r[x];
	}
	
	Uint32 x_scale = domain->screen_w / 32;
	Uint32 y_scale = domain->screen_h / 32;
	
	bool shift_pressed = false;
	
	while(!quit)
	{
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				case SDL_QUIT:
				{
					set_repeat_timer(NULL);
					SDL_PushEvent(&e);
					goto end;
					
					return;
				}
				break;

				case SDL_KEYDOWN:
				{
					if(e.key.keysym.sym == SDLK_LSHIFT || e.key.keysym.sym == SDLK_RSHIFT)
					{
						shift_pressed = true;
					}
					
					switch (e.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_KP_ENTER:
						case SDLK_SPACE:
						case SDLK_BACKSPACE:
						{
							set_repeat_timer(NULL);
							goto end;
							
							return;
						}
						break;
						
						default: break;
					}
					
					break;
				}
				
				case SDL_KEYUP:
				{
					if(e.key.keysym.sym == SDLK_LSHIFT || e.key.keysym.sym == SDLK_RSHIFT)
					{
						shift_pressed = false;
					}
					
					break;
				}

				case SDL_USEREVENT:
					e.type = SDL_MOUSEBUTTONDOWN;
				break;
				
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

				case SDL_MOUSEMOTION:
					if (domain)
					{
						gfx_convert_mouse_coordinates(domain, &e.motion.x, &e.motion.y);
						gfx_convert_mouse_coordinates(domain, &e.motion.xrel, &e.motion.yrel);
					}
				break;
				
				#define SELECT_AREA_SIZE 6
				
				case SDL_MOUSEBUTTONDOWN:
					if (domain)
					{
						gfx_convert_mouse_coordinates(domain, &e.button.x, &e.button.y);
						
						if(e.button.button == SDL_BUTTON_LEFT)
						{
							for(int i = 0; i < total_dots; ++i)
							{
								if(abs(e.button.x - dots[i].x) < SELECT_AREA_SIZE && abs(e.button.y - dots[i].y) < SELECT_AREA_SIZE)
								{
									selected_dot = i;
									goto finish;
								}
							}
							
							if(total_dots < NUM_DOTS * 8)
							{
								dots[total_dots].x = e.button.x;
								dots[total_dots].y = e.button.y;
								
								if(shift_pressed == false)
								{
									dots[total_dots].vx = ((rand() & 1) ? -1 : 1) * (10 + (double)(rand() % 10)) / 40.0;
									dots[total_dots].vy = ((rand() & 1) ? -1 : 1) * (10 + (double)(rand() % 10)) / 40.0;
								}
								
								else
								{
									dots[total_dots].vx = 0;
									dots[total_dots].vy = 0;
								}
								
								for(int i = 0; i < NUM_PREV_COORDS; ++i)
								{
									dots[total_dots].prev_coords[i].x = -100;
									dots[total_dots].prev_coords[i].y = -100;
								}
								
								total_dots++;
							}
						}
						
						if(e.button.button == SDL_BUTTON_RIGHT)
						{
							for(int i = 0; i < total_dots; ++i)
							{
								if(abs(e.button.x - dots[i].x) < SELECT_AREA_SIZE && abs(e.button.y - dots[i].y) < SELECT_AREA_SIZE)
								{
									deleted_dot = i;
									frames_deletion = 0;
									goto finish;
								}
							}
							
							selected_dot = -1;
							goto finish;
						}
						
						finish:;
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
			if(SDL_GetTicks() >= TIMEOUT)
			{
				timeout = SDL_GetTicks() + TIMEOUT;
				
				area.h = domain->screen_h * domain->scale;
				area.w = domain->screen_w * domain->scale;
				
				gfx_domain_set_clip(domain, &area);
				gfx_rect(domain, &area, colors[COLOR_WAVETABLE_BACKGROUND]); //fill screen with black
				
				for(int y = 0; y < domain->screen_h / 16 + 1; y++) //plasma effect stolen from https://gist.github.com/stevenlr/824019
				{
					for(int x = 0; x < domain->screen_w / 16 + 1; x++)
					{
						double c1 = sin(x / 50.0 + f + y / 20.0);
						double c2 = sqrt((sin(0.8 * f) * x_scale - x + x_scale) * (sin(0.8 * f) * x_scale - x + x_scale) + (cos(1.2 * f) * y_scale - y + y_scale) * (cos(1.2 * f) * y_scale - y + y_scale));
						c2 = sin(c2 / 50.0);
						double c3 = (c1 + c2) / 2;

						int res = ceil((c3 + 1) * 127);
						
						plasma_pixel.x = x << 4;
						plasma_pixel.y = y << 4;
						gfx_rect(domain, &plasma_pixel, grey[res]);
					}
				}
				
				f += 0.005;
				
				for(int i = 0; i < total_dots; ++i)
				{
					for(int j = 0; j < total_dots; ++j) //attraction forces
					{
						if(i < j && i != deleted_dot)
						{
							double dx2 = (dots[i].x - dots[j].x) * (dots[i].x - dots[j].x);
							double dy2 = (dots[i].y - dots[j].y) * (dots[i].y - dots[j].y);
							
							//double distance = sqrt(dx2 + dy2);
							
							if(dx2 > 2.0 && dy2 > 2.0) //LOOKS LIKE I FINALLY MADE A BELIEVABLE FUCKING GRAVITY!!!
							{
								//dots[i].vx -= ((dots[i].x - dots[j].x) > 0 ? 1 : -1) * G / ((dots[i].x - dots[j].x) * (dots[i].x - dots[j].x));
								//dots[i].vx = my_min(my_max(dots[i].vx - ((dots[i].x - dots[j].x) > 0 ? 1 : -1) * G / ((dots[i].x - dots[j].x) * (dots[i].x - dots[j].x)), -10.5), 10.5);
								
								double dvx = G / ((dots[i].x - dots[j].x) * fabs(dots[i].x - dots[j].x));
								
								dots[i].vx = my_min(my_max(dots[i].vx - dvx, -10.5), 10.5);
								
								dots[j].vx = my_min(my_max(dots[j].vx + dvx, -10.5), 10.5);
								
								//dots[i].vy -= ((dots[i].y - dots[j].y) > 0 ? 1 : -1) * G / ((dots[i].y - dots[j].y) * (dots[i].y - dots[j].y));
								//dots[i].vy = my_min(my_max(dots[i].vy - ((dots[i].y - dots[j].y) > 0 ? 1 : -1) * G / ((dots[i].y - dots[j].y) * (dots[i].y - dots[j].y)), -10.5), 10.5);
								
								double dvy = G / ((dots[i].y - dots[j].y) * fabs(dots[i].y - dots[j].y));
								
								dots[i].vy = my_min(my_max(dots[i].vy - dvy, -10.5), 10.5);
								
								dots[j].vy = my_min(my_max(dots[j].vy + dvy, -10.5), 10.5);
							}
						}
					}
				}
				
				for(int i = 0; i < total_dots; ++i)
				{
					if(i != deleted_dot)
					{
						dots[i].x = my_min(my_max(dots[i].x + dots[i].vx, 0), domain->screen_w);
						dots[i].y = my_min(my_max(dots[i].y + dots[i].vy, 0), domain->screen_h);
					}
					
					if(dots[i].x == domain->screen_w || dots[i].x == 0)
					{
						dots[i].vx *= -0.5;
						dots[i].vy *= 0.5;
					}
					
					if(dots[i].y == domain->screen_h || dots[i].y == 0)
					{
						dots[i].vy *= -0.5;
						dots[i].vx *= 0.5;
					}
					
					for(int j = NUM_PREV_COORDS - 1 - 1; j >= 0; --j)
					{
						dots[i].prev_coords[j + 1].x = dots[i].prev_coords[j].x; //shifting buffer
						dots[i].prev_coords[j + 1].y = dots[i].prev_coords[j].y;
					}
					
					dots[i].prev_coords[0].x = (Sint16)dots[i].x;
					dots[i].prev_coords[0].y = (Sint16)dots[i].y;
					
					//======================
					
					for(int j = 0; j < NUM_PREV_COORDS - 1; ++j)
					{
						if(dots[i].prev_coords[j + 1].x >= 0 && dots[i].prev_coords[j + 1].y >= 0)
						{
							gfx_translucent_line(domain, dots[i].prev_coords[j].x + 1 /* so we draw to the center of the dot */, dots[i].prev_coords[j].y + 1, dots[i].prev_coords[j + 1].x + 1, dots[i].prev_coords[j + 1].y + 1, (Uint32)RGB(0, 0x7F, 0x7F) | (Uint32)((((NUM_PREV_COORDS - 1 - j) * 3) >> 1) << 24));
						}
					}
					
					if(i != deleted_dot)
					{
						dot_rect.x = dots[i].x;
						dot_rect.y = dots[i].y;
						
						
						gfx_rect(domain, &dot_rect, 0x808080);
					}
				}
				
				if(selected_dot >= 0)
				{
					int i = selected_dot;
					
					gfx_translucent_line(domain, dots[i].x - 4 + 1, dots[i].y - 4 + 1, dots[i].x - 4 + 1, dots[i].y - 2 + 1, 0xFFFFFF | (100 << 24));
					gfx_translucent_line(domain, dots[i].x - 4 + 1, dots[i].y - 4 + 1, dots[i].x - 2 + 1, dots[i].y - 4 + 1, 0xFFFFFF | (100 << 24));
					
					gfx_translucent_line(domain, dots[i].x + 4 + 1, dots[i].y + 4 + 1, dots[i].x + 4 + 1, dots[i].y + 2 + 1, 0xFFFFFF | (100 << 24));
					gfx_translucent_line(domain, dots[i].x + 4 + 1, dots[i].y + 4 + 1, dots[i].x + 2 + 1, dots[i].y + 4 + 1, 0xFFFFFF | (100 << 24));
					
					gfx_translucent_line(domain, dots[i].x + 4 + 1, dots[i].y - 4 + 1, dots[i].x + 4 + 1, dots[i].y - 2 + 1, 0xFFFFFF | (100 << 24));
					gfx_translucent_line(domain, dots[i].x + 4 + 1, dots[i].y - 4 + 1, dots[i].x + 2 + 1, dots[i].y - 4 + 1, 0xFFFFFF | (100 << 24));
					
					gfx_translucent_line(domain, dots[i].x - 4 + 1, dots[i].y + 4 + 1, dots[i].x - 4 + 1, dots[i].y + 2 + 1, 0xFFFFFF | (100 << 24));
					gfx_translucent_line(domain, dots[i].x - 4 + 1, dots[i].y + 4 + 1, dots[i].x - 2 + 1, dots[i].y + 4 + 1, 0xFFFFFF | (100 << 24));
					
					textrect.y = dots[i].y - 10 - 6;
					textrect.x = dots[i].x - 4;
					
					font_set_color(&mused.tinyfont, 0xAAAAAA);
					
					font_write_args(&mused.tinyfont, domain, &textrect, "X: %u Y: %u", (int)dots[i].x, (int)dots[i].y);
					
					textrect.y += 6;
					
					font_write_args(&mused.tinyfont, domain, &textrect, "VX: %0.2f VY: %0.2f", dots[i].vx, dots[i].vy);
				}
				
				if(deleted_dot >= 0 && total_dots > 0)
				{
					dots[deleted_dot].vx = dots[deleted_dot].vy = 0.0;
					
					dot_rect.x = dots[deleted_dot].x;
					dot_rect.y = dots[deleted_dot].y;
					
					for(int q = 0; q < 8; ++q)
					{
						for(int j = NUM_PREV_COORDS - 1 - 1; j >= 0; --j)
						{
							dots[deleted_dot].prev_coords[j + 1].x = dots[deleted_dot].prev_coords[j].x; //shifting buffer
							dots[deleted_dot].prev_coords[j + 1].y = dots[deleted_dot].prev_coords[j].y;
						}
					}
					
					gfx_translucent_rect(domain, &dot_rect, (Uint32)RGB(0x90, 0, 0) | (Uint32)((255 - frames_deletion * 255 / NUM_PREV_COORDS * 8) << 24));
					
					frames_deletion++;
					
					if(frames_deletion > NUM_PREV_COORDS / 8)
					{
						frames_deletion = 0;
						
						for(int i = deleted_dot + 1; i < total_dots; ++i)
						{
							memcpy(dots + (i - 1), dots + i, sizeof(Dot));
						}
						
						if(deleted_dot == selected_dot)
						{
							selected_dot = -1;
						}
						
						else
						{
							if(selected_dot > deleted_dot)
							{
								selected_dot--;
							}
						}
						
						total_dots--;
						
						deleted_dot = -1;
					}
				}
				
				//gfx_domain_set_clip(domain, &textrect); //draw text
				//console_clear(mused.console);
				
				#define SHIFT (50 + 8 + 8)
				
				if(mused.font16.surface && mused.font24.surface)
				{
					textrect.y = text_scroll_position;
					textrect.x = domain->screen_w / 2 - 240;
					
					font_set_color(&mused.font16, 0xFFFF00);
					font_set_color(&mused.font24, 0x00FFFF);
					
					font_write_args(&mused.font24, domain, &textrect, "     kometbomb");
					
					textrect.y += 26 + 26;
					textrect.x -= SHIFT;
					
					font_write_args(&mused.font16, domain, &textrect, "  The author of original klystrack.");
					
					textrect.y += 18;
					
					font_write_args(&mused.font16, domain, &textrect, "   Tracker code and all the ideas.");
					
					textrect.y += 80;
					
					font_set_color(&mused.font24, 0x90A7A0);
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "       ilkke");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "  GUI design, graphics. Demo songs:");
					
					textrect.y += 18;
					
					font_write_args(&mused.font16, domain, &textrect, "   phonkeh.kt, Sprock'n'Sprawl.kt,");
					
					textrect.y += 18;
					
					font_write_args(&mused.font16, domain, &textrect, "        and demo instruments.");
					
					textrect.y += 80;
					
					font_set_color(&mused.font24, 0x308CCD);
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "        Spot");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "           Amiga port, ideas.");
					
					textrect.y += 80;
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "     Deltafire");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "               OSX port.");
					
					textrect.y += 80;
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "     Dr. Mabuse");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "Example song: Paranoimia_(Suntronic).kt");
					
					textrect.y += 80;
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "      null1024");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "       Example song: obspatial.kt");
					
					textrect.y += 80;
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "  Jeremy D. Landry");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, " Example song: hskv03-rygar_trance.kt");
					
					textrect.y += 80;
					
					font_set_color(&mused.font24, 0xF3D115);
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "      major Den");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "         Theme (Golden_Brown).");
					
					textrect.y += 80;
					
					font_set_color(&mused.font24, 0x308CCD);
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "   n00bstar/random");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "Example songs: dr.happy.kt, smp_dpintro.kt,");
					textrect.y += 18;
					
					font_write_args(&mused.font16, domain, &textrect, "              castlevania.kt.");
					
					textrect.y += 18;
					
					font_write_args(&mused.font16, domain, &textrect, "     Example instruments and tutorials.");
					
					textrect.y += 80;
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "       llyode");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "     Example song: Ocean Loader III.kt");
					
					textrect.y += 80;
					
					font_set_color(&mused.font24, 0x90506C);
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "      System64");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "      Example songs: Magica.kt,");
					
					textrect.y += 18;
					
					font_write_args(&mused.font16, domain, &textrect, " 4-Dimensional-Goddess-Of-Existence.kt,");
					
					textrect.y += 18;
					
					font_write_args(&mused.font16, domain, &textrect, "        ideas and some code.");
					
					textrect.y += 80;
					
					font_set_color(&mused.font24, 0x943074);
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "     Ravancloak");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "          Theme (Ravancore).");
					
					textrect.y += 80;
					
					font_set_color(&mused.font24, 0x308CCD);
					
					textrect.x += SHIFT;
					
					font_write_args(&mused.font24, domain, &textrect, "        LTVA");
					
					textrect.x -= SHIFT;
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "       Creating klystrack-plus");
					
					textrect.y += 18;
					
					font_write_args(&mused.font16, domain, &textrect, "in a humble attempt to make this tracker");
					
					textrect.y += 18;
					
					font_write_args(&mused.font16, domain, &textrect, "   better by adding a lot of terrible");
					
					textrect.y += 18;
					
					font_write_args(&mused.font16, domain, &textrect, "           spaghetti code.");
					
					textrect.y += 26 + 26;
					
					font_write_args(&mused.font16, domain, &textrect, "If you hear music, it is \"Gravitus Minus\"");
					
					textrect.y += 18;
					
					font_write_args(&mused.font16, domain, &textrect, "     highscore theme by n00bstar.");
					
					if(frames & 3)
					{
						text_scroll_position--;
					}
					
					if(text_scroll_position < -2050)
					{
						text_scroll_position = domain->screen_h + 10;
					}
				}
				
				else
				{
					textrect.x = domain->screen_w / 2 - 150;
					textrect.y = domain->screen_h / 2 - 8;
					
					font_write_args(&mused.largefont, domain, &textrect, "        No special fonts found.");
					
					textrect.y += 10;
					
					font_write_args(&mused.largefont, domain, &textrect, "       Update your \"res\" folder.");
				}
				
				frames++;
				
				gfx_domain_flip(domain);
			}
		}
		
		else
		{
			SDL_Delay(5);
		}
	}
	
	end:;
	
	font_set_color(&mused.tinyfont, colors[COLOR_MAIN_TEXT]);
	
	SDL_StopTextInput();
	
	if(filename[0])
	{
		cyd_lock(&mused.cyd, 1);
		
		FILE* f = fopen(filename, "rb");
		
		if (f)
		{
			open_song(f);
			fclose(f);
		}
		
		cyd_lock(&mused.cyd, 0);
		
		remove(filename);
		
		mused.modified = modified;
		
		play(0, 0, 0);
		stop(0, 0, 0);
	}
	
	if(grey)
	{
		free(grey);
	}
	
	if(dots)
	{
		free(dots);
	}
}