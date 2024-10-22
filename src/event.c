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


#include "event.h"
#include "mused.h"
#include "action.h"
#include "edit.h"
#include "util/rnd.h"
#include <string.h>
#include <sys/stat.h>
#include "snd/cydwave.h"
#include "snd/freqs.h"
#include "mymsg.h"
#include "command.h"

#include "theme.h"
#include "import/import.h"

#include "gui/view.h"

extern Mused mused;

#define flipbit(val, bit) { val ^= bit; };

static void play_the_jams(int sym, int chn, int state);

void editparambox(int v)
{
	MusInstrument *inst = &mused.song.instrument[mused.current_instrument];
	Uint16 *param;
	
	if(mused.show_four_op_menu)
	{
		if(!(inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]])) return;
		
		param = &inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step];
	}
	
	else
	{
		if(!(inst->program[mused.current_instrument_program])) return;
		
		param = &inst->program[mused.current_instrument_program][mused.current_program_step];
	}
	
	Uint32 mask = 0xffff0fff >> (mused.editpos * 4);

	if (*param == MUS_FX_NOP)
		*param = 0;

	if (mused.editpos != 0 || v < 0xf)
	{
		// Keeps the exec next command bit intact
		*param = (((*param & mask) | ((v&0xf) << ((3-mused.editpos)*4))) & 0xffff); //old command *param = (*param & 0x8000) | (((*param & mask) | ((v&0xf) <<((3-mused.editpos)*4))) & 0x7fff);
		//*param = (*param & 0x8000) | (((*param & mask) | ((v&0xf) << ((3-mused.editpos)*4))) & 0xffff);
	}
	else
	{
		*param = ((*param & mask) | ((v&0xf) << ((3-mused.editpos)*4)));
	}

	if (++mused.editpos > 3)
	{
		*param = validate_command(*param);
		mused.editpos = 3;
	}
}


int find_note(int sym, int oct)
{
	int n = 0;
	
	static const int alias_keys[] =
	{
	SDLK_z, SDLK_s, SDLK_x, SDLK_d, SDLK_c, SDLK_v, SDLK_g, SDLK_b, SDLK_h, SDLK_n, SDLK_j, SDLK_m,
	SDLK_COMMA, SDLK_l, SDLK_w, SDLK_3, SDLK_e, SDLK_r, SDLK_5, SDLK_t, SDLK_6, SDLK_y, SDLK_7, SDLK_u,
	SDLK_i, SDLK_9, SDLK_o, SDLK_0, SDLK_p, SDLK_LEFTBRACKET, 0, SDLK_RIGHTBRACKET, -1};
	
	for (const int *i = alias_keys; *i != -1; ++i, ++n)
	{
		if (*i == sym)
			return n + (oct + 5) * 12;
	}
	
	static const int keys[] =
	{
	SDLK_z, SDLK_s, SDLK_x, SDLK_d, SDLK_c, SDLK_v, SDLK_g, SDLK_b, SDLK_h, SDLK_n, SDLK_j, SDLK_m,
	SDLK_q, SDLK_2, SDLK_w, SDLK_3, SDLK_e, SDLK_r, SDLK_5, SDLK_t, SDLK_6, SDLK_y, SDLK_7, SDLK_u,
	SDLK_i, SDLK_9, SDLK_o, SDLK_0, SDLK_p, SDLK_LEFTBRACKET, 0, SDLK_RIGHTBRACKET, -1};

	n = 0;
	
	for (const int *i = keys; *i != -1; ++i, ++n)
	{
		if (*i == sym)
			return n + (oct + 5) * 12;
	}

	return -1;
}

static int autosave_thread_func(void* payload)
{
	SDL_RWops* rw = (SDL_RWops*)payload;
	save_song(rw, false);
	SDL_RWclose(rw);
	
	return 0;
}

void do_autosave(Uint32* timeout)
{
	if((mused.flags2 & ENABLE_AUTOSAVE) && mused.time_between_autosaves > 0 && !(mused.flags & SONG_PLAYING) && mused.modified)
	{
		//debug("motherfucker outside");
		
		if(SDL_GetTicks() >= *timeout)
		{
			*timeout = SDL_GetTicks() + mused.time_between_autosaves;
			
			//debug("motherfucker");
			
			char filename[10000] = {0};
			//char* song_name = (char*)&mused.song.title;
			char* song_name = calloc(1, strlen((char*)&mused.song.title) + 2);
			strcpy(song_name, (char*)&mused.song.title);
			
			for(int i = 0; i <= strlen((char*)&mused.song.title); i++)
			{
				if(song_name[i] == '\\' || song_name[i] == '/' || song_name[i] == ':' || song_name[i] == '*' || song_name[i] == '?' || song_name[i] == '\"' || song_name[i] == '<' || song_name[i] == '>' || song_name[i] == '|')
				{
					song_name[i] = '_';
				}
			}
			
			time_t now_time;
			time(&now_time);
			struct tm *now_tm = localtime(&now_time);

			if (!now_tm)
			{
				debug("Failed to retrieve current time!");
				goto error;
			}

			snprintf(filename, sizeof(filename) - 1, "%s/autosaves/%s.%04d%02d%02d-%02d%02d%02d.kt.autosave", mused.app_dir, strcmp(song_name, "") == 0 ? "[untitled_song]" : song_name,
				now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);
				
			free(song_name);
			
			char dir_name[10000] = {0};
			snprintf(dir_name, sizeof(dir_name) - 1, "%s/autosaves", mused.app_dir);

			DIR* dir = opendir(dir_name);
			
			if(ENOENT == errno)
			{
				debug("No autosaves directory found, attempt to create it...");
				
				int check = mkdir(dir_name, S_IRWXU);
				
				if(!check)
				{
					debug("Directory created");
					closedir(dir);
				}
				
				else
				{
					debug("Failed to create directory!");
					closedir(dir);
					goto error;
				}
			}
			
			//FILE* f = fopen(filename, "wb");
			SDL_RWops* rw = SDL_RWFromFile(filename, "wb");
			
			if(rw)
			{
				if(mused.flags2 & SHOW_AUTOSAVE_MESSAGE)
				{
					SDL_Rect area = {domain->screen_w / 2 - 65, domain->screen_h / 2 - 12, 130, 24};
					bevel(domain, &area, mused.slider_bevel, BEV_MENU);
					
					adjust_rect(&area, 8);
					area.h = 16;
					
					font_write_args(&mused.largefont, domain, &area, "Autosaving...");
					
					gfx_domain_flip(domain);
				}
				
				//save_song(rw, false);
				//SDL_RWclose(rw);
				
				SDL_Thread* autosave_thread = SDL_CreateThread(autosave_thread_func, "Autosave", (void*)rw);
				SDL_DetachThread(autosave_thread);
			}
			
			else
			{
				//fclose(f);
				//SDL_RWclose(rw);
				debug("Failed to create rw struct from file \"%s\"!", filename);
				goto error;
			}
			
			if(mused.flags2 & SHOW_AUTOSAVE_MESSAGE)
			{
				SDL_Delay(900);
			}
			
			goto end;
			
			error:;
			
			SDL_Rect area = {domain->screen_w / 2 - 65, domain->screen_h / 2 - 12, 130, 24};
			bevel(domain, &area, mused.slider_bevel, BEV_MENU);
			
			adjust_rect(&area, 8);
			area.h = 16;
			
			font_write_args(&mused.largefont, domain, &area, "Error!");
			
			gfx_domain_flip(domain);
			
			SDL_Delay(1000);
		}
		
		end:;
	}
}

void dropfile_event(SDL_Event *e)
{
	debug("Dropped file: \"%s\"", e->drop.file);
	
	char* dropped_filedir = e->drop.file;
	FILE *f = fopen(dropped_filedir, "rb");
	
	const char* extension = &dropped_filedir[strlen(dropped_filedir) - 3];
	debug("Extension %s", extension);
	
	switch(mused.focus)
	{
		case EDITBUFFER:
		{
			switch(mused.prev_mode)
			{
				case EDITPATTERN:
				case EDITSEQUENCE:
				case EDITSONGINFO:
				case EDITCLASSIC:
				{
					goto load_song;
				}
				
				case EDITPROG:
				case EDITPROG4OP:
				{
					goto load_inst;
				}
				
				case EDITWAVETABLE:
				case EDITLOCALSAMPLE:
				{
					goto load_wave;
				}
				
				default: break;
			}
			
			break;
		}
		
		default: break;
	}
	
	switch(mused.mode)
	{
		case EDITPATTERN:
		case EDITSEQUENCE:
		case EDITSONGINFO:
		case EDITCLASSIC:
		{
			load_song:;
			
			if(f)
			{
				if(strcmp(".kt", extension) == 0)
				{
					debug("Dropped a song");
					
					if(mused.modified)
					{
						if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current song?"))
						{
							stop(0, 0, 0);
							
							open_song(f);
							fclose(f);
							
							set_channels(mused.song.num_channels);
							
							mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
							play(0, 0, 0);
							stop(0, 0, 0);
						}
					}
					
					else
					{
						stop(0, 0, 0);
						
						open_song(f);
						fclose(f);
						
						set_channels(mused.song.num_channels);
						
						mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
						play(0, 0, 0);
						stop(0, 0, 0);
					}
				}
				
				else
				{
					const char* mod_extension = &dropped_filedir[strlen(dropped_filedir) - 4];
					
					if((strcmp(".mod", mod_extension)) == 0 || (strcmp(".MOD", mod_extension)) == 0)
					{
						debug("Dropped a .MOD file");
					
						if(mused.modified)
						{
							if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current song?"))
							{
								stop(0, 0, 0);
								
								new_song();
								import_mod(f);
								fclose(f);
								
								kill_empty_patterns(&mused.song, NULL); //wasn't there
								optimize_duplicate_patterns(&mused.song, true);
								set_channels(mused.song.num_channels);
								
								mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
								play(0, 0, 0);
								stop(0, 0, 0);
							}
						}
						
						else
						{
							stop(0, 0, 0);
							
							new_song();
							import_mod(f);
							fclose(f);
							
							kill_empty_patterns(&mused.song, NULL); //wasn't there
							optimize_duplicate_patterns(&mused.song, true);
							set_channels(mused.song.num_channels);
							
							mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
							play(0, 0, 0);
							stop(0, 0, 0);
						}
						
						goto end;
					}
					
					if((strcmp(".ahx", mod_extension)) == 0 || (strcmp(".AHX", mod_extension)) == 0)
					{
						debug("Dropped an .AHX file");
					
						if(mused.modified)
						{
							if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current song?"))
							{
								stop(0, 0, 0);
								
								new_song();
								import_ahx(f);
								fclose(f);
								
								kill_empty_patterns(&mused.song, NULL); //wasn't there
								optimize_duplicate_patterns(&mused.song, true);
								set_channels(mused.song.num_channels);
								
								mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
								play(0, 0, 0);
								stop(0, 0, 0);
							}
						}
						
						else
						{
							stop(0, 0, 0);
							
							new_song();
							import_ahx(f);
							fclose(f);
							
							kill_empty_patterns(&mused.song, NULL); //wasn't there
							optimize_duplicate_patterns(&mused.song, true);
							set_channels(mused.song.num_channels);
							
							mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
							play(0, 0, 0);
							stop(0, 0, 0);
						}
						
						goto end;
					}
					
					if((strcmp(".xm", extension)) == 0 || (strcmp(".XM", extension)) == 0)
					{
						debug("Dropped an .XM file");
					
						if(mused.modified)
						{
							if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current song?"))
							{
								stop(0, 0, 0);
								
								new_song();
								import_xm(f);
								fclose(f);
								
								kill_empty_patterns(&mused.song, NULL); //wasn't there
								optimize_duplicate_patterns(&mused.song, true);
								set_channels(mused.song.num_channels);
								
								mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
								play(0, 0, 0);
								stop(0, 0, 0);
							}
						}
						
						else
						{
							stop(0, 0, 0);
							
							new_song();
							import_xm(f);
							fclose(f);
							
							kill_empty_patterns(&mused.song, NULL); //wasn't there
							optimize_duplicate_patterns(&mused.song, true);
							set_channels(mused.song.num_channels);
							
							mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
							play(0, 0, 0);
							stop(0, 0, 0);
						}
						
						goto end;
					}
					
					if((strcmp(".org", mod_extension)) == 0 || (strcmp(".ORG", mod_extension)) == 0)
					{
						debug("Dropped an .ORG file");
					
						if(mused.modified)
						{
							if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current song?"))
							{
								stop(0, 0, 0);
								
								new_song();
								import_org(f);
								fclose(f);
								
								kill_empty_patterns(&mused.song, NULL); //wasn't there
								optimize_duplicate_patterns(&mused.song, true);
								set_channels(mused.song.num_channels);
								
								mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
								play(0, 0, 0);
								stop(0, 0, 0);
							}
						}
						
						else
						{
							stop(0, 0, 0);
							
							new_song();
							import_org(f);
							fclose(f);
							
							kill_empty_patterns(&mused.song, NULL); //wasn't there
							optimize_duplicate_patterns(&mused.song, true);
							set_channels(mused.song.num_channels);
							
							mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
							play(0, 0, 0);
							stop(0, 0, 0);
						}
						
						goto end;
					}
					
					if((strcmp(".sid", mod_extension)) == 0 || (strcmp(".SID", mod_extension)) == 0)
					{
						debug("Dropped a .sid file");
					
						if(mused.modified)
						{
							if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current song?"))
							{
								stop(0, 0, 0);
								
								new_song();
								import_hubbard(f);
								fclose(f);
								
								set_channels(mused.song.num_channels);
								
								mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
								play(0, 0, 0);
								stop(0, 0, 0);
							}
						}
						
						else
						{
							stop(0, 0, 0);
							
							new_song();
							import_hubbard(f);
							fclose(f);
							
							set_channels(mused.song.num_channels);
							
							mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
							play(0, 0, 0);
							stop(0, 0, 0);
						}
						
						goto end;
					}
					
					if((strcmp(".fzt", mod_extension)) == 0)
					{
						debug("Dropped an .FZT file");
					
						if(mused.modified)
						{
							if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current song?"))
							{
								stop(0, 0, 0);
								
								new_song();
								import_fzt(f);
								fclose(f);
								
								kill_empty_patterns(&mused.song, NULL); //wasn't there
								optimize_duplicate_patterns(&mused.song, true);
								set_channels(mused.song.num_channels);
								
								mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
								play(0, 0, 0);
								stop(0, 0, 0);
							}
						}
						
						else
						{
							stop(0, 0, 0);
							
							new_song();
							import_fzt(f);
							fclose(f);
							
							kill_empty_patterns(&mused.song, NULL); //wasn't there
							optimize_duplicate_patterns(&mused.song, true);
							set_channels(mused.song.num_channels);
							
							mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
							play(0, 0, 0);
							stop(0, 0, 0);
						}
						
						goto end;
					}

					if((strcmp(".ftm", mod_extension)) == 0)
					{
						debug("Dropped an .FTM file");
					
						if(mused.modified)
						{
							if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current song?"))
							{
								stop(0, 0, 0);
								
								new_song();
								import_famitracker(f, IMPORT_FTM);
								fclose(f);

								set_channels(mused.song.num_channels);
								
								mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
								play(0, 0, 0);
								stop(0, 0, 0);
							}
						}
						
						else
						{
							stop(0, 0, 0);
							
							new_song();
							import_famitracker(f, IMPORT_FTM);
							fclose(f);

							set_channels(mused.song.num_channels);
							
							mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
							play(0, 0, 0);
							stop(0, 0, 0);
						}
						
						goto end;
					}

					if((strcmp(".0cc", mod_extension)) == 0)
					{
						debug("Dropped a .0CC file");
					
						if(mused.modified)
						{
							if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current song?"))
							{
								stop(0, 0, 0);
								
								new_song();
								import_famitracker(f, IMPORT_0CC);
								fclose(f);

								set_channels(mused.song.num_channels);
								
								mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
								play(0, 0, 0);
								stop(0, 0, 0);
							}
						}
						
						else
						{
							stop(0, 0, 0);
							
							new_song();
							import_famitracker(f, IMPORT_0CC);
							fclose(f);
							
							set_channels(mused.song.num_channels);
							
							mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
							play(0, 0, 0);
							stop(0, 0, 0);
						}
						
						goto end;
					}

					if((strcmp(".dnm", mod_extension)) == 0)
					{
						debug("Dropped a .DNM file");
					
						if(mused.modified)
						{
							if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current song?"))
							{
								stop(0, 0, 0);
								
								new_song();
								import_famitracker(f, IMPORT_DNM);
								fclose(f);
								
								set_channels(mused.song.num_channels);
								
								mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
								play(0, 0, 0);
								stop(0, 0, 0);
							}
						}
						
						else
						{
							stop(0, 0, 0);
							
							new_song();
							import_famitracker(f, IMPORT_DNM);
							fclose(f);
							
							set_channels(mused.song.num_channels);
							
							mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
							play(0, 0, 0);
							stop(0, 0, 0);
						}
						
						goto end;
					}
					
					if((strcmp(".fur", mod_extension)) == 0)
					{
						debug("Dropped a .FUR file");
					
						if(mused.modified)
						{
							if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current song?"))
							{
								stop(0, 0, 0);
								
								new_song();
								import_fur(f);
								fclose(f);
								
								set_channels(mused.song.num_channels);
								
								mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
								play(0, 0, 0);
								stop(0, 0, 0);
							}
						}
						
						else
						{
							stop(0, 0, 0);
							
							new_song();
							import_fur(f);
							fclose(f);
							
							set_channels(mused.song.num_channels);
							
							mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
							play(0, 0, 0);
							stop(0, 0, 0);
						}
						
						goto end;
					}

					msgbox(domain, mused.slider_bevel, &mused.largefont, "Not a song klystrack can open or import!", MB_OK);
				}

				mused.current_patternx = mused.current_sequencepos = mused.current_patternpos = mused.pattern_position = mused.pattern_horiz_position = mused.sequence_horiz_position = 0;
			}
			
			else
			{
				msgbox(domain, mused.slider_bevel, &mused.largefont, "Failed to load file!", MB_OK);
			}
			
			break;
		}
		
		case EDITINSTRUMENT:
		case EDIT4OP:
		{
			load_inst:;
			
			if(f)
			{
				if(strcmp(".ki", extension) == 0)
				{
					debug("Dropped an instrument");
					
					if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current instrument?"))
					{
						open_instrument(f);
						fclose(f);
					}
				}
				
				else
				{
					msgbox(domain, mused.slider_bevel, &mused.largefont, "Not a klystrack instrument (.ki)!", MB_OK);
				}
			}
			
			else
			{
				msgbox(domain, mused.slider_bevel, &mused.largefont, "Failed to load file!", MB_OK);
			}
			
			break;
		}
		
		case EDITFX:
		{
			if(f)
			{
				if(strcmp(".kx", extension) == 0)
				{
					debug("Dropped an instrument");
					
					if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current FX bus?"))
					{
						open_fx(f);
						fclose(f);
					}
				}
				
				else
				{
					msgbox(domain, mused.slider_bevel, &mused.largefont, "Not a klystrack FX bus (.kx)!", MB_OK);
				}
			}
			
			else
			{
				msgbox(domain, mused.slider_bevel, &mused.largefont, "Failed to load file!", MB_OK);
			}
			
			break;
		}
		
		case EDITWAVETABLE:
		case EDITLOCALSAMPLE:
		{
			load_wave:;
			
			if(f)
			{
				if((mused.flags & SHOW_WAVEGEN) && mused.mode != EDITLOCALSAMPLE)
				{
					if(strcmp(".kw", extension) == 0)
					{
						debug("Dropped a wavepatch");
						
						if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current wavepatch?"))
						{
							open_wavepatch(f);
							fclose(f);
						}
					}
					
					else
					{
						msgbox(domain, mused.slider_bevel, &mused.largefont, "Not a klystrack wavepatch (.kw)!", MB_OK);
					}
				}
				
				else
				{
					const char* wav_extension = &dropped_filedir[strlen(dropped_filedir) - 4];
					debug("Wav extension %s", extension);
					
					if(strcmp(".wav", wav_extension) == 0)
					{
						debug("Dropped a .wav file");
						
						if(confirm(domain, mused.slider_bevel, &mused.largefont, "Overwrite current wavetable?"))
						{
							open_wavetable(f);
							fclose(f);
						}
					}
					
					else
					{
						msgbox(domain, mused.slider_bevel, &mused.largefont, "Not a wave file (.wav)!", MB_OK);
					}
				}
			}
			
			else
			{
				msgbox(domain, mused.slider_bevel, &mused.largefont, "Failed to load file!", MB_OK);
			}
			
			break;
		}
		
		default: break;
	}
	
	end:;
	
	SDL_free(dropped_filedir);
}

void env_editor_add_param(int a)
{
	MusInstrument *i = &mused.song.instrument[mused.current_instrument];

	if (a < 0) a = -1; else if (a > 0) a = 1;

	if (SDL_GetModState() & KMOD_SHIFT)
	{
		switch (mused.env_selected_param)
		{
			default: a *= 16; break;
		}
	}
	
	if (SDL_GetModState() & KMOD_CTRL) //wasn't there
	{
		switch (mused.env_selected_param)
		{
			default: a *= 256; break;
		}
	}

	switch (mused.env_selected_param)
	{
		case ENV_ENABLE_VOLUME_ENVELOPE:
		{
			flipbit(i->flags, MUS_INST_USE_VOLUME_ENVELOPE);
			break;
		}
		
		case ENV_VOLUME_ENVELOPE_FADEOUT:
		{
			clamp(i->vol_env_fadeout, a, 0, 0xFFF);
			break;
		}
		
		case ENV_VOLUME_ENVELOPE_SCALE:
		{
			a *= -1;
			
			if(mused.vol_env_scale + a == 0)
			{
				if(a > 0)
				{
					mused.vol_env_scale = 1;
				}
				
				if(a < 0)
				{
					mused.vol_env_scale = -1;
				}
			}
			
			else
			{
				clamp(mused.vol_env_scale, a, -1, 5);
			}
			
			break;
		}
		
		case ENV_ENABLE_VOLUME_ENVELOPE_SUSTAIN:
		{
			flipbit(i->vol_env_flags, MUS_ENV_SUSTAIN);
			break;
		}
		
		case ENV_VOLUME_ENVELOPE_SUSTAIN_POINT:
		{
			clamp(i->vol_env_sustain, a, 0, mused.song.instrument[mused.current_instrument].num_vol_points - 1);
			break;
		}
		
		case ENV_ENABLE_VOLUME_ENVELOPE_LOOP:
		{
			flipbit(i->vol_env_flags, MUS_ENV_LOOP);
			break;
		}
		
		case ENV_VOLUME_ENVELOPE_LOOP_BEGIN:
		{
			clamp(i->vol_env_loop_start, a, 0, mused.song.instrument[mused.current_instrument].vol_env_loop_end);
			break;
		}
		
		case ENV_VOLUME_ENVELOPE_LOOP_END:
		{
			clamp(i->vol_env_loop_end, a, i->vol_env_loop_start, mused.song.instrument[mused.current_instrument].num_vol_points - 1);
			break;
		}
		
		//===================
		
		case ENV_ENABLE_PANNING_ENVELOPE:
		{
			flipbit(i->flags, MUS_INST_USE_PANNING_ENVELOPE);
			break;
		}
		
		case ENV_PANNING_ENVELOPE_FADEOUT:
		{
			clamp(i->pan_env_fadeout, a, 0, 0xFFF);
			break;
		}
		
		case ENV_PANNING_ENVELOPE_SCALE:
		{
			a *= -1;
			
			if(mused.pan_env_scale + a == 0)
			{
				if(a > 0)
				{
					mused.pan_env_scale = 1;
				}
				
				if(a < 0)
				{
					mused.pan_env_scale = -1;
				}
			}
			
			else
			{
				clamp(mused.pan_env_scale, a, -1, 5);
			}
			
			break;
		}
		
		case ENV_ENABLE_PANNING_ENVELOPE_SUSTAIN:
		{
			flipbit(i->pan_env_flags, MUS_ENV_SUSTAIN);
			break;
		}
		
		case ENV_PANNING_ENVELOPE_SUSTAIN_POINT:
		{
			clamp(i->pan_env_sustain, a, 0, mused.song.instrument[mused.current_instrument].num_pan_points - 1);
			break;
		}
		
		case ENV_ENABLE_PANNING_ENVELOPE_LOOP:
		{
			flipbit(i->vol_env_flags, MUS_ENV_LOOP);
			break;
		}
		
		case ENV_PANNING_ENVELOPE_LOOP_BEGIN:
		{
			clamp(i->pan_env_loop_start, a, 0, mused.song.instrument[mused.current_instrument].pan_env_loop_end);
			break;
		}
		
		case ENV_PANNING_ENVELOPE_LOOP_END:
		{
			clamp(i->pan_env_loop_end, a, i->pan_env_loop_start, mused.song.instrument[mused.current_instrument].num_pan_points - 1);
			break;
		}
	}
}

void edit_env_editor_event(SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:

		switch (e->key.keysym.sym)
		{
			case SDLK_DOWN:
			{
				++mused.env_selected_param;

				if (mused.focus == EDITENVELOPE)
				{
					if (mused.env_selected_param >= ENV_PARAMS) mused.env_selected_param = ENV_PARAMS - 1;
				}
			}
			break;

			case SDLK_UP:
			{
				--mused.env_selected_param;

				if (mused.env_selected_param < 0) mused.env_selected_param = 0;
			}
			break;

			case SDLK_DELETE:
			{
				
			}
			break;

			case SDLK_RIGHT:
			{
				env_editor_add_param(+1);
			}
			break;

			case SDLK_LEFT:
			{
				env_editor_add_param(-1);
			}
			break;

			default:
			{
				play_the_jams(e->key.keysym.sym, -1, 1);
			}
			break;
		}

		break;

		case SDL_KEYUP:
		
			play_the_jams(e->key.keysym.sym, -1, 0);

		break;
	}
}

void fourop_env_editor_add_param(int a)
{
	MusInstrument *i = &mused.song.instrument[mused.current_instrument];

	if (a < 0) a = -1; else if (a > 0) a = 1;

	if (SDL_GetModState() & KMOD_SHIFT)
	{
		switch (mused.fourop_env_selected_param)
		{
			default: a *= 16; break;
		}
	}
	
	if (SDL_GetModState() & KMOD_CTRL) //wasn't there
	{
		switch (mused.fourop_env_selected_param)
		{
			default: a *= 256; break;
		}
	}

	switch (mused.fourop_env_selected_param)
	{
		case FOUROP_ENV_ENABLE_VOLUME_ENVELOPE:
		{
			flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_USE_VOLUME_ENVELOPE);
			break;
		}
		
		case FOUROP_ENV_VOLUME_ENVELOPE_FADEOUT:
		{
			clamp(i->ops[mused.selected_operator - 1].vol_env_fadeout, a, 0, 0xFFF);
			break;
		}
		
		case FOUROP_ENV_VOLUME_ENVELOPE_SCALE:
		{
			a *= -1;
			
			if(mused.fourop_vol_env_scale + a == 0)
			{
				if(a > 0)
				{
					mused.fourop_vol_env_scale = 1;
				}
				
				if(a < 0)
				{
					mused.fourop_vol_env_scale = -1;
				}
			}
			
			else
			{
				clamp(mused.fourop_vol_env_scale, a, -1, 5);
			}
			
			break;
		}
		
		case FOUROP_ENV_ENABLE_VOLUME_ENVELOPE_SUSTAIN:
		{
			flipbit(i->ops[mused.selected_operator - 1].vol_env_flags, MUS_ENV_SUSTAIN);
			break;
		}
		
		case FOUROP_ENV_VOLUME_ENVELOPE_SUSTAIN_POINT:
		{
			clamp(i->ops[mused.selected_operator - 1].vol_env_sustain, a, 0, mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points - 1);
			break;
		}
		
		case FOUROP_ENV_ENABLE_VOLUME_ENVELOPE_LOOP:
		{
			flipbit(i->ops[mused.selected_operator - 1].vol_env_flags, MUS_ENV_LOOP);
			break;
		}
		
		case FOUROP_ENV_VOLUME_ENVELOPE_LOOP_BEGIN:
		{
			clamp(i->ops[mused.selected_operator - 1].vol_env_loop_start, a, 0, mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].vol_env_loop_end);
			break;
		}
		
		case FOUROP_ENV_VOLUME_ENVELOPE_LOOP_END:
		{
			clamp(i->ops[mused.selected_operator - 1].vol_env_loop_end, a, i->ops[mused.selected_operator - 1].vol_env_loop_start, mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].num_vol_points - 1);
			break;
		}
	}
}

void edit_4op_env_editor_event(SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:

		switch (e->key.keysym.sym)
		{
			case SDLK_RETURN:
			{
				//if (mused.selected_param == P_NAME)
					//set_edit_buffer(mused.song.instrument[mused.current_instrument].name, sizeof(mused.song.instrument[mused.current_instrument].name));
			}
			break;

			case SDLK_DOWN:
			{
				++mused.fourop_env_selected_param;

				if (mused.focus == EDITENVELOPE4OP)
				{
					if (mused.fourop_env_selected_param >= FOUROP_ENV_PARAMS) mused.fourop_env_selected_param = FOUROP_ENV_PARAMS - 1;
				}
			}
			break;

			case SDLK_UP:
			{
				--mused.fourop_env_selected_param;

				if (mused.fourop_env_selected_param < 0) mused.fourop_env_selected_param = 0;
			}
			break;

			case SDLK_DELETE:
			{
				
			}
			break;

			case SDLK_RIGHT:
			{
				fourop_env_editor_add_param(+1);
			}
			break;


			case SDLK_LEFT:
			{
				fourop_env_editor_add_param(-1);
			}
			break;

			default:
			{
				play_the_jams(e->key.keysym.sym, -1, 1);
			}
			break;
		}

		break;

		case SDL_KEYUP:
		
			play_the_jams(e->key.keysym.sym, -1, 0);

		break;
	}
}

void instrument_add_param(int a)
{
	MusInstrument *i = &mused.song.instrument[mused.current_instrument];

	if (a < 0) a = -1; else if (a > 0) a = 1;

	if (SDL_GetModState() & KMOD_SHIFT)
	{
		switch (mused.selected_param)
		{
			case P_BASENOTE: a *= 12; break;
			case P_FIXED_NOISE_BASE_NOTE: a *= 12; break;
			case P_BUZZ_SEMI: a *= 12; break;
			case P_FM_BASENOTE: a *= 12; break; //wasn't there
			case P_PHASE_RESET_TIMER_NOTE: a *= 12; break; //wasn't there
			
			default: a *= 16; break;
		}
	}
	
	if (SDL_GetModState() & KMOD_CTRL) //wasn't there
	{
		switch (mused.selected_param)
		{
			default: a *= 256; break;
		}
	}

	switch (mused.selected_param)
	{
		case P_INSTRUMENT:

		clamp(mused.current_instrument, a, 0, NUM_INSTRUMENTS - 1);

		break;

		case P_BASENOTE:

		clamp(i->base_note, a, 0, FREQ_TAB_SIZE - 1);

		break;
		
		case P_FIXED_NOISE_BASE_NOTE:

		clamp(i->noise_note, a, 0, FREQ_TAB_SIZE - 1);

		break;
		
		case P_1_BIT_NOISE:

		flipbit(i->cydflags, CYD_CHN_ENABLE_1_BIT_NOISE);

		break;

		case P_FINETUNE:

		clamp(i->finetune, a, -128, 127);

		break;

		case P_LOCKNOTE:

		flipbit(i->flags, MUS_INST_LOCK_NOTE);

		break;

		case P_DRUM:

		flipbit(i->flags, MUS_INST_DRUM);

		break;

		case P_KEYSYNC:

		flipbit(i->cydflags, CYD_CHN_ENABLE_KEY_SYNC);

		break;

		case P_SYNC:

		flipbit(i->cydflags, CYD_CHN_ENABLE_SYNC);

		break;

		case P_SYNCSRC:
		{
			int x = (Uint8)(i->sync_source+1);
			clamp(x, a, 0, MUS_MAX_CHANNELS);
			i->sync_source = x-1;
		}
		break;

		case P_WAVE:

		flipbit(i->cydflags, CYD_CHN_ENABLE_WAVE);

		break;

		case P_WAVE_ENTRY:

		clamp(i->wavetable_entry, a, 0, CYD_WAVE_MAX_ENTRIES - 1);

		break;

		case P_WAVE_OVERRIDE_ENV:

		flipbit(i->cydflags, CYD_CHN_WAVE_OVERRIDE_ENV);

		break;

		case P_WAVE_LOCK_NOTE:

		flipbit(i->flags, MUS_INST_WAVE_LOCK_NOTE);

		break;

		case P_PULSE:

		flipbit(i->cydflags, CYD_CHN_ENABLE_PULSE);

		break;

		case P_SAW:

		flipbit(i->cydflags, CYD_CHN_ENABLE_SAW);

		break;

		case P_TRIANGLE:

		flipbit(i->cydflags, CYD_CHN_ENABLE_TRIANGLE);

		break;

		case P_LFSR:

		flipbit(i->cydflags, CYD_CHN_ENABLE_LFSR);

		break;

		case P_LFSRTYPE:

		clamp(i->lfsr_type, a, 0, CYD_NUM_LFSR - 1);

		break;

		case P_NOISE:

		flipbit(i->cydflags, CYD_CHN_ENABLE_NOISE);

		break;

		case P_METAL:

		flipbit(i->cydflags, CYD_CHN_ENABLE_METAL);

		break;
		
		case P_OSCMIXMODE: //wasn't there
		
		clamp(i->mixmode, a, 0, 4);

		break;

		case P_RELVOL:

		flipbit(i->flags, MUS_INST_RELATIVE_VOLUME);

		break;

		case P_FX:

		flipbit(i->cydflags, CYD_CHN_ENABLE_FX);

		break;

		case P_FXBUS:

		clamp(i->fx_bus, a, 0, CYD_MAX_FX_CHANNELS - 1);

		break;

		case P_ATTACK:

		clamp(i->adsr.a, a, 0, 32 * ENVELOPE_SCALE - 1);

		break;

		case P_DECAY:

		clamp(i->adsr.d, a, 0, 32 * ENVELOPE_SCALE - 1);

		break;

		case P_SUSTAIN:

		clamp(i->adsr.s, a, 0, 31);

		break;

		case P_RELEASE:

		clamp(i->adsr.r, a, 0, 32 * ENVELOPE_SCALE - 1);

		break;
		
		
		case P_EXP_VOL:
		
		flipbit(i->cydflags, CYD_CHN_ENABLE_EXPONENTIAL_VOLUME);
		
		break;
		
		case P_EXP_ATTACK:
		
		flipbit(i->cydflags, CYD_CHN_ENABLE_EXPONENTIAL_ATTACK);
		
		break;
		
		case P_EXP_DECAY:
		
		flipbit(i->cydflags, CYD_CHN_ENABLE_EXPONENTIAL_DECAY);
		
		break;
		
		case P_EXP_RELEASE:
		
		flipbit(i->cydflags, CYD_CHN_ENABLE_EXPONENTIAL_RELEASE);
		
		break;
		
		
		case P_VOL_KSL:
		
		flipbit(i->cydflags, CYD_CHN_ENABLE_VOLUME_KEY_SCALING);
		
		break;
		
		case P_VOL_KSL_LEVEL:
		
		clamp(i->vol_ksl_level, a, 0, 255);
		
		break;
		
		case P_ENV_KSL:
		
		flipbit(i->cydflags, CYD_CHN_ENABLE_ENVELOPE_KEY_SCALING);
		
		break;
		
		case P_ENV_KSL_LEVEL:
		
		clamp(i->env_ksl_level, a, 0, 255);
		
		break;
		
		

		case P_BUZZ:

		flipbit(i->flags, MUS_INST_YM_BUZZ);

		break;

		case P_BUZZ_SHAPE:

		clamp(i->ym_env_shape, a, 0, 3);

		break;
		
		case P_BUZZ_ENABLE_AY8930:
		
		flipbit(i->cydflags, CYD_CHN_ENABLE_AY8930_BUZZ_MODE);
		
		break;

		case P_BUZZ_FINE:
		{
			clamp(i->buzz_offset, a, -32768, 32767);
		}
		break;

		case P_BUZZ_SEMI:
		{
			int f = i->buzz_offset >> 8;
			clamp(f, a, -99, 99);
			i->buzz_offset = (i->buzz_offset & 0xff) | f << 8;
		}
		break;

		case P_PW:

		clamp(i->pw, a, 0, 0xfff); //was `clamp(i->pw, a*16, 0, 0x7ff);`

		break;

		case P_1_4TH:
		
		flipbit(i->flags, MUS_INST_QUARTER_FREQ);
		
		break;
		
		case P_SINE:
		
		flipbit(i->cydflags, CYD_CHN_ENABLE_SINE);
		
		break;
		
		case P_SINE_PHASE_SHIFT:

		clamp(i->sine_acc_shift, a, 0, 0xf);

		break;
		
		case P_FIX_NOISE_PITCH:
		
		flipbit(i->cydflags, CYD_CHN_ENABLE_FIXED_NOISE_PITCH);
		
		break;

		case P_VOLUME:

		clamp(i->volume, a, 0, 255); // 255 = ~2x boost

		break;

		case P_PROGPERIOD:

		clamp(i->prog_period[mused.current_instrument_program], a, 0, 0xff);

		break;

		case P_SLIDESPEED:

		clamp(i->slide_speed, a, 0, 0xfff);

		break;
		
		

		case P_VIBDEPTH:

		clamp(i->vibrato_depth, a, 0, 0xff);

		break;

		case P_VIBDELAY:

		clamp(i->vibrato_delay, a, 0, 0xff);

		break;

		case P_VIBSHAPE:

		clamp(i->vibrato_shape, a, 0, MUS_NUM_SHAPES - 1);

		break;

		case P_VIBSPEED:

		clamp(i->vibrato_speed, a, 0, 0xff);

		break;
		
		
		case P_FM_VIBDEPTH:

		clamp(i->fm_vibrato_depth, a, 0, 0xff);

		break;

		case P_FM_VIBDELAY:

		clamp(i->fm_vibrato_delay, a, 0, 0xff);

		break;

		case P_FM_VIBSHAPE:

		clamp(i->fm_vibrato_shape, a, 0, MUS_NUM_SHAPES - 1);

		break;

		case P_FM_VIBSPEED:

		clamp(i->fm_vibrato_speed, a, 0, 0xff);

		break;
		
		

		case P_PWMDEPTH:

		clamp(i->pwm_depth, a, 0, 0xff);

		break;

		case P_PWMSPEED:

		clamp(i->pwm_speed, a, 0, 0xff);

		break;

		case P_PWMSHAPE:

		clamp(i->pwm_shape, a, 0, MUS_NUM_SHAPES - 1);

		break;
		
		case P_PWMDELAY: //wasn't there

		clamp(i->pwm_delay, a, 0, 0xff);

		break;
		
		
		
		case P_TREMDEPTH:

		clamp(i->tremolo_depth, a, 0, 0xff);

		break;

		case P_TREMSPEED:

		clamp(i->tremolo_speed, a, 0, 0xff);

		break;

		case P_TREMSHAPE:

		clamp(i->tremolo_shape, a, 0, MUS_NUM_SHAPES - 1);

		break;
		
		case P_TREMDELAY: //wasn't there

		clamp(i->tremolo_delay, a, 0, 0xff);

		break;
		
		
		
		case P_FM_TREMDEPTH:

		clamp(i->fm_tremolo_depth, a, 0, 0xff);

		break;

		case P_FM_TREMSPEED:

		clamp(i->fm_tremolo_speed, a, 0, 0xff);

		break;

		case P_FM_TREMSHAPE:

		clamp(i->fm_tremolo_shape, a, 0, MUS_NUM_SHAPES - 1);

		break;
		
		case P_FM_TREMDELAY: //wasn't there

		clamp(i->fm_tremolo_delay, a, 0, 0xff);

		break;
		
		

		case P_RINGMOD:

		flipbit(i->cydflags, CYD_CHN_ENABLE_RING_MODULATION);

		break;

		case P_SETPW:

		flipbit(i->flags, MUS_INST_SET_PW);

		break;

		case P_SETCUTOFF:

		flipbit(i->flags, MUS_INST_SET_CUTOFF);

		break;

		case P_INVVIB:

		flipbit(i->flags, MUS_INST_INVERT_VIBRATO_BIT);

		break;
		
		case P_INVTREM:

		flipbit(i->flags, MUS_INST_INVERT_TREMOLO_BIT);

		break;

		case P_FILTER:

		flipbit(i->cydflags, CYD_CHN_ENABLE_FILTER);

		break;

		case P_RINGMODSRC:
		{
			int x = (Uint8)(i->ring_mod + 1);
			clamp(x, a, 0, MUS_MAX_CHANNELS);
			i->ring_mod = x - 1;
		}
		break;

		case P_CUTOFF:

		clamp(i->cutoff, a, 0, 4095); //was `clamp(i->cutoff, a*16, 0, 2047);`

		break;

		case P_RESONANCE:

		clamp(i->resonance, a, 0, 0xff);  //was `0, 3)`

		break;
		
		case P_SLOPE: //wasn't there

		clamp(i->slope, a, 0, 5);  //was `0, 3)`

		break;
		
		case P_NUM_OF_MACROS: //wasn't there

		//clamp(i->num_macros, a, 0, MUS_MAX_MACROS_INST);  //was `0, 3)`
		
		if(mused.current_instrument_program + a >= 0 && mused.current_instrument_program + a < MUS_MAX_MACROS_INST)
		{
			if(a == 1)
			{
				if(!(is_empty_program(i->program[mused.current_instrument_program])))
				{
					mused.current_instrument_program++;
					
					if(i->program[i->num_macros] == NULL)
					{
						i->num_macros++;
						
						i->program[i->num_macros - 1] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
						i->program_unite_bits[i->num_macros - 1] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
						
						for (int p = 0; p < MUS_PROG_LEN; ++p)
						{
							i->program[i->num_macros - 1][p] = MUS_FX_NOP;
						}
						
						for (int p = 0; p < MUS_PROG_LEN / 8 + 1; ++p)
						{
							i->program_unite_bits[i->num_macros - 1][p] = 0;
						}
					}
				}
			}
			
			if(a == -1)
			{
				mused.current_instrument_program--;
			}
		}

		break;

		case P_FLTTYPE:

		clamp(i->flttype, a, 0, FLT_TYPES - 1);

		break;

		case P_NORESTART:

		flipbit(i->flags, MUS_INST_NO_PROG_RESTART);

		break;

		case P_MULTIOSC:

		flipbit(i->flags, MUS_INST_MULTIOSC);

		break;
		
		case P_SAVE_LFO_SETTINGS:

		flipbit(i->flags, MUS_INST_SAVE_LFO_SETTINGS);

		break;

		case P_FM_MODULATION:

		clamp(i->fm_modulation, a, 0, 0xff);

		break;

		case P_FM_FEEDBACK:

		clamp(i->fm_feedback, a, 0, 0x7);

		break;
		
		
		
		
		case P_FM_VOL_KSL_ENABLE:

		flipbit(i->fm_flags, CYD_FM_ENABLE_VOLUME_KEY_SCALING);

		break;
		
		case P_FM_VOL_KSL_LEVEL:

		clamp(i->fm_vol_ksl_level, a, 0, 0xff);

		break;
		
		case P_FM_ENV_KSL_ENABLE:

		flipbit(i->fm_flags, CYD_FM_ENABLE_ENVELOPE_KEY_SCALING);

		break;
		
		case P_FM_ENV_KSL_LEVEL:

		clamp(i->fm_env_ksl_level, a, 0, 0xff);

		break;
		
		
		
		
		
		case P_FM_BASENOTE: //weren't there

		clamp(i->fm_base_note, a, 0, FREQ_TAB_SIZE - 1);

		break;

		case P_FM_FINETUNE:

		clamp(i->fm_finetune, a, -128, 127);

		break;
		
		
		
		
		case P_FM_FREQ_LUT:

		clamp(i->fm_freq_LUT, a, 0, 1);

		break;
		
		
		

		case P_FM_ATTACK:

		clamp(i->fm_adsr.a, a, 0, 32 * ENVELOPE_SCALE - 1);

		break;

		case P_FM_DECAY:

		clamp(i->fm_adsr.d, a, 0, 32 * ENVELOPE_SCALE - 1);

		break;

		case P_FM_SUSTAIN:

		clamp(i->fm_adsr.s, a, 0, 31);

		break;

		case P_FM_RELEASE:

		clamp(i->fm_adsr.r, a, 0, 32 * ENVELOPE_SCALE - 1);

		break;
		
		
		case P_FM_EXP_VOL: //wasn't there

		flipbit(i->fm_flags, CYD_FM_ENABLE_EXPONENTIAL_VOLUME);

		break;
		
		case P_FM_EXP_ATTACK: //wasn't there

		flipbit(i->fm_flags, CYD_FM_ENABLE_EXPONENTIAL_ATTACK);

		break;
		
		case P_FM_EXP_DECAY: //wasn't there

		flipbit(i->fm_flags, CYD_FM_ENABLE_EXPONENTIAL_DECAY);

		break;
		
		case P_FM_EXP_RELEASE: //wasn't there

		flipbit(i->fm_flags, CYD_FM_ENABLE_EXPONENTIAL_RELEASE);

		break;
		

		case P_FM_ENV_START:

		clamp(i->fm_attack_start, a, 0, 31);

		break;

		case P_FM_WAVE:

		flipbit(i->fm_flags, CYD_FM_ENABLE_WAVE);

		break;
		
		case P_FM_ADDITIVE: //wasn't there

		flipbit(i->fm_flags, CYD_FM_ENABLE_ADDITIVE);

		break;
		
		case P_FM_SAVE_LFO_SETTINGS: //wasn't there

		flipbit(i->fm_flags, CYD_FM_SAVE_LFO_SETTINGS);

		break;
		
		case P_FM_ENABLE_4OP: //wasn't there

		flipbit(i->fm_flags, CYD_FM_ENABLE_4OP);

		break;

		case P_ENABLE_PHASE_RESET_TIMER:

		flipbit(i->cydflags, CYD_CHN_ENABLE_PHASE_RESET_TIMER);

		break;
		
		case P_PHASE_RESET_TIMER_NOTE:
		
		clamp(i->phase_reset_timer_note, a, 0, FREQ_TAB_SIZE - 1);

		break;
		
		case P_PHASE_RESET_TIMER_FINETUNE:
		
		clamp(i->phase_reset_timer_finetune, a, -128, 127);

		break;
		
		case P_LINK_PHASE_RESET_TIMER_NOTE:
		
		flipbit(i->flags, MUS_INST_LINK_PHASE_RESET_TIMER_NOTE);

		break;

		case P_FM_ENABLE:

		flipbit(i->cydflags, CYD_CHN_ENABLE_FM);

		break;

		case P_FM_HARMONIC_CARRIER:
		{
			Uint8 carrier = (i->fm_harmonic >> 4);
			Uint8 modulator = i->fm_harmonic & 0xf;

			clamp(carrier, a, 0, 15);

			i->fm_harmonic = carrier << 4 | modulator;
		}
		break;

		case P_FM_HARMONIC_MODULATOR:
		{
			Uint8 carrier = (i->fm_harmonic >> 4);
			Uint8 modulator = i->fm_harmonic & 0xf;

			clamp(modulator, a, 0, 15);

			i->fm_harmonic = carrier << 4 | modulator;
		}
		break;


		case P_FM_WAVE_ENTRY:

		clamp(i->fm_wave, a, 0, CYD_WAVE_MAX_ENTRIES - 1);

		break;


		default:
		break;
	}
}

void four_op_add_param(int a)
{
	MusInstrument *i = &mused.song.instrument[mused.current_instrument];

	if (a < 0) a = -1; else if (a > 0) a = 1;

	if (SDL_GetModState() & KMOD_SHIFT)
	{
		switch (mused.fourop_selected_param)
		{
			case FOUROP_BASENOTE: 
			case FOUROP_FIXED_NOISE_BASE_NOTE:
			case FOUROP_PHASE_RESET_TIMER_NOTE:
			case FOUROP_CSM_TIMER_NOTE: a *= 12; break;
			
			default: a *= 16; break;
		}
	}
	
	if (SDL_GetModState() & KMOD_CTRL) //wasn't there
	{
		switch (mused.fourop_selected_param)
		{
			default: a *= 256; break;
		}
	}

	switch (mused.fourop_selected_param)
	{
		case FOUROP_3CH_EXP_MODE:

		flipbit(i->fm_flags, CYD_FM_ENABLE_3CH_EXP_MODE);

		break;
		
		case FOUROP_ALG:

		clamp(i->alg, a, 1, 13);

		break;
		
		case FOUROP_MASTER_VOL:

		clamp(i->fm_4op_vol, a, 0, 0xff);

		break;
		
		case FOUROP_USE_MAIN_INST_PROG:

		flipbit(i->fm_flags, CYD_FM_FOUROP_USE_MAIN_INST_PROG);

		break;
		
		case FOUROP_BYPASS_MAIN_INST_FILTER:

		flipbit(i->fm_flags, CYD_FM_FOUROP_BYPASS_MAIN_INST_FILTER);

		break;
		
		case FOUROP_ENABLE_SSG_EG:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_SSG_EG);

		break;
		
		case FOUROP_SSG_EG_TYPE:

		clamp(i->ops[mused.selected_operator - 1].ssg_eg_type, a, 0, 0x7);

		break;
		
		case FOUROP_BASENOTE:

		clamp(i->ops[mused.selected_operator - 1].base_note, a, 0, FREQ_TAB_SIZE - 1);

		break;
		
		case FOUROP_FINETUNE:

		clamp(i->ops[mused.selected_operator - 1].finetune, a, -128, 127);

		break;
		
		
		case FOUROP_DETUNE:

		clamp(i->ops[mused.selected_operator - 1].detune, a, -7, 7);

		break;
		
		case FOUROP_COARSE_DETUNE:

		clamp(i->ops[mused.selected_operator - 1].coarse_detune, a, 0, 3);

		break;
		
		case FOUROP_HARMONIC_CARRIER:
		{
			Uint8 carrier = (i->ops[mused.selected_operator - 1].harmonic >> 4);
			Uint8 modulator = i->ops[mused.selected_operator - 1].harmonic & 0xf;

			clamp(carrier, a, 0, 15);

			i->ops[mused.selected_operator - 1].harmonic = carrier << 4 | modulator;
		}
		break;
		
		case FOUROP_HARMONIC_MODULATOR:
		{
			Uint8 carrier = (i->ops[mused.selected_operator - 1].harmonic >> 4);
			Uint8 modulator = i->ops[mused.selected_operator - 1].harmonic & 0xf;

			clamp(modulator, a, 0, 15);

			i->ops[mused.selected_operator - 1].harmonic = carrier << 4 | modulator;
		}
		break;
		
		
		case FOUROP_FIXED_NOISE_BASE_NOTE:

		clamp(i->ops[mused.selected_operator - 1].noise_note, a, 0, FREQ_TAB_SIZE - 1);

		break;
		
		case FOUROP_1_BIT_NOISE:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_1_BIT_NOISE);

		break;

		case FOUROP_LOCKNOTE:

		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_LOCK_NOTE);

		break;

		case FOUROP_DRUM:

		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_DRUM);

		break;

		case FOUROP_KEYSYNC:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_KEY_SYNC);

		break;

		case FOUROP_SYNC:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_SYNC);

		break;

		case FOUROP_SYNCSRC:
		{
			int x = (Uint8)(i->ops[mused.selected_operator - 1].sync_source+1);
			clamp(x, a, 0, MUS_MAX_CHANNELS);
			i->ops[mused.selected_operator - 1].sync_source = x-1;
		}
		break;

		case FOUROP_WAVE:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_WAVE);

		break;

		case FOUROP_WAVE_ENTRY:

		clamp(i->ops[mused.selected_operator - 1].wavetable_entry, a, 0, CYD_WAVE_MAX_ENTRIES - 1);

		break;

		case FOUROP_WAVE_OVERRIDE_ENV:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_WAVE_OVERRIDE_ENV);

		break;

		case FOUROP_WAVE_LOCK_NOTE:

		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_WAVE_LOCK_NOTE);

		break;

		case FOUROP_PULSE:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_PULSE);

		break;

		case FOUROP_SAW:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_SAW);

		break;

		case FOUROP_TRIANGLE:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_TRIANGLE);

		break;

		case FOUROP_NOISE:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_NOISE);

		break;

		case FOUROP_METAL:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_METAL);

		break;
		
		case FOUROP_SINE:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_SINE);

		break;
		
		case FOUROP_SINE_PHASE_SHIFT: //wasn't there
		
		clamp(i->ops[mused.selected_operator - 1].sine_acc_shift, a, 0, 0xf);

		break;
		
		case FOUROP_OSCMIXMODE: //wasn't there
		
		clamp(i->ops[mused.selected_operator - 1].mixmode, a, 0, 4);

		break;

		case FOUROP_RELVOL:

		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_RELATIVE_VOLUME);

		break;

		case FOUROP_ATTACK:

		clamp(i->ops[mused.selected_operator - 1].adsr.a, a, 0, 0xFF);

		break;

		case FOUROP_DECAY:

		clamp(i->ops[mused.selected_operator - 1].adsr.d, a, 0, 0xFF);

		break;

		case FOUROP_SUSTAIN:

		clamp(i->ops[mused.selected_operator - 1].adsr.s, a, 0, 31);

		break;
		
		case FOUROP_SUSTAIN_RATE:

		clamp(i->ops[mused.selected_operator - 1].adsr.sr, a, 0, 0xFF);

		break;

		case FOUROP_RELEASE:

		clamp(i->ops[mused.selected_operator - 1].adsr.r, a, 0, 0xFF);

		break;
		
		
		case FOUROP_EXP_VOL:
		
		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_EXPONENTIAL_VOLUME);
		
		break;
		
		case FOUROP_EXP_ATTACK:
		
		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_EXPONENTIAL_ATTACK);
		
		break;
		
		case FOUROP_EXP_DECAY:
		
		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_EXPONENTIAL_DECAY);
		
		break;
		
		case FOUROP_EXP_RELEASE:
		
		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_EXPONENTIAL_RELEASE);
		
		break;
		
		
		case FOUROP_VOL_KSL:
		
		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_VOLUME_KEY_SCALING);
		
		break;
		
		case FOUROP_VOL_KSL_LEVEL:
		
		clamp(i->ops[mused.selected_operator - 1].vol_ksl_level, a, 0, 255);
		
		break;
		
		case FOUROP_ENV_KSL:
		
		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_ENVELOPE_KEY_SCALING);
		
		break;
		
		case FOUROP_ENV_KSL_LEVEL:
		
		clamp(i->ops[mused.selected_operator - 1].env_ksl_level, a, 0, 255);
		
		break;
		

		case FOUROP_PW:

		clamp(i->ops[mused.selected_operator - 1].pw, a, 0, 0xfff); //was `clamp(i->ops[mused.selected_operator - 1].pw, a*16, 0, 0x7ff);`

		break;

		case FOUROP_1_4TH:
		
		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_QUARTER_FREQ);
		
		break;
		
		case FOUROP_FIX_NOISE_PITCH:
		
		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_FIXED_NOISE_PITCH);
		
		break;

		case FOUROP_VOLUME:

		clamp(i->ops[mused.selected_operator - 1].volume, a, 0, 255); // 255 = ~2x boost

		break;

		case FOUROP_PROGPERIOD:

		clamp(i->ops[mused.selected_operator - 1].prog_period[mused.current_fourop_program[mused.selected_operator - 1]], a, 0, 0xff);

		break;

		case FOUROP_SLIDESPEED:

		clamp(i->ops[mused.selected_operator - 1].slide_speed, a, 0, 0xfff);

		break;
		
		

		case FOUROP_VIBDEPTH:

		clamp(i->ops[mused.selected_operator - 1].vibrato_depth, a, 0, 0xff);

		break;

		case FOUROP_VIBDELAY:

		clamp(i->ops[mused.selected_operator - 1].vibrato_delay, a, 0, 0xff);

		break;

		case FOUROP_VIBSHAPE:

		clamp(i->ops[mused.selected_operator - 1].vibrato_shape, a, 0, MUS_NUM_SHAPES - 1);

		break;

		case FOUROP_VIBSPEED:

		clamp(i->ops[mused.selected_operator - 1].vibrato_speed, a, 0, 0xff);

		break;
		

		case FOUROP_PWMDEPTH:

		clamp(i->ops[mused.selected_operator - 1].pwm_depth, a, 0, 0xff);

		break;

		case FOUROP_PWMSPEED:

		clamp(i->ops[mused.selected_operator - 1].pwm_speed, a, 0, 0xff);

		break;

		case FOUROP_PWMSHAPE:

		clamp(i->ops[mused.selected_operator - 1].pwm_shape, a, 0, MUS_NUM_SHAPES - 1);

		break;
		
		case FOUROP_PWMDELAY: //wasn't there

		clamp(i->ops[mused.selected_operator - 1].pwm_delay, a, 0, 0xff);

		break;
		
		
		
		case FOUROP_TREMDEPTH:

		clamp(i->ops[mused.selected_operator - 1].tremolo_depth, a, 0, 0xff);

		break;

		case FOUROP_TREMSPEED:

		clamp(i->ops[mused.selected_operator - 1].tremolo_speed, a, 0, 0xff);

		break;

		case FOUROP_TREMSHAPE:

		clamp(i->ops[mused.selected_operator - 1].tremolo_shape, a, 0, MUS_NUM_SHAPES - 1);

		break;
		
		case FOUROP_TREMDELAY: //wasn't there

		clamp(i->ops[mused.selected_operator - 1].tremolo_delay, a, 0, 0xff);

		break;


		case FOUROP_RINGMOD:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_RING_MODULATION);

		break;

		case FOUROP_SETPW:

		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_SET_PW);

		break;

		case FOUROP_SETCUTOFF:

		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_SET_CUTOFF);

		break;

		case FOUROP_INVVIB:

		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_INVERT_VIBRATO_BIT);

		break;
		
		case FOUROP_INVTREM:

		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_INVERT_TREMOLO_BIT);

		break;

		case FOUROP_FILTER:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_FILTER);

		break;

		case FOUROP_RINGMODSRC:
		{
			int x = (Uint8)(i->ops[mused.selected_operator - 1].ring_mod + 1);
			clamp(x, a, 0, MUS_MAX_CHANNELS);
			i->ops[mused.selected_operator - 1].ring_mod = x - 1;
		}
		break;

		case FOUROP_CUTOFF:

		clamp(i->ops[mused.selected_operator - 1].cutoff, a, 0, 4095); //was `clamp(i->ops[mused.selected_operator - 1].cutoff, a*16, 0, 2047);`

		break;

		case FOUROP_RESONANCE:

		clamp(i->ops[mused.selected_operator - 1].resonance, a, 0, 0xff);  //was `0, 3)`

		break;
		
		case FOUROP_SLOPE: //wasn't there

		clamp(i->ops[mused.selected_operator - 1].slope, a, 0, 5);  //was `0, 3)`

		break;
		
		case FOUROP_NUM_OF_MACROS: //wasn't there
		
		if(mused.current_fourop_program[mused.selected_operator - 1] + a >= 0 && mused.current_fourop_program[mused.selected_operator - 1] + a < MUS_MAX_MACROS_OP)
		{
			if(a == 1)
			{
				if(!(is_empty_program(i->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]])))
				{
					mused.current_fourop_program[mused.selected_operator - 1]++;
					
					if(i->ops[mused.selected_operator - 1].program[i->ops[mused.selected_operator - 1].num_macros] == NULL)
					{
						i->ops[mused.selected_operator - 1].num_macros++;
						
						i->ops[mused.selected_operator - 1].program[i->ops[mused.selected_operator - 1].num_macros - 1] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
						i->ops[mused.selected_operator - 1].program_unite_bits[i->ops[mused.selected_operator - 1].num_macros - 1] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
						
						for (int p = 0; p < MUS_PROG_LEN; ++p)
						{
							i->ops[mused.selected_operator - 1].program[i->ops[mused.selected_operator - 1].num_macros - 1][p] = MUS_FX_NOP;
						}
						
						for (int p = 0; p < MUS_PROG_LEN / 8 + 1; ++p)
						{
							i->ops[mused.selected_operator - 1].program_unite_bits[i->ops[mused.selected_operator - 1].num_macros - 1][p] = 0;
						}
					}
				}
			}
			
			if(a == -1)
			{
				mused.current_fourop_program[mused.selected_operator - 1]--;
			}
		}

		break;

		case FOUROP_FLTTYPE:

		clamp(i->ops[mused.selected_operator - 1].flttype, a, 0, FLT_TYPES - 1);

		break;

		case FOUROP_NORESTART:

		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_NO_PROG_RESTART);

		break;
		
		case FOUROP_SAVE_LFO_SETTINGS:

		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_SAVE_LFO_SETTINGS);

		break;
		
		case FOUROP_TRIG_DELAY:

		clamp(i->ops[mused.selected_operator - 1].trigger_delay, a, 0, 0xFF);

		break;

		case FOUROP_ENV_OFFSET:

		clamp(i->ops[mused.selected_operator - 1].env_offset, a, 0, 0xFF);

		break;
		
		case FOUROP_ENABLE_CSM_TIMER:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_CSM_TIMER);

		break;
		
		case FOUROP_CSM_TIMER_NOTE:
		
		clamp(i->ops[mused.selected_operator - 1].CSM_timer_note, a, 0, FREQ_TAB_SIZE - 1);

		break;
		
		case FOUROP_CSM_TIMER_FINETUNE:
		
		clamp(i->ops[mused.selected_operator - 1].CSM_timer_finetune, a, -128, 127);

		break;
		
		case FOUROP_LINK_CSM_TIMER_NOTE:
		
		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_LINK_CSM_TIMER_NOTE);

		break;

		case FOUROP_ENABLE_PHASE_RESET_TIMER:

		flipbit(i->ops[mused.selected_operator - 1].cydflags, CYD_FM_OP_ENABLE_PHASE_RESET_TIMER);

		break;
		
		case FOUROP_PHASE_RESET_TIMER_NOTE:
		
		clamp(i->ops[mused.selected_operator - 1].phase_reset_timer_note, a, 0, FREQ_TAB_SIZE - 1);

		break;
		
		case FOUROP_PHASE_RESET_TIMER_FINETUNE:
		
		clamp(i->ops[mused.selected_operator - 1].phase_reset_timer_finetune, a, -128, 127);

		break;
		
		case FOUROP_LINK_PHASE_RESET_TIMER_NOTE:
		
		flipbit(i->ops[mused.selected_operator - 1].flags, MUS_FM_OP_LINK_PHASE_RESET_TIMER_NOTE);

		break;

		case FOUROP_FEEDBACK:

		clamp(i->ops[mused.selected_operator - 1].feedback, a, 0, 0xf);

		break;

		default:
		break;
	}
}

static int note_playing[MUS_MAX_CHANNELS] = {-1};

static int find_playing_note(int n)
{
	cyd_lock(&mused.cyd, 1);

	for (int i = 0; i < MUS_MAX_CHANNELS && i < mused.cyd.n_channels; ++i)
	{
		if (note_playing[i] == n && mused.mus.channel[i].instrument == &mused.song.instrument[mused.current_instrument])
		{
			cyd_lock(&mused.cyd, 0);
			return i;
		}
	}

	cyd_lock(&mused.cyd, 0);

	return -1;
}


static void play_note(int note) 
{
	if (find_playing_note(note) == -1 && note < FREQ_TAB_SIZE)
	{
		int c = mus_trigger_instrument(&mused.mus, -1, &mused.song.instrument[mused.current_instrument], note << 8, CYD_PAN_CENTER);
		note_playing[c] = note;

		for(int s = 0; s < CYD_SUB_OSCS; s++)
		{
			mused.cyd.channel[c].subosc[s].accumulator = 0;
			mused.cyd.channel[c].subosc[s].noise_accumulator = 0;
			mused.cyd.channel[c].subosc[s].wave.acc = 0;
		}

		mused.mus.song_track[c].extarp1 = 0;
		mused.mus.song_track[c].extarp2 = 0;

		mus_advance_tick(&mused.mus);
	}
}


static void stop_note(int note)
{
	int c;
	if ((c = find_playing_note(note)) != -1)
	{
		mus_release(&mused.mus, c);
		note_playing[c] = -1;
	}
}


static void play_the_jams(int sym, int chn, int state)
{
	if (sym == SDLK_SPACE && state == 0)
	{
		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
		{
			cyd_enable_gate(mused.mus.cyd, &mused.mus.cyd->channel[i], 0);
			
			int chan = i;
			
			if(mused.mus.channel[chan].instrument != NULL)
			{
				for(int pr = 0; pr < mused.mus.channel[chan].instrument->num_macros; ++pr)
				{
					for(int j = 0; j < MUS_PROG_LEN; ++j)
					{
						if((mused.mus.channel[chan].instrument->program[pr][j] & 0xff00) == MUS_FX_RELEASE_POINT)
						{
							mused.mus.channel[chan].program_tick[pr] = j + 1;
							break;
						}
					}
				}
				
				if(mused.mus.channel[chan].instrument->fm_flags & CYD_FM_ENABLE_4OP)
				{
					for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
					{
						for(int pr = 0; pr < mused.mus.channel[chan].instrument->ops[j].num_macros; ++pr)
						{
							for(int k = 0; k < MUS_PROG_LEN; ++k)
							{
								if((mused.mus.channel[chan].instrument->ops[j].program[pr][k] & 0xff00) == MUS_FX_RELEASE_POINT)
								{
									mused.mus.channel[chan].ops[j].program_tick[pr] = k + 1;
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	
	else
	{
		int note = find_note(sym, mused.octave);
		
		if (note != -1)
		{
			if (mused.flags & MULTIKEY_JAMMING)
			{
				if (state)
					play_note(note);
				else
					stop_note(note);
			}
			
			else
			{
				if (state == 1)
				{
					if (chn == -1 && !(mused.flags & MULTICHANNEL_PREVIEW))
						chn = 0;

					int c = mus_trigger_instrument(&mused.mus, chn, &mused.song.instrument[mused.current_instrument], note << 8, CYD_PAN_CENTER);
					
					for(int s = 0; s < CYD_SUB_OSCS; s++)
					{
						mused.cyd.channel[c].subosc[s].accumulator = 0;
						mused.cyd.channel[c].subosc[s].noise_accumulator = 0;
						mused.cyd.channel[c].subosc[s].wave.acc = 0;
					}

					mus_advance_tick(&mused.mus);
				}
			}
		}
	}
}


static void wave_the_jams(int sym)
{
	if (sym == SDLK_SPACE)
	{
		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
		{
			cyd_enable_gate(mused.mus.cyd, &mused.mus.cyd->channel[i], 0);
		
			int chan = i;
			
			if(mused.mus.channel[chan].instrument != NULL)
			{
				for(int pr = 0; pr < mused.mus.channel[chan].instrument->num_macros; ++pr)
				{
					for(int j = 0; j < MUS_PROG_LEN; ++j)
					{
						if((mused.mus.channel[chan].instrument->program[pr][j] & 0xff00) == MUS_FX_RELEASE_POINT)
						{
							mused.mus.channel[chan].program_tick[pr] = j + 1;
							break;
						}
					}
				}
				
				if(mused.mus.channel[chan].instrument->fm_flags & CYD_FM_ENABLE_4OP)
				{
					for(int j = 0; j < CYD_FM_NUM_OPS; ++j)
					{
						for(int pr = 0; pr < mused.mus.channel[chan].instrument->ops[j].num_macros; ++pr)
						{
							for(int k = 0; k < MUS_PROG_LEN; ++k)
							{
								if((mused.mus.channel[chan].instrument->ops[j].program[pr][k] & 0xff00) == MUS_FX_RELEASE_POINT)
								{
									mused.mus.channel[chan].ops[j].program_tick[pr] = k + 1;
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	
	else
	{
		int note = find_note(sym, mused.octave);
		if (note != -1)
		{
			memset(&mused.inst.local_samples, 0, mused.inst.num_local_samples * sizeof(CydWavetableEntry*));
			
			mus_get_default_instrument(&mused.inst);
			
			if(mused.mode == EDITWAVETABLE)
			{
				mused.inst.wavetable_entry = mused.selected_wavetable;
				mused.inst.flags &= ~(MUS_INST_USE_LOCAL_SAMPLES);
			}
			
			if(mused.mode == EDITLOCALSAMPLE)
			{
				if(mused.show_local_samples_list)
				{
					mused.inst.local_sample = mused.selected_local_sample;
					mused.inst.flags |= MUS_INST_USE_LOCAL_SAMPLES;
					
					mused.inst.local_samples = (CydWavetableEntry**)calloc(1, mused.song.instrument[mused.current_instrument].num_local_samples * sizeof(CydWavetableEntry*));
					memcpy(mused.inst.local_samples, mused.song.instrument[mused.current_instrument].local_samples, mused.song.instrument[mused.current_instrument].num_local_samples * sizeof(CydWavetableEntry*));
					mused.inst.num_local_samples = mused.song.instrument[mused.current_instrument].num_local_samples;
				}
				
				else
				{
					mused.inst.wavetable_entry = mused.selected_local_sample;
					mused.inst.flags &= ~(MUS_INST_USE_LOCAL_SAMPLES);
				}
			}
			
			mused.inst.cydflags &= ~WAVEFORMS;
			mused.inst.cydflags |= CYD_CHN_WAVE_OVERRIDE_ENV | CYD_CHN_ENABLE_WAVE;
			mused.inst.flags &= ~MUS_INST_DRUM;
			mus_trigger_instrument(&mused.mus, 0, &mused.inst, note << 8, CYD_PAN_CENTER);
		}
	}
}

void edit_instrument_event(SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:

		switch (e->key.keysym.sym)
		{
			case SDLK_RETURN:
			{
				if (mused.selected_param == P_NAME)
					set_edit_buffer(mused.song.instrument[mused.current_instrument].name, sizeof(mused.song.instrument[mused.current_instrument].name));
			}
			break;

			case SDLK_DOWN:
			{
				++mused.selected_param;

				if (mused.mode == EDITINSTRUMENT)
				{
					if (mused.selected_param >= P_PARAMS) mused.selected_param = P_PARAMS - 1;
					
					if(mused.show_fm_settings)
					{
						if (mused.selected_param == P_FX) mused.selected_param = P_FM_ENABLE;
					}
					
					else
					{
						if (mused.selected_param == P_FM_ENABLE) mused.selected_param = P_SAVE_LFO_SETTINGS;
					}
				}
				
				else
				{
					if (mused.selected_param >= P_NAME) mused.selected_param = P_NAME;
				}
			}
			break;

			case SDLK_UP:
			{
				--mused.selected_param;

				if (mused.selected_param < 0) mused.selected_param = 0;
				
				if(mused.show_fm_settings && mused.mode == EDITINSTRUMENT)
				{
					if (mused.selected_param == P_SAVE_LFO_SETTINGS) mused.selected_param = P_PROGPERIOD;
				}
			}
			break;

			case SDLK_DELETE:
			{
				char buffer[500];
				
				snprintf(buffer, 499, "Delete selected instrument (%s)?", mused.song.instrument[mused.current_instrument].name);
				
				if (confirm(domain, mused.slider_bevel, &mused.largefont, buffer))
				{
					MusSong* song = &mused.song;
					
					for (int p = 0; p < song->num_patterns; ++p)
					{
						for (int i = 0; i < song->pattern[p].num_steps; ++i)
						{
							if (song->pattern[p].step[i].instrument == mused.current_instrument)
							{
								song->pattern[p].step[i].instrument = MUS_NOTE_NO_INSTRUMENT;
							}
						}
					}
					
					remove_instrument(&mused.song, mused.current_instrument);
				}
			}
			break;

			case SDLK_RIGHT:
			{
				instrument_add_param(+1);
			}
			break;


			case SDLK_LEFT:
			{
				instrument_add_param(-1);
			}
			break;

			default:
			{
				play_the_jams(e->key.keysym.sym, -1, 1);
			}
			break;
		}

		break;

		case SDL_KEYUP:

			play_the_jams(e->key.keysym.sym, -1, 0);

		break;
	}
}

void edit_fourop_event(SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:

		switch (e->key.keysym.sym)
		{
			case SDLK_RETURN:
			{
				if (mused.fourop_selected_param == P_NAME)
					set_edit_buffer(mused.song.instrument[mused.current_instrument].name, sizeof(mused.song.instrument[mused.current_instrument].name));
			}
			break;

			case SDLK_DOWN: //switching between neighbour params through keyboard
			{
				++mused.fourop_selected_param;
				
				if(!(mused.song.instrument[mused.current_instrument].fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) && mused.fourop_selected_param > FOUROP_BYPASS_MAIN_INST_FILTER && mused.fourop_selected_param < FOUROP_HARMONIC_CARRIER)
				{
					mused.fourop_selected_param = FOUROP_HARMONIC_CARRIER;
				}
				
				if((mused.song.instrument[mused.current_instrument].fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) && mused.fourop_selected_param > FOUROP_LOCKNOTE && mused.fourop_selected_param < FOUROP_FEEDBACK)
				{
					mused.fourop_selected_param = FOUROP_FEEDBACK;
				}
				
				if (mused.mode == EDIT4OP)
				{
					if (mused.fourop_selected_param >= FOUROP_PARAMS) mused.fourop_selected_param = FOUROP_PARAMS - 1;
				}
				
				else
				{
					if (mused.fourop_selected_param >= FOUROP_3CH_EXP_MODE) mused.fourop_selected_param = FOUROP_3CH_EXP_MODE;
				}
			}
			break;

			case SDLK_UP:
			{
				--mused.fourop_selected_param;
				
				if(!(mused.song.instrument[mused.current_instrument].fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) && mused.fourop_selected_param > FOUROP_BYPASS_MAIN_INST_FILTER && mused.fourop_selected_param < FOUROP_HARMONIC_CARRIER)
				{
					mused.fourop_selected_param = FOUROP_BYPASS_MAIN_INST_FILTER;
				}
				
				if((mused.song.instrument[mused.current_instrument].fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE) && mused.fourop_selected_param > FOUROP_LOCKNOTE && mused.fourop_selected_param < FOUROP_FEEDBACK)
				{
					mused.fourop_selected_param = FOUROP_LOCKNOTE;
				}

				if (mused.fourop_selected_param < 0) mused.fourop_selected_param = 0;
			}
			break;



			case SDLK_RIGHT:
			{
				four_op_add_param(+1);
			}
			break;
			
			case SDLK_LEFT:
			{
				four_op_add_param(-1);
			}
			break;

			default:
			{
				play_the_jams(e->key.keysym.sym, -1, 1);
			}
			break;
		}

		break;

		case SDL_KEYUP:
		
			play_the_jams(e->key.keysym.sym, -1, 0);

		break;
	}
}


static int gethex(int key)
{
	if (key >= SDLK_0 && key <= SDLK_9)
	{
		return key - SDLK_0;
	}
	else if (key >= SDLK_KP_0 && key <= SDLK_KP_9)
	{
		return key - SDLK_KP_0;
	}
	else if (key >= SDLK_a && key <= SDLK_f)
	{
		return key - SDLK_a + 0xa;
	}
	else return -1;
}


static int getalphanum(const SDL_Keysym *keysym)
{
	int key = keysym->sym;

	if (key >= SDLK_0 && key <= SDLK_9)
	{
		return key - SDLK_0;
	}
	else if (key >= SDLK_KP_0 && key <= SDLK_KP_9)
	{
		return key - SDLK_KP_0;
	}
	else if (!(keysym->mod & KMOD_SHIFT) && key >= SDLK_a && key <= SDLK_z)
	{
		return key - SDLK_a + 0xa;
	}
	else if ((keysym->mod & KMOD_SHIFT) && key >= SDLK_a && key <= SDLK_z)
	{
		return key - SDLK_a + 0xa + (SDLK_z - SDLK_a) + 1;
	}
	else return -1;
}


int seqsort(const void *_a, const void *_b)
{
	const MusSeqPattern *a = _a;
	const MusSeqPattern *b = _b;
	if (a->position > b->position) return 1;
	if (a->position < b->position) return -1;

	return 0;
}


void add_sequence(int channel, int position, int pattern, int offset)
{
	if(mused.song.pattern[pattern].num_steps == 0)
		resize_pattern(&mused.song.pattern[pattern], mused.sequenceview_steps);

	for (int i = 0; i < mused.song.num_sequences[channel]; ++i)
		if (mused.song.sequence[channel][i].position == position)
		{
			mused.song.sequence[channel][i].pattern = pattern;
			mused.song.sequence[channel][i].note_offset = offset;
			return;
		}

	if (mused.song.num_sequences[channel] >= NUM_SEQUENCES)
		return;

	mused.song.sequence[channel][mused.song.num_sequences[channel]].position = position;
	mused.song.sequence[channel][mused.song.num_sequences[channel]].pattern = pattern;
	mused.song.sequence[channel][mused.song.num_sequences[channel]].note_offset = offset;

	++mused.song.num_sequences[channel];

	qsort(mused.song.sequence[channel], mused.song.num_sequences[channel], sizeof(mused.song.sequence[channel][0]), seqsort);
}


Uint16 get_pattern_at(int channel, int position)
{
	for (int i = 0; i < mused.song.num_sequences[channel]; ++i)
		if (mused.song.sequence[channel][i].position == position)
		{
			return mused.song.sequence[channel][i].pattern;
		}

	return 0;
}


void del_sequence(int first,int last,int track)
{
	if (mused.song.num_sequences[track] == 0) return;

	for (int i = 0; i < mused.song.num_sequences[mused.current_sequencetrack]; ++i)
		if (mused.song.sequence[track][i].position >= first && mused.song.sequence[track][i].position < last)
		{
			mused.song.sequence[track][i].position = 0xffff;
		}

	qsort(mused.song.sequence[track], mused.song.num_sequences[track], sizeof(mused.song.sequence[track][0]), seqsort);

	while (mused.song.num_sequences[track] > 0 && mused.song.sequence[track][mused.song.num_sequences[track]-1].position == 0xffff) --mused.song.num_sequences[track];
}


void add_note_offset(int a)
{
	for (int i = (int)mused.song.num_sequences[mused.current_sequencetrack] - 1; i >= 0; --i)
	{
		if (mused.current_sequencepos >= mused.song.sequence[mused.current_sequencetrack][i].position &&
			mused.song.sequence[mused.current_sequencetrack][i].position + mused.song.pattern[mused.song.sequence[mused.current_sequencetrack][i].pattern].num_steps > mused.current_sequencepos)
		{
			mused.song.sequence[mused.current_sequencetrack][i].note_offset += a;
			break;
		}
	}
}


static void update_sequence_slider(int d)
{
	int o = mused.current_sequencepos - mused.current_patternpos;
	
	slider_move_position(&mused.current_sequencepos, &mused.sequence_position, &mused.sequence_slider_param, d);

	if (!(mused.flags & SONG_PLAYING))
	{
		mused.pattern_position = mused.current_patternpos = mused.current_sequencepos - o;
	}
}



static void update_pattern_slider(int d)
{
	/*if (mused.flags & CENTER_PATTERN_EDITOR)
	{
		mused.pattern_position = current_patternstep() + d;

		if (mused.pattern_position < 0) mused.pattern_position += mused.song.pattern[current_pattern()].num_steps;
		if (mused.pattern_position >= mused.song.pattern[current_pattern()].num_steps) mused.pattern_position -= mused.song.pattern[current_pattern()].num_steps;

		current_patternstep() = mused.pattern_position;
	}
	else */
	slider_move_position(&mused.current_patternpos, &mused.pattern_position, &mused.pattern_slider_param, d);
	mused.pattern_position = mused.current_patternpos;

	if (!(mused.flags & SONG_PLAYING))
		mused.current_sequencepos = mused.current_patternpos - mused.current_patternpos % mused.sequenceview_steps;
}


void update_position_sliders()
{
	update_pattern_slider(0);
	update_sequence_slider(0);
}


void update_horiz_sliders()
{
	slider_move_position(&mused.current_sequencetrack, &mused.sequence_horiz_position, &mused.sequence_horiz_slider_param, 0);
	slider_move_position(&mused.current_sequencetrack, &mused.pattern_horiz_position, &mused.pattern_horiz_slider_param, 0);
}


void sequence_event(SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:

		switch (e->key.keysym.sym)
		{
			case SDLK_SPACE:
			{
				if ((mused.flags & TOGGLE_EDIT_ON_STOP) || !(mused.flags & SONG_PLAYING))
						mused.flags ^= EDIT_MODE;

				if (mused.flags & SONG_PLAYING) stop(0, 0, 0);
			}
			break;

			case SDLK_RETURN:
			{
				if (mused.mode != EDITCLASSIC) change_mode(EDITPATTERN);
				else mused.focus = EDITPATTERN;

				//mused.current_patternpos = mused.current_sequencepos;

				update_position_sliders();
			}
			break;

			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
				begin_selection(mused.current_sequencepos);
			break;

			case SDLK_PAGEDOWN:
			case SDLK_DOWN:
			{
				if (e->key.keysym.mod & KMOD_ALT)
				{
					snapshot(S_T_SEQUENCE);
					add_note_offset(-1);
					break;
				}
				
				if (e->key.keysym.mod & KMOD_CTRL)
				{
					snapshot(S_T_SEQUENCE);
					
					for (int i = (int)mused.song.num_sequences[mused.current_sequencetrack] - 1; i >= 0; --i)
					{
						if (mused.current_sequencepos >= mused.song.sequence[mused.current_sequencetrack][i].position - mused.sequenceview_steps &&
							mused.song.sequence[mused.current_sequencetrack][i].position + mused.song.pattern[mused.song.sequence[mused.current_sequencetrack][i].pattern].num_steps > mused.current_sequencepos + mused.sequenceview_steps && mused.song.sequence[mused.current_sequencetrack][i].position > 0)
						{
							mused.song.sequence[mused.current_sequencetrack][i].position -= 1;
							break;
						}
					}
					
					break;
				}

				int steps = mused.sequenceview_steps;
				if (e->key.keysym.sym == SDLK_PAGEDOWN)
					steps *= 16;
				if (e->key.keysym.mod & KMOD_CTRL)
				{
					snapshot_cascade(S_T_SONGINFO, SI_LOOP, -1);
					if (e->key.keysym.mod & KMOD_SHIFT)
					{
						change_loop_point(MAKEPTR(steps), 0, 0);
					}
					
					else
					{
						change_song_length(MAKEPTR(steps), 0, 0);
					}
				}
				
				else
				{
					update_sequence_slider(steps);
				}

				if (((e->key.keysym.mod & KMOD_SHIFT) && !(e->key.keysym.mod & KMOD_CTRL)) )
				{
					select_range(mused.current_sequencepos);
				}
			}
			break;

			case SDLK_PAGEUP:
			case SDLK_UP:
			{
				if (e->key.keysym.mod & KMOD_ALT)
				{
					snapshot(S_T_SEQUENCE);
					add_note_offset(1);
					break;
				}
				
				if (e->key.keysym.mod & KMOD_CTRL)
				{
					snapshot(S_T_SEQUENCE);
					
					//mused.sequenceview_steps
					for (int i = (int)mused.song.num_sequences[mused.current_sequencetrack] - 1; i >= 0; --i)
					{
						if (mused.current_sequencepos >= mused.song.sequence[mused.current_sequencetrack][i].position - mused.sequenceview_steps &&
							mused.song.sequence[mused.current_sequencetrack][i].position + mused.song.pattern[mused.song.sequence[mused.current_sequencetrack][i].pattern].num_steps > mused.current_sequencepos + mused.sequenceview_steps && mused.song.sequence[mused.current_sequencetrack][i].position + mused.song.pattern[mused.song.sequence[mused.current_sequencetrack][i].pattern].num_steps < 0xffff)
						{
							mused.song.sequence[mused.current_sequencetrack][i].position += 1;
							break;
						}
					}
					
					break;
				}

				int steps = mused.sequenceview_steps;
				if (e->key.keysym.sym == SDLK_PAGEUP)
					steps *= 16;

				if (e->key.keysym.mod & KMOD_CTRL)
				{
					snapshot_cascade(S_T_SONGINFO, SI_LOOP, -1);
					if (e->key.keysym.mod & KMOD_SHIFT)
					{
						change_loop_point(MAKEPTR(-steps), 0,0);
					}
					else
					{
						change_song_length(MAKEPTR(-steps), 0, 0);
					}
				}
				
				else
				{
					update_sequence_slider(-steps);
				}

				if (((e->key.keysym.mod & KMOD_SHIFT) && !(e->key.keysym.mod & KMOD_CTRL)) )
				{
					select_range(mused.current_sequencepos);
				}
			}
			break;

			case SDLK_INSERT:
			{
				snapshot(S_T_SEQUENCE);

				for (int i = 0; i < mused.song.num_sequences[mused.current_sequencetrack]; ++i)
				{
					if (mused.song.sequence[mused.current_sequencetrack][i].position >= mused.current_sequencepos)
						mused.song.sequence[mused.current_sequencetrack][i].position += mused.sequenceview_steps;
				}
			}
			break;

			case SDLK_PERIOD:
			{
				snapshot(S_T_SEQUENCE);

				del_sequence(mused.current_sequencepos, mused.current_sequencepos+mused.sequenceview_steps, mused.current_sequencetrack);
				if (mused.song.song_length > mused.current_sequencepos + mused.sequenceview_steps)
					mused.current_sequencepos += mused.sequenceview_steps;
			}
			break;

			case SDLK_DELETE:
			{
				snapshot(S_T_SEQUENCE);

				del_sequence(mused.current_sequencepos, mused.current_sequencepos+mused.sequenceview_steps, mused.current_sequencetrack);

				if (!(mused.flags & DELETE_EMPTIES))
				{
					for (int i = 0; i < mused.song.num_sequences[mused.current_sequencetrack]; ++i)
					{
						if (mused.song.sequence[mused.current_sequencetrack][i].position >= mused.current_sequencepos)
							mused.song.sequence[mused.current_sequencetrack][i].position -= mused.sequenceview_steps;
					}
				}
			}
			break;

			case SDLK_RIGHT:
			{
				if (e->key.keysym.mod & KMOD_CTRL)
				{
					change_seq_steps((void *)1, 0, 0);
				}
				
				else
				{
					if (mused.flags & EDIT_SEQUENCE_DIGITS)
					{
						Uint16 p = get_pattern_at(mused.current_sequencetrack, mused.current_sequencepos);
						
						if(p < 0x100)
						{
							if (mused.sequence_digit < 1)
							{
								++mused.sequence_digit;
								break;
							}
						}
						
						else
						{
							if (mused.sequence_digit < 2)
							{
								++mused.sequence_digit;
								break;
							}
						}
					}

					mused.sequence_digit = 0;
					++mused.current_sequencetrack;
					if (mused.current_sequencetrack >= mused.song.num_channels)
						mused.current_sequencetrack = 0;

					update_horiz_sliders();
				}
			}
			break;

			case SDLK_LEFT:
			{
				if (e->key.keysym.mod & KMOD_CTRL)
				{
					change_seq_steps((void *)-1, 0, 0);
				}
				
				else
				{
					if (mused.flags & EDIT_SEQUENCE_DIGITS)
					{
						if (mused.sequence_digit > 0)
						{
							--mused.sequence_digit;
							break;
						}
					}

					mused.sequence_digit = 0;
					--mused.current_sequencetrack;
					if (mused.current_sequencetrack < 0)
						mused.current_sequencetrack = mused.song.num_channels - 1;

					update_horiz_sliders();
				}
			}
			break;

			default:
			{
				if (mused.flags & EDIT_SEQUENCE_DIGITS)
				{
					int k;

					if ((k = gethex(e->key.keysym.sym)) != -1)
					{
						snapshot(S_T_SEQUENCE);

						Uint16 p = get_pattern_at(mused.current_sequencetrack, mused.current_sequencepos);
						
						if(p < 0x100)
						{
							if (mused.sequence_digit == 0)
								p = (p & (0x0f)) | (k << 4);
							else
								p = (p & (0xf0)) | k;
						}
						
						else
						{
							switch(mused.sequence_digit)
							{
								case 0:
								{
									p = (p & (0x0ff)) | (k << 8);
									break;
								}
								
								case 1:
								{
									p = (p & (0xf0f)) | (k << 4);
									break;
								}
								
								case 2:
								{
									p = (p & (0xff0)) | k;
									break;
								}
							}
						}

						add_sequence(mused.current_sequencetrack, mused.current_sequencepos, p, 0);

						if (mused.song.song_length > mused.current_sequencepos + mused.sequenceview_steps)
						{
							mused.current_sequencepos += mused.sequenceview_steps;
							mused.current_patternpos += mused.sequenceview_steps;
							update_position_sliders();
						}
					}
				}
				else
				{
					int p = getalphanum(&e->key.keysym);
					if (p != -1 && p < NUM_PATTERNS)
					{
						snapshot(S_T_SEQUENCE);
						add_sequence(mused.current_sequencetrack, mused.current_sequencepos, p, 0);
						if (mused.song.song_length > mused.current_sequencepos + mused.sequenceview_steps)
						{
							mused.current_sequencepos += mused.sequenceview_steps;
							mused.current_patternpos += mused.sequenceview_steps;
							update_position_sliders();
						}
					}
				}
			}
			break;
		}

		break;
	}

	//update_ghost_patterns();
}


static void switch_track(int d)
{
	mused.current_sequencetrack = (mused.current_sequencetrack + (int)mused.song.num_channels + d) % mused.song.num_channels;
	update_horiz_sliders();
	mused.sequence_digit = 0;
}


static void write_note(int note)
{
	snapshot(S_T_PATTERN);

	mused.song.pattern[current_pattern()].step[current_patternstep()].note = note;
	mused.song.pattern[current_pattern()].step[current_patternstep()].instrument = mused.current_instrument;

	update_pattern_slider(mused.note_jump);
}


void pattern_event(SDL_Event *e)
{
	int last_param = PED_PARAMS - 1;
	if (!viscol(PED_COMMAND1)) last_param = my_max(PED_COMMAND1 - 1, last_param);
	else if (!viscol(PED_CTRL)) last_param = my_max(PED_CTRL - 1, last_param);
	else if (!viscol(PED_VOLUME1)) last_param = my_max(PED_VOLUME1 - 1, last_param);
	else if (!viscol(PED_INSTRUMENT1)) last_param = my_max(PED_INSTRUMENT1 - 1, last_param);

	switch (e->type)
	{
		case SDL_KEYDOWN:

		switch (e->key.keysym.sym)
		{
			case SDLK_RETURN:
			{
				if (e->key.keysym.mod & KMOD_CTRL)
				{
					Uint8 p = get_pattern_at(mused.current_sequencetrack, mused.current_sequencepos);
					add_sequence(mused.current_sequencetrack, mused.current_patternpos, p, 0);
				}
				else
				{
					if (mused.mode != EDITCLASSIC) change_mode(EDITSEQUENCE);
					else mused.focus = EDITSEQUENCE;

					mused.flags &= ~SHOW_LOGO; // hide logo when jumping to sequence

					mused.current_sequencepos = mused.current_patternpos;

					update_position_sliders();
				}
			}
			break;

			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
				begin_selection(mused.current_patternpos);
			break;

			case SDLK_END:
			case SDLK_PAGEDOWN:
			case SDLK_DOWN:
			{
				if (e->key.keysym.mod & KMOD_CTRL)
				{
					update_sequence_slider(mused.sequenceview_steps);
				}
				
				else
				{
					int steps = 1;
					if (e->key.keysym.sym == SDLK_PAGEDOWN) steps = steps * 16;

					if (get_current_pattern())
					{
						if (e->key.keysym.sym == SDLK_END) steps = get_current_pattern()->num_steps - current_patternstep() - 1;

						if ((e->key.keysym.mod & KMOD_SHIFT) && get_current_pattern())
							steps = my_min(steps, get_current_pattern()->num_steps - current_patternstep() - 1);
					}

					update_pattern_slider(steps);

					if (e->key.keysym.mod & KMOD_SHIFT)
					{
						select_range(mused.current_patternpos);
					}
				}
			}
			break;

			case SDLK_HOME:
			case SDLK_PAGEUP:
			case SDLK_UP:
			{
				if (e->key.keysym.mod & KMOD_CTRL)
				{
					update_sequence_slider(-mused.sequenceview_steps);
				}
				else
				{

					int steps = 1;
					if (e->key.keysym.sym == SDLK_PAGEUP) steps = steps * 16;
					if (e->key.keysym.sym == SDLK_HOME) steps = current_patternstep();

					if (e->key.keysym.mod & KMOD_SHIFT)
						steps = my_min(steps, current_patternstep());

					update_pattern_slider(-steps);

					if (e->key.keysym.mod & KMOD_SHIFT)
					{
						select_range(mused.current_patternpos);
					}

				}
			}
			break;

			case SDLK_INSERT:
			{
				if (!(mused.flags & EDIT_MODE)) break;

				if (get_current_step())
				{
					snapshot(S_T_PATTERN);

					if ((e->key.keysym.mod & KMOD_ALT))
					{
						resize_pattern(get_current_pattern(), get_current_pattern()->num_steps + 1);
						zero_step(&mused.song.pattern[current_pattern()].step[mused.song.pattern[current_pattern()].num_steps - 1]);
						break;
					}

					for (int i = mused.song.pattern[current_pattern()].num_steps-1; i >= current_patternstep(); --i)
						memcpy(&mused.song.pattern[current_pattern()].step[i], &mused.song.pattern[current_pattern()].step[i-1], sizeof(mused.song.pattern[current_pattern()].step[0]));

					zero_step(&mused.song.pattern[current_pattern()].step[current_patternstep()]);
				}
			}
			break;

			case SDLK_BACKSPACE:
			case SDLK_DELETE:
			{
				if (!(mused.flags & EDIT_MODE)) break;

				if (get_current_step())
				{
					snapshot(S_T_PATTERN);
					
					if(mused.selection.start == mused.selection.end)
					{
						if (e->key.keysym.sym == SDLK_BACKSPACE)
						{
							if (current_patternstep() > 0) update_pattern_slider(-1);
							else break;
						}

						if ((e->key.keysym.mod & KMOD_ALT))
						{
							if (mused.song.pattern[current_pattern()].num_steps > 1)
								resize_pattern(&mused.song.pattern[current_pattern()], mused.song.pattern[current_pattern()].num_steps - 1);

							if (current_patternstep() >= mused.song.pattern[current_pattern()].num_steps) --mused.current_patternpos;
							break;
						}

						if (!(mused.flags & DELETE_EMPTIES) || e->key.keysym.sym == SDLK_BACKSPACE)
						{
							for (int i = current_patternstep() ; i < mused.song.pattern[current_pattern()].num_steps; ++i)
								memcpy(&mused.song.pattern[current_pattern()].step[i], &mused.song.pattern[current_pattern()].step[i+1], sizeof(mused.song.pattern[current_pattern()].step[0]));

							zero_step(&mused.song.pattern[current_pattern()].step[mused.song.pattern[current_pattern()].num_steps - 1]);

							if (current_patternstep() >= mused.song.pattern[current_pattern()].num_steps) --mused.current_patternpos;
						}
						
						else
						{
							if (e->key.keysym.mod & KMOD_SHIFT)
							{
								zero_step(&mused.song.pattern[current_pattern()].step[current_patternstep()]);
							}
							
							else
							{
								switch (mused.current_patternx)
								{
									case PED_NOTE:
										mused.song.pattern[current_pattern()].step[current_patternstep()].note = MUS_NOTE_NONE;
										mused.song.pattern[current_pattern()].step[current_patternstep()].instrument = MUS_NOTE_NO_INSTRUMENT;
										break;

									case PED_INSTRUMENT1:
									case PED_INSTRUMENT2:
										mused.song.pattern[current_pattern()].step[current_patternstep()].instrument = MUS_NOTE_NO_INSTRUMENT;
										break;

									case PED_VOLUME1:
									case PED_VOLUME2:
										mused.song.pattern[current_pattern()].step[current_patternstep()].volume = MUS_NOTE_NO_VOLUME;
										break;

									case PED_LEGATO:
									case PED_SLIDE:
									case PED_VIB:
									case PED_TREM:
										mused.song.pattern[current_pattern()].step[current_patternstep()].ctrl = 0;
										break;

									case PED_COMMAND1:
									case PED_COMMAND2:
									case PED_COMMAND3:
									case PED_COMMAND4:
										mused.song.pattern[current_pattern()].step[current_patternstep()].command[0] = 0;
										break;
										
									case PED_COMMAND21:
									case PED_COMMAND22:
									case PED_COMMAND23:
									case PED_COMMAND24:
										mused.song.pattern[current_pattern()].step[current_patternstep()].command[1] = 0;
										break;
										
									case PED_COMMAND31:
									case PED_COMMAND32:
									case PED_COMMAND33:
									case PED_COMMAND34:
										mused.song.pattern[current_pattern()].step[current_patternstep()].command[2] = 0;
										break;
										
									case PED_COMMAND41:
									case PED_COMMAND42:
									case PED_COMMAND43:
									case PED_COMMAND44:
										mused.song.pattern[current_pattern()].step[current_patternstep()].command[3] = 0;
										break;
										
									case PED_COMMAND51:
									case PED_COMMAND52:
									case PED_COMMAND53:
									case PED_COMMAND54:
										mused.song.pattern[current_pattern()].step[current_patternstep()].command[4] = 0;
										break;
										
									case PED_COMMAND61:
									case PED_COMMAND62:
									case PED_COMMAND63:
									case PED_COMMAND64:
										mused.song.pattern[current_pattern()].step[current_patternstep()].command[5] = 0;
										break;
										
									case PED_COMMAND71:
									case PED_COMMAND72:
									case PED_COMMAND73:
									case PED_COMMAND74:
										mused.song.pattern[current_pattern()].step[current_patternstep()].command[6] = 0;
										break;
										
									case PED_COMMAND81:
									case PED_COMMAND82:
									case PED_COMMAND83:
									case PED_COMMAND84:
										mused.song.pattern[current_pattern()].step[current_patternstep()].command[7] = 0;
										break;
								}
							}

							update_pattern_slider(mused.note_jump);
						}
					}
					
					else
					{
						if(e->key.keysym.sym == SDLK_BACKSPACE) break;
						
						MusStep* s = &mused.song.pattern[get_pattern(mused.selection.start, mused.current_sequencetrack)].step[get_patternstep(mused.selection.start, mused.current_sequencetrack)];
						
						for(int i = 0; i < abs(mused.selection.start - mused.selection.end); ++i)
						{
							if(mused.selection.patternx_start == 0 && mused.selection.patternx_end >= 0)
							{
								s[i].note = MUS_NOTE_NONE;
							}
							
							if((mused.selection.patternx_start <= 1 && mused.selection.patternx_end >= 1) || (mused.selection.patternx_start <= 2 && mused.selection.patternx_end >= 2))
							{
								s[i].instrument = MUS_NOTE_NO_INSTRUMENT;
							}
							
							if((mused.selection.patternx_start <= 3 && mused.selection.patternx_end >= 3) || (mused.selection.patternx_start <= 4 && mused.selection.patternx_end >= 4))
							{
								s[i].volume = MUS_NOTE_NO_VOLUME;
							}
							
							if(mused.selection.patternx_start <= 5 && mused.selection.patternx_end >= 5)
							{
								s[i].ctrl &= ~(0b0001);
							}
							
							if(mused.selection.patternx_start <= 6 && mused.selection.patternx_end >= 6)
							{
								s[i].ctrl &= ~(0b0010);
							}
							
							if(mused.selection.patternx_start <= 7 && mused.selection.patternx_end >= 7)
							{
								s[i].ctrl &= ~(0b0100);
							}
							
							if(mused.selection.patternx_start <= 8 && mused.selection.patternx_end >= 8)
							{
								s[i].ctrl &= ~(0b1000);
							}
							
							for(int j = 0; j < MUS_MAX_COMMANDS; ++j)
							{
								if(mused.selection.patternx_start <= 9 + j * 4 && mused.selection.patternx_end >= 9 + j * 4)
								{
									s[i].command[j] &= ~(0xf000);
								}
								
								if(mused.selection.patternx_start <= 10 + j * 4 && mused.selection.patternx_end >= 10 + j * 4)
								{
									s[i].command[j] &= ~(0x0f00);
								}
								
								if(mused.selection.patternx_start <= 11 + j * 4 && mused.selection.patternx_end >= 11 + j * 4)
								{
									s[i].command[j] &= ~(0x00f0);
								}
								
								if(mused.selection.patternx_start <= 12 + j * 4 && mused.selection.patternx_end >= 12 + j * 4)
								{
									s[i].command[j] &= ~(0x000f);
								}
							}
						}
					}
				}
			}
			break;

			case SDLK_RIGHT:
			{
				if (e->key.keysym.mod & KMOD_CTRL)
				{
					int x = mused.current_patternx;

					switch_track(+1);

					if (e->key.keysym.mod & KMOD_SHIFT)
						mused.current_patternx = my_max(x, x);
				}
				
				else
				{
					++mused.current_patternx;

					if (!viscol(mused.current_patternx) && (mused.current_patternx == PED_INSTRUMENT1 || mused.current_patternx == PED_INSTRUMENT2))
						mused.current_patternx = PED_VOLUME1;

					if (!viscol(mused.current_patternx) && (mused.current_patternx == PED_VOLUME1 || mused.current_patternx == PED_VOLUME2))
						mused.current_patternx = PED_CTRL;

					if (!viscol(mused.current_patternx) && (mused.current_patternx >= PED_CTRL && mused.current_patternx < PED_COMMAND1))
						mused.current_patternx = PED_COMMAND1;

					if (!viscol(mused.current_patternx) && (mused.current_patternx >= PED_COMMAND1 && mused.current_patternx <= PED_COMMAND4))
						mused.current_patternx = PED_PARAMS;
					
					if(current_pattern() != -1)
					{
						if (mused.current_patternx > PED_COMMAND4 + mused.command_columns[mused.current_sequencetrack] * 4) //if (mused.current_patternx >= PED_PARAMS)
						{
							mused.current_patternx = 0;
							switch_track(+1);
						}
					}

					else //don't have any pattern
					{
						if (mused.current_patternx > PED_COMMAND4)
						{
							mused.current_patternx = 0;
							switch_track(+1);
						}
					}
					
					//mused.song.pattern[current_pattern()].step[current_patternstep()].command[(mused.current_patternx - PED_COMMAND1) / 4]
					//mused.song.pattern[mused.song.sequence[channel][0]->pattern]
				}
			}
			break;

			case SDLK_LEFT:
			{
				if (e->key.keysym.mod & KMOD_CTRL)
				{
					int x = mused.current_patternx;

					switch_track(-1);

					if (e->key.keysym.mod & KMOD_SHIFT)
						mused.current_patternx = x;
				}
				
				else
				{
					--mused.current_patternx;

					if (mused.current_patternx < 0)
					{
						//mused.current_patternx = PED_COMMAND4;
						
						if(mused.current_sequencetrack > 0)
						{
							//mused.current_patternx = PED_COMMAND4 + mused.song.pattern[mused.song.sequence[mused.current_sequencetrack - 1][mused.sequence_position].pattern].command_columns * 4;
							//debug("current pattern %d, current sequence track %d, current seq pos %d", mused.song.sequence[mused.current_sequencetrack - 1][mused.current_sequencepos].pattern, mused.current_sequencetrack, mused.current_sequencepos);
							
							for(int i = 0; i < NUM_SEQUENCES; ++i)
							{
								const MusSeqPattern *sp = &mused.song.sequence[mused.current_sequencetrack - 1][i];
								
								if(sp->position + mused.song.pattern[sp->pattern].num_steps >= mused.current_patternpos && sp->position <= mused.current_patternpos)
								{
									mused.current_patternx = PED_COMMAND4 + mused.command_columns[mused.current_sequencetrack - 1] * 4;
									
									//debug("track %d pattern %d pos %d", mused.current_sequencetrack, sp->pattern, mused.current_patternx);
									
									break;
								}
							}
						}
							
						else
						{
							//mused.current_patternx = PED_COMMAND4 + mused.song.pattern[mused.song.sequence[mused.song.num_channels - 1][mused.sequence_position].pattern].command_columns * 4;
							//debug("current pattern %d, current sequence track %d, current seq pos %d", mused.song.sequence[mused.song.num_channels - 1][mused.current_sequencepos].pattern, mused.current_sequencetrack, mused.current_sequencepos);
							
							for(int i = 0; i < NUM_SEQUENCES; ++i)
							{
								const MusSeqPattern *sp = &mused.song.sequence[mused.song.num_channels - 1][i];
								
								if(sp->position + mused.song.pattern[sp->pattern].num_steps >= mused.current_patternpos && sp->position <= mused.current_patternpos)
								{
									mused.current_patternx = PED_COMMAND4 + mused.command_columns[mused.song.num_channels - 1] * 4;
									
									//debug("track %d pattern %d pos %d", mused.current_sequencetrack, sp->pattern, mused.current_patternx);
									
									break;
								}
							}
						}
						
						switch_track(-1);
					}

					if (!viscol(mused.current_patternx) && (mused.current_patternx >= PED_COMMAND1 && mused.current_patternx <= PED_COMMAND4))
						mused.current_patternx = PED_VIB;

					if (!viscol(mused.current_patternx) && (mused.current_patternx < PED_COMMAND1 && mused.current_patternx > PED_VOLUME2))
						mused.current_patternx = PED_VOLUME2;

					if (!viscol(mused.current_patternx) && (mused.current_patternx == PED_VOLUME1 || mused.current_patternx == PED_VOLUME2))
						mused.current_patternx = PED_INSTRUMENT2;

					if (!viscol(mused.current_patternx) && (mused.current_patternx == PED_INSTRUMENT1 || mused.current_patternx == PED_INSTRUMENT2))
						mused.current_patternx = PED_NOTE;

					if (mused.current_patternx < 0)
					{
						if (mused.single_pattern_edit)
						{
							mused.current_patternx = 0;
						}
						
						else
						{
							if(mused.current_sequencetrack > 0)
							{
								mused.current_patternx = PED_COMMAND4 + mused.command_columns[mused.current_sequencetrack - 1] * 4;
								//debug("current pattern %d, current sequence track %d, current seq pos %d", mused.song.sequence[mused.current_sequencetrack - 1][mused.current_sequencepos].pattern, mused.current_sequencetrack, mused.current_sequencepos);
								
								for(int i = 0; i < NUM_SEQUENCES; ++i)
								{
									const MusSeqPattern *sp = &mused.song.sequence[mused.current_sequencetrack - 1][i];
									
									if(sp->position + mused.song.pattern[sp->pattern].num_steps - 1 >= mused.current_patternpos && sp->position <= mused.current_patternpos)
									{
										{
											mused.current_patternx = PED_COMMAND4 + mused.command_columns[mused.current_sequencetrack - 1] * 4;
											
											//debug("track %d pattern %d pos %d", mused.current_sequencetrack, sp->pattern, mused.current_patternx);
											
											break;
										}
										/*if((sp + 1)->position + mused.song.pattern[(sp + 1)->pattern].num_steps >= mused.current_sequencepos && (sp + 1)->position <= mused.current_sequencepos)
										{
											mused.current_patternx = PED_COMMAND4 + mused.song.pattern[(sp + 1)->pattern].command_columns * 4;
										
											debug("track %d pattern %d pos %d", mused.current_sequencetrack, sp->pattern, mused.current_patternx);
											
											break;
										}
										
										else if((sp)->position + mused.song.pattern[(sp)->pattern].num_steps >= mused.current_sequencepos && (sp)->position <= mused.current_sequencepos)
										{
											mused.current_patternx = PED_COMMAND4 + mused.song.pattern[sp->pattern].command_columns * 4;
											
											debug("track %d pattern %d pos %d", mused.current_sequencetrack, sp->pattern, mused.current_patternx);
											
											break;
										}*/
									}
								}
							}
							
							else
							{
								//mused.current_patternx = PED_COMMAND4 + mused.song.pattern[mused.song.sequence[mused.song.num_channels - 1][mused.sequence_position].pattern].command_columns * 4;
								//debug("current pattern %d, current sequence track %d, current seq pos %d", mused.song.sequence[mused.song.num_channels - 1][mused.current_sequencepos].pattern, mused.current_sequencetrack, mused.current_sequencepos);
								
								for(int i = 0; i < NUM_SEQUENCES; ++i)
								{
									const MusSeqPattern *sp = &mused.song.sequence[mused.song.num_channels - 1][i];
									
									if(sp->position + mused.song.pattern[sp->pattern].num_steps - 1 >= mused.current_patternpos && sp->position <= mused.current_patternpos)
									{
										mused.current_patternx = PED_COMMAND4 + mused.command_columns[mused.song.num_channels - 1] * 4;
										
										//debug("track %d pattern %d pos %d", mused.current_sequencetrack, sp->pattern, mused.current_patternx);
										
										break;
									}
								}
							}
							
							//mused.current_patternx = PED_COMMAND4;
							switch_track(-1);
						}
					}
				}
			}
			break;

			default:
			{
				if (e->key.keysym.sym == SDLK_SPACE)
				{
					if ((mused.flags & TOGGLE_EDIT_ON_STOP) || !(mused.flags & SONG_PLAYING))
						mused.flags ^= EDIT_MODE;

					if (mused.flags & SONG_PLAYING) stop(0, 0, 0);

					play_the_jams(e->key.keysym.sym, -1, 1);
				}

				if (!(mused.flags & EDIT_MODE))
				{
					/* still has to play a note */

					if (mused.current_patternx == PED_NOTE)
					{
						play_the_jams(e->key.keysym.sym, mused.current_sequencetrack, 1);
					}

					break;
				}

				if (get_current_step())
				{
					if (mused.current_patternx == PED_NOTE)
					{
						if (e->key.keysym.sym == SDLK_PERIOD)
						{
							snapshot(S_T_PATTERN);
							
							mused.song.pattern[current_pattern()].step[current_patternstep()].note = MUS_NOTE_NONE;

							update_pattern_slider(mused.note_jump);
						}
						
						else if (e->key.keysym.sym == SDLK_1 || e->key.keysym.sym == SDLK_BACKQUOTE || e->key.keysym.sym == SDLK_EQUALS || e->key.keysym.sym == SDLK_MINUS)
						{
							snapshot(S_T_PATTERN);
							
							if(e->key.keysym.sym == SDLK_BACKQUOTE)
							{
								mused.song.pattern[current_pattern()].step[current_patternstep()].note = MUS_NOTE_CUT;
							}
							
							if(e->key.keysym.sym == SDLK_1)
							{
								mused.song.pattern[current_pattern()].step[current_patternstep()].note = MUS_NOTE_RELEASE;
							}
							
							if(e->key.keysym.sym == SDLK_EQUALS)
							{
								mused.song.pattern[current_pattern()].step[current_patternstep()].note = MUS_NOTE_MACRO_RELEASE;
							}
							
							if(e->key.keysym.sym == SDLK_MINUS)
							{
								mused.song.pattern[current_pattern()].step[current_patternstep()].note = MUS_NOTE_RELEASE_WITHOUT_MACRO;
							}

							update_pattern_slider(mused.note_jump);
						}
						
						else
						{
							int note = find_note(e->key.keysym.sym, mused.octave);
							if (note != -1)
							{
								play_the_jams(e->key.keysym.sym, mused.current_sequencetrack, 1);
								write_note(note);
							}
						}
					}
					
					else if (mused.current_patternx == PED_INSTRUMENT1 || mused.current_patternx == PED_INSTRUMENT2)
					{
						if (e->key.keysym.sym == SDLK_PERIOD)
						{
							snapshot(S_T_PATTERN);
							
							mused.song.pattern[current_pattern()].step[current_patternstep()].instrument = MUS_NOTE_NO_INSTRUMENT;

							update_pattern_slider(mused.note_jump);
						}
						else if (gethex(e->key.keysym.sym) != -1)
						{
							if (mused.song.num_instruments > 0)
							{
								Uint8 inst = mused.song.pattern[current_pattern()].step[current_patternstep()].instrument;
								if (inst == MUS_NOTE_NO_INSTRUMENT) inst = 0;

								switch (mused.current_patternx)
								{
									case PED_INSTRUMENT1:
									inst = (inst & 0x0f) | ((gethex(e->key.keysym.sym) << 4) & 0xf0);
									break;

									case PED_INSTRUMENT2:
									inst = (inst & 0xf0) | gethex(e->key.keysym.sym);
									break;
								}

								if (inst > (mused.song.num_instruments-1)) inst = (mused.song.num_instruments-1);

								snapshot(S_T_PATTERN);

								mused.song.pattern[current_pattern()].step[current_patternstep()].instrument = inst;
								mused.current_instrument = inst % mused.song.num_instruments;
							}

							update_pattern_slider(mused.note_jump);
						}
					}
					
					else if (mused.current_patternx == PED_VOLUME1 || mused.current_patternx == PED_VOLUME2)
					{
						switch (e->key.keysym.sym)
						{
							case  SDLK_PERIOD:
								snapshot(S_T_PATTERN);

								mused.song.pattern[current_pattern()].step[current_patternstep()].volume = MUS_NOTE_NO_VOLUME;

								update_pattern_slider(mused.note_jump);
								break;

							default:
								if (mused.current_patternx == PED_VOLUME1)
								{
									int cmd = 0;

									switch (e->key.keysym.sym)
									{
										case SDLK_u: cmd = MUS_NOTE_VOLUME_FADE_UP; break;
										case SDLK_d: cmd = MUS_NOTE_VOLUME_FADE_DN; break;
										case SDLK_p: cmd = MUS_NOTE_VOLUME_SET_PAN; break;
										case SDLK_l: cmd = MUS_NOTE_VOLUME_PAN_LEFT; break;
										case SDLK_r: cmd = MUS_NOTE_VOLUME_PAN_RIGHT; break;
										
										case SDLK_w: cmd = MUS_NOTE_VOLUME_FADE_UP_FINE; break; 
										case SDLK_s: cmd = MUS_NOTE_VOLUME_FADE_DN_FINE; break;
										default: break;
									}

									//snapshot(S_T_PATTERN);

									if (cmd != 0)
									{
										if (mused.song.pattern[current_pattern()].step[current_patternstep()].volume == MUS_NOTE_NO_VOLUME)
											mused.song.pattern[current_pattern()].step[current_patternstep()].volume = 0;

										if (mused.current_patternx == PED_VOLUME1)
										{
											mused.song.pattern[current_pattern()].step[current_patternstep()].volume =
												(mused.song.pattern[current_pattern()].step[current_patternstep()].volume & 0xf)
												| cmd;
											
											if(e->key.keysym.sym == SDLK_w && (mused.song.pattern[current_pattern()].step[current_patternstep()].volume & 0xf) == 0)
											{
												mused.song.pattern[current_pattern()].step[current_patternstep()].volume += 1; //without "+1" it would be 0x80 which is volume value lol
											}
										}

										update_pattern_slider(mused.note_jump);

										break;
									}
								}

								if (gethex(e->key.keysym.sym) != -1)
								{
									Uint8 vol = mused.song.pattern[current_pattern()].step[current_patternstep()].volume;
									if ((vol == MUS_NOTE_NO_VOLUME)) vol = 0;

									switch (mused.current_patternx)
									{
										case PED_VOLUME1:
										vol = (vol & 0x0f) | ((gethex(e->key.keysym.sym) << 4) & 0xf0);
										break;

										case PED_VOLUME2:
										vol = (vol & 0xf0) | gethex(e->key.keysym.sym);
										break;
									}
									
									snapshot(S_T_PATTERN);

									if ((vol & 0xf0) != MUS_NOTE_VOLUME_FADE_UP &&
										(vol & 0xf0) != MUS_NOTE_VOLUME_FADE_DN &&
										(vol & 0xf0) != MUS_NOTE_VOLUME_PAN_LEFT &&
										(vol & 0xf0) != MUS_NOTE_VOLUME_PAN_RIGHT &&
										(vol & 0xf0) != MUS_NOTE_VOLUME_SET_PAN &&
										((vol & 0xf0) != MUS_NOTE_VOLUME_FADE_UP_FINE && (vol & 0xf) != 0) &&
										(vol & 0xf0) != MUS_NOTE_VOLUME_FADE_DN_FINE)
										mused.song.pattern[current_pattern()].step[current_patternstep()].volume = my_min(MAX_VOLUME, vol);
									else mused.song.pattern[current_pattern()].step[current_patternstep()].volume = vol;

									update_pattern_slider(mused.note_jump);
								}
								break;
						}
					}
					
					else if (mused.current_patternx >= PED_COMMAND1) //command input
					{
						if(e->key.keysym.sym == SDLK_PERIOD)
						{
							snapshot(S_T_PATTERN);
							
							Uint16 inst = mused.song.pattern[current_pattern()].step[current_patternstep()].command[(mused.current_patternx - PED_COMMAND1) / 4];
							
							switch ((mused.current_patternx - PED_COMMAND1) % 4)
							{
								case 0:
								inst = (inst & 0x0fff);
								break;

								case 1:
								inst = (inst & 0xf0ff);
								break;

								case 2:
								inst = (inst & 0xff0f);
								break;

								case 3:
								inst = (inst & 0xfff0);
								break;
							}
							
							mused.song.pattern[current_pattern()].step[current_patternstep()].command[(mused.current_patternx - PED_COMMAND1) / 4] = validate_command(inst);
							update_pattern_slider(mused.note_jump);
						}
						
						else if (gethex(e->key.keysym.sym) != -1)
						{
							Uint16 inst = mused.song.pattern[current_pattern()].step[current_patternstep()].command[(mused.current_patternx - PED_COMMAND1) / 4];

							switch ((mused.current_patternx - PED_COMMAND1) % 4)
							{
								case 0:
								inst = (inst & 0x0fff) | ((gethex(e->key.keysym.sym) << 12) & 0xf000);
								break;

								case 1:
								inst = (inst & 0xf0ff) | ((gethex(e->key.keysym.sym) << 8) & 0x0f00);
								break;

								case 2:
								inst = (inst & 0xff0f) | ((gethex(e->key.keysym.sym) << 4) & 0x00f0);
								break;

								case 3:
								inst = (inst & 0xfff0) | gethex(e->key.keysym.sym);
								break;
							}

							snapshot(S_T_PATTERN);

							mused.song.pattern[current_pattern()].step[current_patternstep()].command[(mused.current_patternx - PED_COMMAND1) / 4] = validate_command(inst); //you need `& 0x7fff` there to return old command system

							update_pattern_slider(mused.note_jump);
						}
					}
					
					else if (mused.current_patternx >= PED_CTRL && mused.current_patternx < PED_COMMAND1)
					{
						if (e->key.keysym.sym == SDLK_PERIOD || e->key.keysym.sym == SDLK_0)
						{
							snapshot(S_T_PATTERN);

							mused.song.pattern[current_pattern()].step[current_patternstep()].ctrl &= ~(MUS_CTRL_BIT << (mused.current_patternx - PED_CTRL));

							update_pattern_slider(mused.note_jump);
						}
						
						if (e->key.keysym.sym == SDLK_1)
						{
							snapshot(S_T_PATTERN);

							mused.song.pattern[current_pattern()].step[current_patternstep()].ctrl |= (MUS_CTRL_BIT << (mused.current_patternx - PED_CTRL));

							update_pattern_slider(mused.note_jump);
						}
					}
				}
			}
			break;
		}

		break;

		case SDL_KEYUP:

			play_the_jams(e->key.keysym.sym, -1, 0);

		break;
	}
}

void edit_program_event(SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:

		switch (e->key.keysym.sym)
		{
			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
				begin_selection(mused.current_program_step);
			break;

			case SDLK_PERIOD:
				snapshot(S_T_INSTRUMENT);
				
				if(mused.show_four_op_menu)
				{
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step] = MUS_FX_NOP;
				}
				
				else
				{
					mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][mused.current_program_step] = MUS_FX_NOP;
				}
			break;

			case SDLK_SPACE:
			{
				snapshot(S_T_INSTRUMENT);
				
				if(mused.show_four_op_menu)
				{
					Uint16 opcode = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step];
					
					if (((opcode & 0xff00) != MUS_FX_JUMP && (opcode & 0xff00) != MUS_FX_LABEL && (opcode & 0xff00) != MUS_FX_RELEASE_POINT && (opcode & 0xff00) != MUS_FX_LOOP && opcode != MUS_FX_NOP && opcode != MUS_FX_END) && (mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][my_min(mused.current_program_step + 1, MUS_PROG_LEN)] & 0xff00) != MUS_FX_JUMP)
						//mused.song.instrument[mused.current_instrument].program[mused.current_program_step] ^= 0x8000; //old command mused.song.instrument[mused.current_instrument].program[mused.current_program_step] ^= 0x8000;
						mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step / 8] ^= (1 << (mused.current_program_step & 7));
				}
				
				else
				{
					Uint16 opcode = mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][mused.current_program_step];
					
					if (((opcode & 0xff00) != MUS_FX_JUMP && (opcode & 0xff00) != MUS_FX_LABEL && (opcode & 0xff00) != MUS_FX_RELEASE_POINT && (opcode & 0xff00) != MUS_FX_LOOP && opcode != MUS_FX_NOP && opcode != MUS_FX_END) && (mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][my_min(mused.current_program_step + 1, MUS_PROG_LEN)] & 0xff00) != MUS_FX_JUMP)
						//mused.song.instrument[mused.current_instrument].program[mused.current_program_step] ^= 0x8000; //old command mused.song.instrument[mused.current_instrument].program[mused.current_program_step] ^= 0x8000;
						mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][mused.current_program_step / 8] ^= (1 << (mused.current_program_step & 7));
				}
			}
			break;

			case SDLK_RETURN:
			{
				MusInstrument *inst = &mused.song.instrument[mused.current_instrument];
				Uint16 *param;
	
				if(mused.show_four_op_menu)
				{
					param = &inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step];
				}
				
				else
				{
					param = &inst->program[mused.current_instrument_program][mused.current_program_step];
				}
				
				*param = validate_command(*param);
			}
			break;

			case SDLK_PAGEDOWN:
			case SDLK_DOWN:
			{
				MusInstrument *inst = &mused.song.instrument[mused.current_instrument];
				
				Uint16 *param;
	
				if(mused.show_four_op_menu)
				{
					param = &inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step];
				}
				
				else
				{
					param = &inst->program[mused.current_instrument_program][mused.current_program_step];
				}
				
				*param = validate_command(*param);

				int steps = 1;
				if (e->key.keysym.sym == SDLK_PAGEDOWN) steps *= 16;
				
				if(mused.show_four_op_menu)
				{
					slider_move_position(&mused.current_program_step, &mused.fourop_program_position[mused.selected_operator - 1], &mused.four_op_slider_param, steps);
				}
				
				else
				{
					slider_move_position(&mused.current_program_step, &mused.program_position, &mused.program_slider_param, steps);
				}
				
				//slider_move_position(&mused.current_program_step, &mused.program_position, &mused.program_slider_param, steps);

				if (e->key.keysym.mod & KMOD_SHIFT)
				{
					select_range(mused.current_program_step);
				}
			}
			break;

			case SDLK_PAGEUP:
			case SDLK_UP:
			{
				MusInstrument *inst = &mused.song.instrument[mused.current_instrument];
				
				Uint16 *param;
	
				if(mused.show_four_op_menu)
				{
					param = &inst->ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step];
				}
				
				else
				{
					param = &inst->program[mused.current_instrument_program][mused.current_program_step];
				}
				
				*param = validate_command(*param);

				int steps = 1;
				if (e->key.keysym.sym == SDLK_PAGEUP) steps *= 16;
				
				if(mused.show_four_op_menu)
				{
					slider_move_position(&mused.current_program_step, &mused.fourop_program_position[mused.selected_operator - 1], &mused.four_op_slider_param, -steps);
				}
				
				else
				{
					slider_move_position(&mused.current_program_step, &mused.program_position, &mused.program_slider_param, -steps);
				}
				
				//slider_move_position(&mused.current_program_step, &mused.program_position, &mused.program_slider_param, -steps);

				if (e->key.keysym.mod & KMOD_SHIFT)
				{
					select_range(mused.current_program_step);
				}
			}
			break;

			case SDLK_INSERT:
			{
				snapshot(S_T_INSTRUMENT);
				for (int i = MUS_PROG_LEN - 1; i > mused.current_program_step; --i)
				{
					if(mused.show_four_op_menu)
					{
						mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i - 1];
						
						bool b = (mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][(i - 1) / 8] & (1 << ((i - 1) & 7)));
						
						if(b == false)
						{
							mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] &= ~(1 << (i & 7));
						}
						
						else
						{
							mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] |= (1 << (i & 7));
						}
						
						mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][(i - 1) / 8] &= ~(1 << ((i - 1) & 7));
					}
					
					else
					{
						mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][i] = mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][i - 1];
						
						bool b = (mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][(i - 1) / 8] & (1 << ((i - 1) & 7)));
						
						if(b == false)
						{
							mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][i / 8] &= ~(1 << (i & 7));
						}
						
						else
						{
							mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][i / 8] |= (1 << (i & 7));
						}
						
						mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][(i - 1) / 8] &= ~(1 << ((i - 1) & 7));
					}
				}
				
				if(mused.show_four_op_menu)
				{
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step] = MUS_FX_NOP;
				}
				
				else
				{
					mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][mused.current_program_step] = MUS_FX_NOP;
				}
			}
			break;

			case SDLK_BACKSPACE:
			case SDLK_DELETE:
			{
				snapshot(S_T_INSTRUMENT);
				if (e->key.keysym.sym == SDLK_BACKSPACE)
				{
					if (mused.current_program_step > 0) 
					{
						if(mused.show_four_op_menu)
						{
							slider_move_position(&mused.current_program_step, &mused.fourop_program_position[mused.selected_operator - 1], &mused.four_op_slider_param, -1);
						}
						
						else
						{
							slider_move_position(&mused.current_program_step, &mused.program_position, &mused.program_slider_param, -1);
						}
						
						//slider_move_position(&mused.current_program_step, &mused.program_position, &mused.program_slider_param, -1);
					}
					
					else break;
				}

				if (!(mused.flags & DELETE_EMPTIES) || e->key.keysym.sym == SDLK_BACKSPACE)
				{
					for (int i = mused.current_program_step; i < MUS_PROG_LEN-1; ++i)
					{
						if(mused.show_four_op_menu)
						{
							mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] = mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i + 1];
							
							bool b = (mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][(i + 1) / 8] & (1 << ((i + 1) & 7)));
							
							if(b == false)
							{
								mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] &= ~(1 << (i & 7));
							}
							
							else
							{
								mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] |= (1 << (i & 7));
							}
						}
						
						else
						{
							mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][i] = mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][i + 1];
							
							bool b = (mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][(i + 1) / 8] & (1 << ((i + 1) & 7)));
							
							if(b == false)
							{
								mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][i / 8] &= ~(1 << (i & 7));
							}
							
							else
							{
								mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][i / 8] |= (1 << (i & 7));
							}
						}
					}
					
					if(mused.show_four_op_menu)
					{
						mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][MUS_PROG_LEN - 1] = MUS_FX_NOP;
					}
					
					else
					{
						mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][MUS_PROG_LEN - 1] = MUS_FX_NOP;
					}
				}
				
				else
				{
					if(mused.show_four_op_menu)
					{
						mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step] = MUS_FX_NOP;
						
						mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step / 8] &= ~(1 << (mused.current_program_step & 7));
					}
					
					else
					{
						mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][mused.current_program_step] = MUS_FX_NOP;
						
						mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][mused.current_program_step / 8] &= ~(1 << (mused.current_program_step & 7));
					}
					
					++mused.current_program_step;
					if (mused.current_program_step >= MUS_PROG_LEN)
						mused.current_program_step = MUS_PROG_LEN - 1;
				}
			}
			break;

			case SDLK_RIGHT:
			{
				clamp(mused.editpos, +1, 0, 3);
			}
			break;

			case SDLK_LEFT:
			{
				clamp(mused.editpos, -1, 0, 3);
			}
			break;

			default:
			{
				int v = gethex(e->key.keysym.sym);

				if (v != -1)
				{
					snapshot(S_T_INSTRUMENT);
					editparambox(v);
				}
			}
			break;
		}

		break;

		case SDL_KEYUP:
			
			play_the_jams(e->key.keysym.sym, -1, 0);
			
		break;
	}
}


void edit_text(SDL_Event *e)
{
	int r = generic_edit_text(e, mused.edit_buffer, mused.edit_buffer_size, &mused.editpos);

	if (r == -1)
	{
		strcpy(mused.edit_buffer, mused.edit_backup_buffer);
		change_mode(mused.prev_mode);
	}
	
	else if (r == 1)
	{
		change_mode(mused.prev_mode);
	}
}


void set_room_size(int fx, int size, int vol, int dec)
{
	snapshot(S_T_FX);

	int min_delay = 5;
	int ms = (CYDRVB_SIZE - min_delay) * size / 64;

	if (mused.fx_room_ticks)
	{
		ms = 1000 * size / 16 * mused.song.song_speed / mused.song.song_rate;
		min_delay = ms;
	}

	int low = CYDRVB_LOW_LIMIT + 300; // +30 dB

	for (int i = 0; i < mused.song.fx[mused.fx_bus].rvb.taps_quant; ++i)
	{
		int p, g, e = 1;

		if (mused.fx_room_ticks)
		{
			p = i * ms + min_delay;
			g = low - low * pow(1.0 - (double)p / CYDRVB_SIZE, (double)dec / 3) * vol / 16;
		}
		else
		{
			p = rnd(i * ms / mused.song.fx[mused.fx_bus].rvb.taps_quant, (i + 1) * ms / mused.song.fx[mused.fx_bus].rvb.taps_quant) + min_delay;
			g = low - low * pow(1.0 - (double)my_min(p, ms - 1) / ms, (double)dec / 3) * vol / 16;
		}

		if (p >= CYDRVB_SIZE)
		{
			p = CYDRVB_SIZE - 1;
			e = 0;
		}

		mused.song.fx[fx].rvb.tap[i].delay = p;
		mused.song.fx[fx].rvb.tap[i].gain = g;
		mused.song.fx[fx].rvb.tap[i].flags = e;
		
		mused.song.fx[fx].rvb.tap[i].panning = CYD_PAN_CENTER;
	}
	
	//mused.song.fx[mused.fx_bus].rvb.taps_quant = CYDRVB_TAPS;

	mus_set_fx(&mused.mus, &mused.song);
}


void fx_add_param(int d)
{
	if (d < 0) d = -1; else if (d > 0) d = 1;

	if (SDL_GetModState() & KMOD_SHIFT)
	{
		switch (mused.edit_reverb_param)
		{
			case R_DELAY:
				if (mused.flags & SHOW_DELAY_IN_TICKS)
					d *= (1000 / (float)mused.song.song_rate) * mused.song.song_speed;
				else
					d *= 100;
				break;

			default:
				d *= 10;
				break;
		}
	}

	snapshot_cascade(S_T_FX, mused.fx_bus, mused.edit_reverb_param);

	switch (mused.edit_reverb_param)
	{
		case R_FX_BUS:
		{
			clamp(mused.fx_bus, d, 0, CYD_MAX_FX_CHANNELS - 1);
		}
		break;

		case R_CRUSH:
		{
			flipbit(mused.song.fx[mused.fx_bus].flags, CYDFX_ENABLE_CRUSH);
		}
		break;

		case R_CRUSHDITHER:
		{
			flipbit(mused.song.fx[mused.fx_bus].flags, CYDFX_ENABLE_CRUSH_DITHER);
		}
		break;

		case R_CRUSHGAIN:
		{
			clamp(mused.song.fx[mused.fx_bus].crushex.gain, d, 0, 128);
			mus_set_fx(&mused.mus, &mused.song);
		}
		break;

		case R_CRUSHBITS:
		{
			clamp(mused.song.fx[mused.fx_bus].crush.bit_drop, d, 0, 15);
			mus_set_fx(&mused.mus, &mused.song);
		}
		break;

		case R_CRUSHDOWNSAMPLE:
		{
			clamp(mused.song.fx[mused.fx_bus].crushex.downsample, d, 0, 64);
			mus_set_fx(&mused.mus, &mused.song);
		}
		break;

		case R_CHORUS:
		{
			flipbit(mused.song.fx[mused.fx_bus].flags, CYDFX_ENABLE_CHORUS);
		}
		break;

		case R_MINDELAY:
		{
			clamp(mused.song.fx[mused.fx_bus].chr.min_delay, d, 0, mused.song.fx[mused.fx_bus].chr.max_delay);
			clamp(mused.song.fx[mused.fx_bus].chr.min_delay, 0, mused.song.fx[mused.fx_bus].chr.min_delay, 255);
		}
		break;

		case R_MAXDELAY:
		{
			clamp(mused.song.fx[mused.fx_bus].chr.max_delay, d, mused.song.fx[mused.fx_bus].chr.min_delay, 255);
			clamp(mused.song.fx[mused.fx_bus].chr.min_delay, 0, 0, mused.song.fx[mused.fx_bus].chr.max_delay);
		}
		break;

		case R_SEPARATION:
		{
			clamp(mused.song.fx[mused.fx_bus].chr.sep, d, 0, 64);
		}
		break;

		case R_RATE:
		{
			clamp(mused.song.fx[mused.fx_bus].chr.rate, d, 0, 255);
		}
		break;

		case R_PITCH_INACCURACY:
		{
			clamp(mused.song.pitch_inaccuracy, d, 0, 12);
		}
		break;

		case R_MULTIPLEX:
		{
			flipbit(mused.song.fx[mused.fx_bus].flags, MUS_ENABLE_MULTIPLEX);
		}
		break;

		case R_MULTIPLEX_PERIOD:
		{
			clamp(mused.song.multiplex_period, d, 0, 255);
		}
		break;

		case R_ENABLE:
		{
			flipbit(mused.song.fx[mused.fx_bus].flags, CYDFX_ENABLE_REVERB);
		}
		break;

		case R_ROOMSIZE:
		{
			clamp(mused.fx_room_size, d, 1, 64);
		}
		break;

		case R_ROOMDECAY:
		{
			clamp(mused.fx_room_dec, d, 1, 9);
		}
		break;

		case R_ROOMVOL:
		{
			clamp(mused.fx_room_vol, d, 1, 16);
		}
		break;

		case R_SNAPTICKS:
		{
			flipbit(mused.fx_room_ticks, 1);
		}
		break;

		case R_TAP:
		{
			clamp(mused.fx_tap, d, 0, CYDRVB_TAPS - 1);
		}
		break;

		case R_DELAY:
		{
			clamp(mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].delay, d * 1, 0, CYDRVB_SIZE - 1);
			mus_set_fx(&mused.mus, &mused.song);
		}
		break;

		case R_GAIN:
		{
			clamp(mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].gain, d * 1, CYDRVB_LOW_LIMIT, 0);
			mus_set_fx(&mused.mus, &mused.song);
		}
		break;
		
		case R_NUM_TAPS: //wasn't there
		{
			int temp = (int)mused.song.fx[mused.fx_bus].rvb.taps_quant + (int)d;
			
			if(temp >= 0)
			{
				clamp(mused.song.fx[mused.fx_bus].rvb.taps_quant, d * 1, 0, CYDRVB_TAPS);
				mused.song.fx[mused.fx_bus].rvb.tap[mused.song.fx[mused.fx_bus].rvb.taps_quant - 1].panning = CYD_PAN_CENTER;
				mus_set_fx(&mused.mus, &mused.song);
			}
		}
		break;

		case R_PANNING:
		{
			clamp(mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].panning, d * 8, CYD_PAN_LEFT, CYD_PAN_RIGHT);
			mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].panning &= ~7;
			mus_set_fx(&mused.mus, &mused.song);
		}
		break;

		case R_TAPENABLE:
		{
			flipbit(mused.song.fx[mused.fx_bus].rvb.tap[mused.fx_tap].flags, 1);
		}
		break;
	}

}


void fx_event(SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:

		switch (e->key.keysym.sym)
		{
			case SDLK_DOWN:
			{
				++mused.edit_reverb_param;
				if (mused.edit_reverb_param >= R_N_PARAMS) mused.edit_reverb_param = R_N_PARAMS - 1;
			}
			break;

			case SDLK_UP:
			{
				--mused.edit_reverb_param;
				if (mused.edit_reverb_param < 0) mused.edit_reverb_param = 0;
			}
			break;

			case SDLK_RIGHT:
			{
				fx_add_param(+1);
			}
			break;

			case SDLK_LEFT:
			{
				fx_add_param(-1);
			}
			break;

			default:
				play_the_jams(e->key.keysym.sym, -1, 1);
			break;
		}

		break;

		case SDL_KEYUP:

			play_the_jams(e->key.keysym.sym, -1, 0);

		break;
	}
}


void wave_add_param(int d)
{
	CydWavetableEntry *w = NULL;
	
	if(mused.focus == EDITLOCALSAMPLE && !(mused.show_local_samples_list))
	{
		w = &mused.mus.cyd->wavetable_entries[mused.selected_local_sample];
	}
	
	else
	{
		w = &mused.mus.cyd->wavetable_entries[mused.selected_wavetable];
	}
	
	WgOsc *osc = &mused.wgset.chain[mused.selected_wg_osc];
	
	//MusInstrument* inst = &mused.song.instrument[mused.current_instrument];
	
	//debug("d = %d, lsp = %d wp = %d LS_WAVE = %d, lsp - LS_WAVE %d", d, mused.local_sample_param, mused.wavetable_param, LS_WAVE, mused.local_sample_param - LS_WAVE);
	
	if (d < 0) d = -1; else if (d > 0) d = 1;
	
	int param = 0;
	
	if(mused.focus == EDITLOCALSAMPLE && !(mused.show_local_samples_list))
	{
		param = mused.local_sample_param - LS_WAVE;
	}
	
	else
	{
		param = mused.wavetable_param;
	}

	if (SDL_GetModState() & KMOD_SHIFT)
	{
		switch (param)
		{
			case W_BASE: d *= 12; break;
			case W_BASEFINE: d *= 16; break;
			default: d *= 16; break;
		}
	}
	
	if (SDL_GetModState() & KMOD_CTRL) //wasn't there
	{
		switch (param)
		{
			case W_OSCVOL: d *= 256; break;
			default: d *= 4096; break;
		}
	}
	
	if (SDL_GetModState() & KMOD_ALT) //wasn't there
	{
		switch (param)
		{
			default: d *= 65536; break;
		}
	}
	
	if(!(mused.show_local_samples_list))
	{
		snapshot_cascade(S_T_WAVE_PARAM, mused.selected_wavetable, mused.wavetable_param);
	}

	switch (param)
	{
		case W_INTERPOLATE:
		{
			flipbit(w->flags, CYD_WAVE_NO_INTERPOLATION);
		}
		break;
		
		case W_INTERPOLATION_TYPE:
		{
			if(((w->flags & (CYD_WAVE_INTERPOLATION_BIT_1|CYD_WAVE_INTERPOLATION_BIT_2|CYD_WAVE_INTERPOLATION_BIT_3)) >> 5) + d >= 0 && ((w->flags & (CYD_WAVE_INTERPOLATION_BIT_1|CYD_WAVE_INTERPOLATION_BIT_2|CYD_WAVE_INTERPOLATION_BIT_3)) >> 5) + d <= 2 && (w->flags & CYD_WAVE_NO_INTERPOLATION) == 0)
			{
				w->flags += d << 5;
			}
		}
		break;

		case W_WAVE:
		{
			if(mused.focus == EDITLOCALSAMPLE && !(mused.show_local_samples_list))
			{
				clamp(mused.selected_local_sample, d, 0, CYD_WAVE_MAX_ENTRIES - 1);
			}
			
			else
			{
				clamp(mused.selected_wavetable, d, 0, CYD_WAVE_MAX_ENTRIES - 1);
			}
		}
		break;

		case W_RATE:
		{
			clamp(w->sample_rate, d, 1, 192000);
		}
		break;

		case W_BASE:
		{
			clamp(w->base_note, d * 256, 0, (FREQ_TAB_SIZE - 1) * 256);
		}
		break;

		case W_BASEFINE:
		{
			clamp(w->base_note, d, 0, (FREQ_TAB_SIZE - 1) * 256);
		}
		break;

		case W_LOOP:
		{
			flipbit(w->flags, CYD_WAVE_LOOP);
		}
		break;

		case W_LOOPPINGPONG:
		{
			flipbit(w->flags, CYD_WAVE_PINGPONG);
		}
		break;

		case W_LOOPBEGIN:
		{
			clamp(w->loop_begin, d, 0, my_min(w->samples, w->loop_end));
			clamp(w->loop_end, 0, w->loop_begin, w->samples);
		}
		break;

		case W_LOOPEND:
		{
			clamp(w->loop_end, d, w->loop_begin, w->samples);
			clamp(w->loop_begin, 0, 0, my_min(w->samples, w->loop_end));
		}
		break;

		case W_NUMOSCS:
		{
			mused.wgset.num_oscs += d;

			if (mused.wgset.num_oscs < 1)
				mused.wgset.num_oscs = 1;
			else if (mused.wgset.num_oscs > WG_CHAIN_OSCS)
				mused.wgset.num_oscs = WG_CHAIN_OSCS;
		}
		break;

		case W_OSCTYPE:
		{
			osc->osc = my_max(0, my_min(WG_NUM_OSCS - 1, (int)osc->osc + d));
		}
		break;

		case W_OSCMUL:
		{
			osc->mult = my_max(1, my_min(255, osc->mult + d));
		}
		break;

		case W_OSCSHIFT:
		{
			osc->shift = my_max(0, my_min(255, osc->shift + d));
		}
		break;
		
		case W_OSCVOL: //wasn't there
		{
			//osc->vol = my_min(255, osc->vol + d); 
			//based on `clamp(i->adsr.r, a, 0, 32 * ENVELOPE_SCALE - 1);`
			//clamp(osc->vol, d, 0, 65535);
			
			if(osc->vol + d > 65535)
			{
				osc->vol = 65535;
			}
			
			else if(osc->vol + d < 0)
			{
				osc->vol = 0;
			}
			
			else
			{
				osc->vol += d;
			}
		}
		break;

		case W_OSCEXP:
		{
			osc->exp += d;

			if (osc->exp < 1)
				osc->exp = 1;
			else if (osc->exp > 99)
				osc->exp = 99;
		}
		break;

		case W_OSCABS:
			flipbit(osc->flags, WG_OSC_FLAG_ABS);
		break;

		case W_OSCNEG:
			flipbit(osc->flags, WG_OSC_FLAG_NEG);
		break;

		case W_WAVELENGTH:
		{
			mused.wgset.length = my_max(2, my_min(2147483646, mused.wgset.length + d)); //was `mused.wgset.length = my_max(16, my_min(65536, mused.wgset.length + d));`
		}
		break;
	}
}

void local_sample_add_param(int d)
{
	if(!(mused.show_local_samples_list))
	{
		wave_add_param(d);
		return;
	}
	
	CydWavetableEntry *w = mused.song.instrument[mused.current_instrument].local_samples[mused.selected_local_sample];
	
	MusInstrument* inst = &mused.song.instrument[mused.current_instrument];
	
	if(w == NULL)
	{
		return;
	}

	if (d < 0) d = -1; else if (d > 0) d = 1;

	if (SDL_GetModState() & KMOD_SHIFT)
	{
		switch (mused.local_sample_param)
		{
			case LS_BASE: d *= 12; break;
			default: d *= 16; break;
		}
	}
	
	if (SDL_GetModState() & KMOD_CTRL) //wasn't there
	{
		switch (mused.local_sample_param)
		{
			default: d *= 4096; break;
		}
	}
	
	if (SDL_GetModState() & KMOD_ALT) //wasn't there
	{
		switch (mused.local_sample_param)
		{
			default: d *= 65536; break;
		}
	}

	switch (mused.local_sample_param)
	{
		case LS_ENABLE:
		{
			flipbit(inst->flags, MUS_INST_USE_LOCAL_SAMPLES);
		}
		break;
		
		case LS_LOCAL_SAMPLE:
		{
			clamp(inst->local_sample, d, 0, inst->num_local_samples - 1);
		}
		break;
		
		case LS_INTERPOLATE:
		{
			flipbit(w->flags, CYD_WAVE_NO_INTERPOLATION);
		}
		break;
		
		case LS_INTERPOLATION_TYPE:
		{
			if(((w->flags & (CYD_WAVE_INTERPOLATION_BIT_1|CYD_WAVE_INTERPOLATION_BIT_2|CYD_WAVE_INTERPOLATION_BIT_3)) >> 5) + d >= 0 && ((w->flags & (CYD_WAVE_INTERPOLATION_BIT_1|CYD_WAVE_INTERPOLATION_BIT_2|CYD_WAVE_INTERPOLATION_BIT_3)) >> 5) + d <= 2 && (w->flags & CYD_WAVE_NO_INTERPOLATION) == 0)
			{
				w->flags += d << 5;
			}
		}
		break;

		case LS_WAVE:
		{
			clamp(mused.selected_local_sample, d, 0, inst->num_local_samples - 1);
		}
		break;

		case LS_RATE:
		{
			clamp(w->sample_rate, d, 1, 192000);
		}
		break;

		case LS_BASE:
		{
			clamp(w->base_note, d * 256, 0, (FREQ_TAB_SIZE - 1) * 256);
		}
		break;

		case LS_BASEFINE:
		{
			clamp(w->base_note, d, 0, (FREQ_TAB_SIZE - 1) * 256);
		}
		break;

		case LS_LOOP:
		{
			flipbit(w->flags, CYD_WAVE_LOOP);
		}
		break;

		case LS_LOOPPINGPONG:
		{
			flipbit(w->flags, CYD_WAVE_PINGPONG);
		}
		break;

		case LS_LOOPBEGIN:
		{
			clamp(w->loop_begin, d, 0, my_min(w->samples, w->loop_end));
			clamp(w->loop_end, 0, w->loop_begin, w->samples);
		}
		break;

		case LS_LOOPEND:
		{
			clamp(w->loop_end, d, w->loop_begin, w->samples);
			clamp(w->loop_begin, 0, 0, my_min(w->samples, w->loop_end));
		}
		break;
		
		case LS_BINDTONOTES:
		{
			flipbit(inst->flags, MUS_INST_BIND_LOCAL_SAMPLES_TO_NOTES);
		}
		break;
	}
}

void wave_event(SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:

		switch (e->key.keysym.sym)
		{
			case SDLK_DOWN:
			{
				++mused.wavetable_param;

				if (!(mused.flags & SHOW_WAVEGEN) && mused.wavetable_param >= W_NUMOSCS && mused.wavetable_param <= W_TOOLBOX)
				{
					mused.wavetable_param = W_TOOLBOX + 1;
				}

				if (mused.wavetable_param >= W_N_PARAMS) mused.wavetable_param = W_N_PARAMS - 1;
			}
			break;
			
			case SDLK_DELETE:
			{
				char buffer[500];
				
				snprintf(buffer, 499, "Delete selected wavetable (%s)?", mused.song.wavetable_names[mused.selected_wavetable]);
				
				if (confirm(domain, mused.slider_bevel, &mused.largefont, buffer))
				{
					CydWavetableEntry *wave = &mused.mus.cyd->wavetable_entries[mused.selected_wavetable];
					
					wave->flags = 0;
					wave->sample_rate = 0;
					wave->samples = 0;
					wave->loop_begin = 0;
					wave->loop_end = 0;
					wave->base_note = C_ZERO;
					
					free(wave->data);
					wave->data = NULL;
					
					cyd_wave_entry_init(wave, NULL, 0, 0, 0, 0, 0);
					
					memset(mused.song.wavetable_names[mused.selected_wavetable], 0, MUS_WAVETABLE_NAME_LEN + 1);
					
					if (!(mused.flags & SHOW_WAVEGEN))
					{
						SDL_FillRect(mused.wavetable_preview->surface, NULL, SDL_MapRGB(mused.wavetable_preview->surface->format, (colors[COLOR_WAVETABLE_BACKGROUND] >> 16) & 255, (colors[COLOR_WAVETABLE_BACKGROUND] >> 8) & 255, colors[COLOR_WAVETABLE_BACKGROUND] & 255));
						
						gfx_update_texture(domain, mused.wavetable_preview);
					}
				}
			}
			break;

			case SDLK_UP:
			{
				--mused.wavetable_param;

				if (!(mused.flags & SHOW_WAVEGEN) && mused.wavetable_param >= W_NUMOSCS && mused.wavetable_param <= W_TOOLBOX)
				{
					mused.wavetable_param = W_NUMOSCS - 1;
				}

				if (mused.wavetable_param < 0) mused.wavetable_param = 0;
			}
			break;

			case SDLK_RIGHT:
			{
				wave_add_param(+1);
			}
			break;

			case SDLK_LEFT:
			{
				wave_add_param(-1);
			}
			break;

			default:
				wave_the_jams(e->key.keysym.sym);
			break;
		}

		break;
	}
}


void local_sample_event(SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:

		switch (e->key.keysym.sym)
		{
			case SDLK_DOWN:
			{
				++mused.local_sample_param;

				if (mused.local_sample_param >= LS_N_PARAMS) mused.local_sample_param = LS_N_PARAMS - 1;
			}
			break;
			
			case SDLK_DELETE:
			{
				char buffer[500];
				
				snprintf(buffer, 499, "Delete selected wavetable (%s)?", (mused.show_local_samples_list ? mused.song.instrument[mused.current_instrument].local_sample_names[mused.selected_local_sample] : mused.song.wavetable_names[mused.selected_local_sample]));
				
				if (confirm(domain, mused.slider_bevel, &mused.largefont, buffer))
				{
					CydWavetableEntry *wave = NULL;
					
					if(mused.show_local_samples_list)
					{
						wave = mused.song.instrument[mused.current_instrument].local_samples[mused.selected_local_sample];
					}
					
					else
					{
						wave = &mused.mus.cyd->wavetable_entries[mused.selected_local_sample];
					}
					
					if(wave == NULL)
					{
						return;
					}
					
					wave->flags = 0;
					wave->sample_rate = 0;
					wave->samples = 0;
					wave->loop_begin = 0;
					wave->loop_end = 0;
					wave->base_note = C_ZERO;
					
					free(wave->data);
					wave->data = NULL;
					
					cyd_wave_entry_init(wave, NULL, 0, 0, 0, 0, 0);
					
					memset(mused.show_local_samples_list ? mused.song.instrument[mused.current_instrument].local_sample_names[mused.selected_local_sample] : mused.song.wavetable_names[mused.selected_local_sample], 0, MUS_WAVETABLE_NAME_LEN + 1);
					
					SDL_FillRect(mused.wavetable_preview->surface, NULL, SDL_MapRGB(mused.wavetable_preview->surface->format, (colors[COLOR_WAVETABLE_BACKGROUND] >> 16) & 255, (colors[COLOR_WAVETABLE_BACKGROUND] >> 8) & 255, colors[COLOR_WAVETABLE_BACKGROUND] & 255));
					
					gfx_update_texture(domain, mused.wavetable_preview);
				}
			}
			break;

			case SDLK_UP:
			{
				--mused.local_sample_param;

				if (mused.local_sample_param < 0) mused.local_sample_param = 0;
			}
			break;

			case SDLK_RIGHT:
			{
				local_sample_add_param(+1);
			}
			break;

			case SDLK_LEFT:
			{
				local_sample_add_param(-1);
			}
			break;

			default:
				wave_the_jams(e->key.keysym.sym);
			break;
		}

		break;
	}
}


void songinfo_add_param(int d)
{
	if (d < 0) d = -1; else if (d > 0) d = 1;

	if (SDL_GetModState() & KMOD_SHIFT)
		d *= 16;
	
	if (SDL_GetModState() & KMOD_CTRL)
		d *= 256;

	if (d) snapshot_cascade(S_T_SONGINFO, mused.songinfo_param, -1);

	switch (mused.songinfo_param)
	{
		case SI_MASTERVOL:
		{
			change_master_volume(MAKEPTR(d), 0, 0);
			mused.cyd.mus_volume = mused.mus.volume;
			
			break;
		}

		case SI_LENGTH:
			if(d != 0)
			{
				change_song_length(MAKEPTR(d * mused.sequenceview_steps), 0, 0);
			}
			break;

		case SI_LOOP:
			change_loop_point(MAKEPTR(d * mused.sequenceview_steps), 0, 0);
			break;

		case SI_STEP:
			change_seq_steps(MAKEPTR(d), 0, 0);
			break;

		case SI_SPEED1:
			change_song_speed(MAKEPTR(0), MAKEPTR(d), 0);
			break;

		case SI_SPEED2:
			change_song_speed(MAKEPTR(1), MAKEPTR(d), 0);
			break;

		case SI_RATE:
			change_song_rate(MAKEPTR(d), 0, 0);
			break;

		case SI_TIME1:
			change_timesig(MAKEPTR(d), MAKEPTR(1), 0);
			break;
			
		case SI_TIME2:
			change_timesig(MAKEPTR(d), MAKEPTR(2), 0);
			break;

		case SI_OCTAVE:
			change_octave(MAKEPTR(d), 0, 0);
			break;

		case SI_CHANNELS:
			change_channels(MAKEPTR(d), 0, 0);
			break;
	}
}


void songinfo_event(SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:

		switch (e->key.keysym.sym)
		{
			case SDLK_DOWN:
			{
				++mused.songinfo_param;
				if (mused.songinfo_param >= SI_N_PARAMS) mused.songinfo_param = SI_N_PARAMS - 1;
			}
			break;

			case SDLK_UP:
			{
				--mused.songinfo_param;
				if (mused.songinfo_param < 0) mused.songinfo_param = 0;
			}
			break;

			case SDLK_RIGHT:
			{
				songinfo_add_param(+1);
			}
			break;

			case SDLK_LEFT:
			{
				songinfo_add_param(-1);
			}
			break;

			default: break;
		}

		break;
	}
}


void note_event(SDL_Event *e)
{
	switch (e->type)
	{
		case MSG_NOTEON:
		{
			Uint32 note = e->user.code + 12 * 5; //to account for negative octaves
			
			play_note(note);
			
			if (mused.focus == EDITPATTERN && (mused.flags & EDIT_MODE) && get_current_step() && mused.current_patternx == PED_NOTE)
			{
				write_note(note);
			}
		}
		break;

		case MSG_NOTEOFF:
		{
			Uint32 note = e->user.code + 12 * 5; //to account for negative octaves
			
			stop_note(note);
		}
		break;

		case MSG_PROGRAMCHANGE:
		{
			if (e->user.code >= 0 && e->user.code < NUM_INSTRUMENTS)
				mused.current_instrument = e->user.code;
		}
		break;
	}
}
