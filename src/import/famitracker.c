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

#include "famitracker.h"

extern Mused mused;
extern GfxDomain *domain;

const char* block_sigs[FT_BLOCK_TYPES] = 
{
	"PARAMS",
	"TUNING",
	"INFO",
	"INSTRUMENTS",
	"SEQUENCES",
	"FRAMES",
	"PATTERNS",
	"DPCM SAMPLES",
	"HEADER",
	"COMMENTS",
	// VRC6
	"SEQUENCES_VRC6",
	// N163
	"SEQUENCES_N163",
	"SEQUENCES_N106",
	// Sunsoft
	"SEQUENCES_S5B",
	// 0CC-FamiTracker specific
	"DETUNETABLES",
	"GROOVES",
	"BOOKMARKS",
	"PARAMS_EXTRA",
};

const char* sequence_names[FT_SEQ_CNT] = 
{
	"volume",
	"arpeggio",
	"pitch",
	"hi-pitch",
	"pul.w./noi mode"
};

const char* sequence_names_vrc6[FT_SEQ_CNT] = 
{
	"volume",
	"arpeggio",
	"pitch",
	"hi-pitch",
	"pulse width"
};

const char* sequence_names_n163[FT_SEQ_CNT] = 
{
	"volume",
	"arpeggio",
	"pitch",
	"hi-pitch",
	"wave index"
};

const char* sequence_names_fds[FT_SEQ_CNT] = 
{
	"volume",
	"arpeggio",
	"pitch",
	"",
	""
};

const char* sequence_names_vrc7[FT_SEQ_CNT] = 
{
	"",
	"",
	"",
	"",
	""
};

const char* sequence_names_s5b[FT_SEQ_CNT] = 
{
	"volume",
	"arpeggio",
	"pitch",
	"hi-pitch",
	"noise / mode"
};

const Uint8 eff_conversion_050[][2] = 
{
	{FT_EF_SUNSOFT_NOISE,		FT_EF_NOTE_RELEASE},
	{FT_EF_VRC7_PORT,			FT_EF_GROOVE},
	{FT_EF_VRC7_WRITE,			FT_EF_TRANSPOSE},
	{FT_EF_NOTE_RELEASE,		FT_EF_N163_WAVE_BUFFER},
	{FT_EF_GROOVE,				FT_EF_FDS_VOLUME},
	{FT_EF_TRANSPOSE,			FT_EF_FDS_MOD_BIAS},
	{FT_EF_N163_WAVE_BUFFER,	FT_EF_SUNSOFT_NOISE},
	{FT_EF_FDS_VOLUME,			FT_EF_VRC7_PORT},
	{FT_EF_FDS_MOD_BIAS,		FT_EF_VRC7_WRITE},
	{0xFF,	0xFF}, //end mark
};

// from https://www.nesdev.org/wiki/APU_DMC
const Uint16 dpcm_sample_rates_ntsc[16] = 
{ 4182, 4710, 5264, 5593, 6258, 7046, 7919, 8363, 9420, 11186, 12604, 13983, 16885, 21307, 24858, 33144 };

const Uint16 dpcm_sample_rates_pal[16] = 
{ 4177, 4697, 5261, 5579, 6024, 7045, 7917, 8397, 9447, 11234, 12696, 14090, 16965, 21316, 25191, 33252 };

const Uint8 convert_volumes_16[16] = 
{ 0, 9, 17, 26, 34, 43, 51, 60, 68, 77, 85, 94, 102, 111, 119, 128 };

const Uint8 convert_volumes_32[32] = 
{ 
	0, 4, 8, 12, 17, 21, 25, 29, 33, 37, 41, 45, 50, 54, 58, 62,
	66, 70, 74, 78, 83, 87, 91, 95, 99, 103, 107, 111, 116, 120, 124, 128
};

const Uint8 convert_volumes_64[64] = 
{ 
	0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
	32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62,
	65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 95,
	97, 99, 101, 103, 105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125, 128
};

const Uint8 ft_2a03_pulse_widths[4] = 
{ 0xe0, 0xc0, 0x80, 0x40 };

const Uint8 ft_vrc6_pulse_widths[8] = 
{ 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80 };

const Uint8 noise_notes[16] = 
{ 
	C_ZERO - 3, C_ZERO + 9, C_ZERO + 12 + 9, C_ZERO + 12 * 2 + 1, C_ZERO + 12 * 2 + 9, C_ZERO + 12 * 3 + 2, 
	C_ZERO + 12 * 3 + 5, C_ZERO + 12 * 3 + 11, C_ZERO + 12 * 4 + 3, C_ZERO + 12 * 4 + 5, C_ZERO + 12 * 4 + 11, 
	C_ZERO + 12 * 5 + 5, C_ZERO + 12 * 5 + 11, C_ZERO + 12 * 6 + 7, C_ZERO + 12 * 7 + 11, C_ZERO + 12 * 9 + 11
};

const Uint8 s5b_noise_notes[32] = /*TODO: add actual notes*/
{ 
	C_ZERO + 12 + 10, C_ZERO + 12 + 11, C_ZERO + 12 * 2, C_ZERO + 12 * 2, C_ZERO + 12 * 2 + 1, C_ZERO + 12 * 2 + 2, C_ZERO + 12 * 2 + 3, C_ZERO + 12 * 2 + 4, C_ZERO + 12 * 2 + 5, 
	C_ZERO + 12 * 2 + 5, C_ZERO + 12 * 2 + 6, C_ZERO + 12 * 2 + 6, C_ZERO + 12 * 2 + 7, C_ZERO + 12 * 2 + 8, C_ZERO + 12 * 2 + 8, C_ZERO + 12 * 2 + 10, C_ZERO + 12 * 3,
	C_ZERO + 12 * 3, C_ZERO + 12 * 3 + 1, C_ZERO + 12 * 3 + 2, C_ZERO + 12 * 3 + 4, C_ZERO + 12 * 3 + 5, C_ZERO + 12 * 3 + 8, C_ZERO + 12 * 3 + 11, C_ZERO + 12 * 4,
	C_ZERO + 12 * 4 + 2, C_ZERO + 12 * 4 + 4, C_ZERO + 12 * 4 + 10, C_ZERO + 12 * 5 + 2, C_ZERO + 12 * 5 + 11, C_ZERO + 12 * 6 + 11, C_ZERO + 12 * 7 + 11,
};

Uint8 expansion_chips;
Uint8 namco_channels;
Uint32 version;

bool transpose_fds;

Uint16 pattern_length;

Uint16 sequence_length;
Uint8 selected_subsong;

Uint8 num_subsongs;

Uint8 machine;

Uint8 num_instruments;

Uint8** inst_numbers_to_inst_indices;

Uint8* initial_delta_counter_positions;

Uint8** effect_columns;

Uint8* channels_to_chips;

ft_inst* ft_instruments;

Uint8** ft_sequence;
Uint16** klystrack_sequence;

char** subsong_names;

bool is_dn_fami_module;

Uint32 display_comment;

int draw_box_select_subsong(GfxDomain *dest, GfxSurface *gfx, const Font *font, const SDL_Event *event, const char *msg, int *selected)
{
	int w = 0, max_w = 300, h = font->h;
	
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
	
	h += 14 * num_subsongs - 16;
	
	SDL_Rect area = { dest->screen_w / 2 - max_w / 2, dest->screen_h / 2 - h / 2 - 8, max_w, h + 16 + 16 + 4 };
	
	bevel(dest, &area, gfx, BEV_MENU);
	SDL_Rect content, pos;
	copy_rect(&content, &area);
	adjust_rect(&content, 8);
	copy_rect(&pos, &content);
	
	font_write(font, dest, &pos, msg);
	update_rect(&content, &pos);
		
	*selected = (*selected + num_subsongs) % num_subsongs;
	
	//pos.w = 50;
	pos.w = content.w;
	pos.h = 14;
	pos.x = content.x;
	pos.y -= 14 * num_subsongs;
	
	int r = -1;
	int idx = 0;
	
	for (int i = 0; i < num_subsongs; ++i)
	{
		int p = button_text_event(dest, event, &pos, gfx, font, BEV_BUTTON, BEV_BUTTON_ACTIVE, subsong_names[i], NULL, 0, 0, 0);
		
		if (idx == *selected)
		{	
			if (event->type == SDL_KEYDOWN && (event->key.keysym.sym == SDLK_SPACE || event->key.keysym.sym == SDLK_RETURN))
				p = 1;
		
			bevel(dest, &pos, gfx, BEV_CURSOR);
		}
		
		pos.y += 14;
		
		//update_rect(&content, &pos);
		if (p & 1) r = i;
		++idx;
	}
	
	return r;
}

int msgbox_select_subsong(GfxDomain *domain, GfxSurface *gfx, const Font *font, const char *msg)
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
							return selected;
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
			int r = draw_box_select_subsong(domain, gfx, font, &e, msg, &selected);
			gfx_domain_flip(domain);
			set_repeat_timer(NULL);
			
			if (r != -1) 
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

void load_sequence_indices(FILE* f, Uint8 inst_num) //bool CSeqInstrument::Load(CDocumentFile *pDocFile)
{
	Uint32 num_macros = 0;
	
	read_uint32(f, &num_macros);

	num_macros = FT_SEQ_CNT; //idk why

	for(int i = 0; i < num_macros; i++)
	{
		Uint8 enable = 0;
		fread(&enable, 1, 1, f);

		//seq_inst_enable[inst_num][i] = enable;

		ft_instruments[inst_num].seq_enable[i] = enable;

		Uint8 index = 0;
		fread(&index, 1, 1, f);

		//seq_inst_index[inst_num][i] = index;

		ft_instruments[inst_num].seq_indices[i] = index;

		//debug("inst num %d seq %d enable %d index %d", inst_num, i, enable, index);
	}
}

#define NOTE_RANGE 12

int GET_OCTAVE(int midi_note)
{
	int x = midi_note / NOTE_RANGE;
	if (midi_note < 0 && (midi_note % NOTE_RANGE)) --x;
	return x;
}

int GET_NOTE(int midi_note)
{
	int x = midi_note % NOTE_RANGE;
	if (x < 0) x += NOTE_RANGE;
	return ++x;
}

void load_dpcm_sample_map(Uint8 note, Uint8 octave, MusInstrument* inst, FILE* f, ftm_block* block, Uint8 inst_num /*famitracker inst*/, Uint8 i) //const auto ReadAssignment = [&] (int Octave, int Note)
{
	inst->flags |= MUS_INST_USE_LOCAL_SAMPLES | MUS_INST_BIND_LOCAL_SAMPLES_TO_NOTES;

	Uint8 index = 0;
	Uint8 pitch = 0;

	Uint8 initial_delta_counter_position = 0;

	fread(&index, 1, 1, f);

	if(index > FT_MAX_DSAMPLES)
	{
		index = 0;
	}

	fread(&pitch, 1, 1, f); //0-15 = 0-15 unlooped, 128-143 = 0-15 looped

	if(block->version > 5)
	{
		fread(&initial_delta_counter_position, 1, 1, f);
		initial_delta_counter_positions[index] = initial_delta_counter_position;
	}

	if(octave * 12 + note < 96)
	{
		inst->note_to_sample_array[C_ZERO + octave * 12 + note].sample = (index > 0) ? (index - 1) : MUS_NOTE_TO_SAMPLE_NONE;
		inst->note_to_sample_array[C_ZERO + octave * 12 + note].flags |= MUS_NOTE_TO_SAMPLE_GLOBAL;

		ft_instruments[i].dpcm_sample_map.sample[octave * 12 + note] = (index > 0) ? (index - 1) : MUS_NOTE_TO_SAMPLE_NONE;
		ft_instruments[i].dpcm_sample_map.pitch[octave * 12 + note] = pitch;

		if(pitch == 0xff)
		{
			ft_instruments[i].dpcm_sample_map.sample[octave * 12 + note] = MUS_NOTE_TO_SAMPLE_NONE;
		}
	}
}

bool load_inst_2a03(FILE* f, ftm_block* block, MusInstrument* inst, Uint8 inst_num, Uint8 i /*klystrack inst num*/)
{
	bool success = true;

	//debug("loading 2A03 instrument");

	Uint8 octaves = (block->version == 1) ? 6 : 8;

	load_sequence_indices(f, inst_num);

	if(block->version >= 7)
	{
		Uint32 num_notes = 0;

		read_uint32(f, &num_notes);

		for(int i1 = 0; i1 < num_notes; i1++)
		{
			Sint8 note = 0;

			fread(&note, 1, 1, f);

			load_dpcm_sample_map(GET_NOTE((int)note) - 1, GET_OCTAVE((int)note) + 1, inst, f, block, inst_num, i); //klystrack C-1 is FT C-0
		}
	}

	else
	{
		for(int i1 = 0; i1 < octaves; i1++)
		{
			for(int j = 0; j < NOTE_RANGE; j++)
			{
				load_dpcm_sample_map(j, i1 + 1, inst, f, block, inst_num, i);
			}
		}
	}

	return success;
}

bool load_inst_vrc6(FILE* f, ftm_block* block, MusInstrument* inst, Uint8 inst_num)
{
	bool success = true;

	//debug("loading VRC6 instrument");

	load_sequence_indices(f, inst_num);

	return success;
}

bool load_inst_vrc7(FILE* f, ftm_block* block, MusInstrument* inst, Uint8 inst_num)
{
	bool success = true;

	//debug("loading VRC7 instrument");

	Uint32 patch_number = 0;
	read_uint32(f, &patch_number);

	for(int i = 0; i < 8; i++)
	{
		Uint8 custom_patch_param = 0;
		fread(&custom_patch_param, 1, 1, f);

		ft_instruments[inst_num].vrc7_custom_patch[i] = custom_patch_param;
	}

	return success;
}

void ft_load_fds_sequence(FILE* f, Uint8 inst_num, Uint8 seq_type)
{
	Uint8 seq_len = 0;
	fread(&seq_len, 1, 1, f);

	Sint32 loop_point = 0;
	Sint32 release_point = 0;

	read_uint32(f, (Uint32*)&loop_point); //I hope this preserves the -1
	read_uint32(f, (Uint32*)&release_point);

	ft_instruments[inst_num].macros[seq_type].sequence_size = seq_len;
	ft_instruments[inst_num].macros[seq_type].loop = loop_point;
	ft_instruments[inst_num].macros[seq_type].release = release_point;

	Uint32 setting = 0;

	read_uint32(f, &setting);

	ft_instruments[inst_num].macros[seq_type].setting = setting;

	for(int i = 0; i < seq_len; i++)
	{
		Uint8 value = 0;
		fread(&value, 1, 1, f);
		ft_instruments[inst_num].macros[seq_type].sequence[i] = value;
	}
}

bool load_inst_fds(FILE* f, ftm_block* block, MusInstrument* inst, Uint8 inst_num)
{
	bool success = true;

	//debug("loading FDS instrument");

	for (int i = 0; i < FT_FDS_WAVE_SIZE; ++i)
	{
		Uint8 sample = 0;
		fread(&sample, 1, 1, f);
		ft_instruments[inst_num].fds_wave[i] = sample;
	}

	for (int i = 0; i < FT_FDS_MOD_SIZE; ++i)
	{
		Uint8 sample = 0;
		fread(&sample, 1, 1, f);
		ft_instruments[inst_num].fds_mod[i] = sample;
	}

	Uint32 mod = 0;
	read_uint32(f, &mod);
	ft_instruments[inst_num].fds_mod_speed = mod;
	read_uint32(f, &mod);
	ft_instruments[inst_num].fds_mod_depth = mod;
	read_uint32(f, &mod);
	ft_instruments[inst_num].fds_mod_delay = mod;

	Uint32 a = 0;
	Uint32 b = 0;

	read_uint32(f, &a);
	read_uint32(f, &b);

	//pDocFile->RollbackPointer(8);

	fseek(f, -8, SEEK_CUR); //idk bruh it's some hacky mess /shrug
	//all FamiTracker is a hacky mess in some sort, actually
	//and C++ classes only make understanding source code more difficult

	if(a < 256 && (b & 0xFF) != 0x00)
	{

	}

	else
	{
		ft_load_fds_sequence(f, inst_num, FT_SEQ_VOLUME);
		ft_load_fds_sequence(f, inst_num, FT_SEQ_ARPEGGIO);

		ft_instruments[inst_num].seq_enable[FT_SEQ_VOLUME] = true;
		ft_instruments[inst_num].seq_enable[FT_SEQ_ARPEGGIO] = true;

		if(block->version > 2)
		{
			ft_load_fds_sequence(f, inst_num, FT_SEQ_PITCH);
			ft_instruments[inst_num].seq_enable[FT_SEQ_PITCH] = true;
		}
	}

	if(block->version <= 3) //double the volume (0-15 -> 0-31)
	{
		for(int i = 0; i < ft_instruments[inst_num].macros[FT_SEQ_VOLUME].sequence_size; i++)
		{
			ft_instruments[inst_num].macros[FT_SEQ_VOLUME].sequence[i] *= 2;
		}
	}

	return success;
}

bool load_inst_n163(FILE* f, ftm_block* block, MusInstrument* inst, Uint8 inst_num)
{
	bool success = true;

	//debug("loading N163 instrument");

	load_sequence_indices(f, inst_num);

	Uint32 wave_size = 0;
	Uint32 wave_position = 0;
	Uint32 wave_count = 0;

	read_uint32(f, &wave_size);
	read_uint32(f, &wave_position);

	if(block->version >= 8)
	{
		Uint32 autopos = 0;
		read_uint32(f, &autopos);
	}

	read_uint32(f, &wave_count);

	ft_instruments[inst_num].num_n163_samples = wave_count;

	for(int i = 0; i < wave_count; i++)
	{
		for(int j = 0; j < wave_size; j++)
		{
			Uint8 val = 0;
			fread(&val, 1, 1, f);
			ft_instruments[inst_num].n163_samples[i][j] = val;
		}
	}

	ft_instruments[inst_num].n163_samples_len = wave_size;

	return success;
}

bool load_inst_s5b(FILE* f, ftm_block* block, MusInstrument* inst, Uint8 inst_num)
{
	bool success = true;

	//debug("loading S5B instrument");

	load_sequence_indices(f, inst_num);

	return success;
}

void fill_channels_to_chips_array()
{
	Uint8 curr_chan = 0;

	memset(channels_to_chips, 0, FT_MAX_CHANNELS);

	for(int i = 0; i < 5; i++)
	{
		channels_to_chips[curr_chan] = FT_SNDCHIP_NONE; //2A03
		curr_chan++;
	}

	if(expansion_chips & FT_SNDCHIP_VRC6)
	{
		for(int i = 0; i < 3; i++)
		{
			channels_to_chips[curr_chan] = FT_SNDCHIP_VRC6;
			curr_chan++;
		}
	}

	if(expansion_chips & FT_SNDCHIP_MMC5)
	{
		for(int i = 0; i < 2; i++)
		{
			channels_to_chips[curr_chan] = FT_SNDCHIP_MMC5;
			curr_chan++;
		}
	}

	if(expansion_chips & FT_SNDCHIP_N163)
	{
		for(int i = 0; i < namco_channels; i++)
		{
			channels_to_chips[curr_chan] = FT_SNDCHIP_N163;
			curr_chan++;
		}
	}

	if(expansion_chips & FT_SNDCHIP_FDS)
	{
		for(int i = 0; i < 1; i++)
		{
			channels_to_chips[curr_chan] = FT_SNDCHIP_FDS;
			curr_chan++;
		}
	}

	if(expansion_chips & FT_SNDCHIP_VRC7)
	{
		for(int i = 0; i < 6; i++)
		{
			channels_to_chips[curr_chan] = FT_SNDCHIP_VRC7;
			curr_chan++;
		}
	}

	if(expansion_chips & FT_SNDCHIP_S5B)
	{
		for(int i = 0; i < 3; i++)
		{
			channels_to_chips[curr_chan] = FT_SNDCHIP_S5B;
			curr_chan++;
		}
	}
}

bool ft_process_params_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	//debug("processing params block");
	
	Uint32 song_speed = 0;
	Uint32 t_machine = 0;
	Uint32 num_channels = 0;
	expansion_chips = 0;
	namco_channels = 0;
	
	if(block->version == 1)
	{
		read_uint32(f, &song_speed);
		
		mused.song.song_speed = mused.song.song_speed2 = song_speed;
	}
	
	else
	{
		fread(&expansion_chips, 1, 1, f);
	}
	
	read_uint32(f, &num_channels);
	
	mused.song.num_channels = num_channels;
	
	read_uint32(f, &t_machine);

	machine = t_machine;
	
	if(t_machine == FT_MACHINE_NTSC)
	{
		mused.song.song_rate = 60;
	}
	
	else if(t_machine == FT_MACHINE_PAL)
	{
		mused.song.song_rate = 50;
	}
	
	else
	{
		debug("unknown machine type, defaulting song rate to 50 Hz");
		mused.song.song_rate = 50;
	}
	
	if(block->version >= 7)
	{
		Uint32 playback_rate_type = 0;
		read_uint32(f, &playback_rate_type);
		
		Uint32 playback_rate = 0;
		read_uint32(f, &playback_rate);
		
		switch(playback_rate_type)
		{
			case 1:
			{
				// workaround for now
				mused.song.song_rate = my_min(44100, (Uint16)(1000000.0 / playback_rate + 0.5));
				
				if(mused.song.song_rate == 0)
				{
					debug("rate is 0 Hz, defaulting song rate to 50 Hz");
					mused.song.song_rate = 50;
				}
				
				break;
			}
			
			case 0: case 2: default:
			{
				debug("rate is 0 Hz or uncertain, defaulting song rate to 50 Hz");
				mused.song.song_rate = 50;
				break;
			}
		}
	}
	
	else
	{
		Uint32 song_rate = 0;
		read_uint32(f, &song_rate);
		
		mused.song.song_rate = my_min(44100, song_rate);
		
		if(mused.song.song_rate == 0)
		{
			debug("rate is 0 Hz, defaulting song rate to 50 Hz");
			mused.song.song_rate = 50;
		}
	}
	
	if(block->version > 2)
	{
		Uint32 vibrato_style = 0; //unused in klystrack
		read_uint32(f, &vibrato_style);
	}
	
	if(block->version >= 9)
	{
		//bool sweep_reset = false;
		
		Uint32 temp = 0;
		read_uint32(f, &temp);
		
		//sweep_reset = temp != 0 ? true : false; //unused in klystrack
	}
	
	if(block->version > 3 && block->version <= 6)
	{
		Uint32 highlight_first = 0;
		Uint32 highlight_second = 0;
		
		read_uint32(f, &highlight_first);
		read_uint32(f, &highlight_second);
		
		mused.song.time_signature = (((highlight_second / highlight_first) & 0xff) << 8) | ((highlight_first) & 0xff);
		
		debug("seting time signature from params block");
	}
	
	// This is strange. Sometimes expansion chip is set to 0xFF in files
	if(mused.song.num_channels == 5)
	{
		expansion_chips = 0;
	}

	fill_channels_to_chips_array();
	
	if(version == 0x0200)
	{
		if(mused.song.song_speed < 20)
		{
			mused.song.song_speed++;
			mused.song.song_speed2++;
		}
	}
	
	if(block->version == 1)
	{
		if(mused.song.song_speed > 19)
		{
			mused.song.song_speed = mused.song.song_speed2 = 6;
		}
	}
	
	if(block->version >= 5 && (expansion_chips & FT_SNDCHIP_N163))
	{
		Uint32 chans = 0;
		read_uint32(f, &chans);
		namco_channels = chans;
	}
	
	if(block->version >= 6)
	{
		Uint32 speed_split_point = 0;
		read_uint32(f, &speed_split_point); //unused in klystrack; wtf it even is?
	}
	
	if(block->version == 8)
	{
		Sint8 detune_semitone = 0;
		Sint8 detune_cent = 0;
		
		fread(&detune_semitone, 1, sizeof(detune_semitone), f); //unused in klystrack
		fread(&detune_cent, 1, sizeof(detune_cent), f);
	}
	
	return success;
}

bool ft_process_tuning_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	if(block->version == 1)
	{
		Sint8 detune_semitone = 0;
		Sint8 detune_cent = 0;
		
		fread(&detune_semitone, 1, sizeof(detune_semitone), f); //unused in klystrack
		fread(&detune_cent, 1, sizeof(detune_cent), f);
	}
	
	return success;
}

bool ft_process_info_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	char name[33] = { 0 };
	char author[33] = { 0 };
	char copyright[33] = { 0 };
	
	fread(name, 1, 32, f);
	fread(author, 1, 32, f);
	fread(copyright, 1, 32, f);
	
	snprintf(mused.song.title, sizeof(mused.song.title), "%s. Author: %s. Copyright: %s", name, author, copyright);
	
	return success;
}

bool ft_process_instruments_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	Uint32 num_insts = 0;
	
	read_uint32(f, &num_insts);
	
	if(num_insts > FT_MAX_INSTRUMENTS)
	{
		debug("Too many instruments!");
		success = false;
		goto end;
	}

	num_instruments = num_insts;

	//debug("song has %d instruments", num_insts);

	for(int i = 0; i < num_insts; i++)
	{
		Uint32 inst_index = 0;
		read_uint32(f, &inst_index);
		
		Uint8 inst_type = 0;
		fread(&inst_type, sizeof(inst_type), 1, f);

		inst_numbers_to_inst_indices[i][0] = i;
		inst_numbers_to_inst_indices[i][1] = inst_index; //so we can remap inst numbers in patterns later

		ft_instruments[inst_index].klystrack_instrument = i;

		ft_instruments[inst_index].type = inst_type;
		
		//load instrument

		//debug("instrument type %d", inst_type);
		
		switch(inst_type)
		{
			case INST_2A03:
			{
				success = load_inst_2a03(f, block, &mused.song.instrument[i], inst_index, i);
				break;
			}

			case INST_VRC6:
			{
				success = load_inst_vrc6(f, block, &mused.song.instrument[i], inst_index);
				break;
			}
			
			case INST_VRC7:
			{
				success = load_inst_vrc7(f, block, &mused.song.instrument[i], inst_index);
				break;
			}
			
			case INST_FDS:
			{
				success = load_inst_fds(f, block, &mused.song.instrument[i], inst_index);
				break;
			}
			
			case INST_N163:
			{
				success = load_inst_n163(f, block, &mused.song.instrument[i], inst_index);
				break;
			}
			
			case INST_S5B:
			{
				success = load_inst_s5b(f, block, &mused.song.instrument[i], inst_index);
				break;
			}
			
			default: break;
		}
		
		Uint32 inst_name_len = 0;
		read_uint32(f, &inst_name_len);
		
		//debug("inst name len %d", inst_name_len);

		fread(mused.song.instrument[i].name, inst_name_len, 1, f);
	}
	
	end:;
	
	return success;
}

bool ft_process_sequences_block(FILE* f, ftm_block* block)
{
	bool success = true;

	//debug("Processing 2A03 sequences");

	Uint32 seq_count = 0;

	read_uint32(f, &seq_count);

	ft_inst_macro* temp_macro = (ft_inst_macro*)calloc(1, sizeof(ft_inst_macro));

	if(block->version == 1)
	{
		for(int i = 0; i < seq_count; i++)
		{
			Uint32 index = 0;
			read_uint32(f, &index);

			Uint8 size = 0;
			fread(&size, sizeof(size), 1, f);

			memset(temp_macro, 0, sizeof(ft_inst_macro));

			temp_macro->loop = -1;
			temp_macro->release = -1;

			temp_macro->sequence_size = size;
			temp_macro->setting = 0;

			for(int j = 0; j < size; j++)
			{
				fread(&temp_macro->sequence[j], sizeof(Uint8), 1, f);

				Uint8 pos = 0;
				fread(&pos, sizeof(pos), 1, f); //unused here but kept for proper alignment
			}

			for(int j = 0; j < num_instruments; j++)
			{
				if(ft_instruments[j].seq_indices[0] == index && ft_instruments[j].type == INST_2A03) //idk bruh no macro type is provided in version 1 so maybe it's volume?
				{
					//TODO: reorder sequences or whatever shit
					memcpy(&ft_instruments[j].macros[0], temp_macro, sizeof(ft_inst_macro));
				}
			}
		}
	}

	if(block->version == 2)
	{
		for(int i = 0; i < seq_count; i++)
		{
			Uint32 index = 0;
			read_uint32(f, &index);

			Uint32 type = 0;
			read_uint32(f, &type);

			Uint8 size = 0;
			fread(&size, sizeof(size), 1, f);

			memset(temp_macro, 0, sizeof(ft_inst_macro));

			temp_macro->loop = -1;
			temp_macro->release = -1;

			temp_macro->sequence_size = size;
			temp_macro->setting = 0;

			for(int j = 0; j < size; j++)
			{
				fread(&temp_macro->sequence[j], sizeof(Uint8), 1, f);

				Uint8 pos = 0;
				fread(&pos, sizeof(pos), 1, f); //unused here but kept for proper alignment
			}

			for(int j = 0; j < num_instruments; j++)
			{
				if(ft_instruments[j].seq_indices[type] == index && ft_instruments[j].type == INST_2A03) //finally we were blessed with sequence type!
				{
					memcpy(&ft_instruments[j].macros[type], temp_macro, sizeof(ft_inst_macro));
				}
			}
		}
	}

	if(block->version >= 3)
	{
		Uint8* Indices = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_SEQUENCES * FT_SEQ_CNT);
		Uint8* Types = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_SEQUENCES * FT_SEQ_CNT);

		for(int i = 0; i < seq_count; i++)
		{
			Uint32 index = 0;
			read_uint32(f, &index);
			Indices[i] = index;

			Uint32 type = 0;
			read_uint32(f, &type);
			Types[i] = type;

			Uint8 size = 0;
			fread(&size, sizeof(size), 1, f);

			memset(temp_macro, 0, sizeof(ft_inst_macro));

			temp_macro->loop = -1;
			temp_macro->release = -1;

			temp_macro->sequence_size = size;
			temp_macro->setting = 0;

			Sint32 loop_point = 0;
			Sint32 release_point = 0;

			Uint32 setting = 0;

			read_uint32(f, (Uint32*)&loop_point);

			temp_macro->loop = loop_point;

			if(block->version == 4)
			{
				read_uint32(f, (Uint32*)&release_point);
				read_uint32(f, &setting);

				temp_macro->release = release_point;
				temp_macro->setting = setting;
			}

			for(int j = 0; j < size; j++)
			{
				fread(&temp_macro->sequence[j], sizeof(Uint8), 1, f);
			}

			//debug("num instruments %d", num_instruments);

			for(int j = 0; j < FT_MAX_INSTRUMENTS; j++)
			{
				if(ft_instruments[j].seq_indices[type] == index && ft_instruments[j].type == INST_2A03)
				{
					memcpy(&ft_instruments[j].macros[type], temp_macro, sizeof(ft_inst_macro));

					if(ft_instruments[j].seq_enable[type])
					{
						//debug("Macro %d type %d instrument %d klystrack instrument %d", Indices[i], Types[i], j, ft_instruments[j].klystrack_instrument);

						/*for(int k = 0; k < ft_instruments[j].macros[type].sequence_size; k++)
						{
							debug("%d", ft_instruments[j].macros[type].sequence[k]);
						}*/
					}
				}
			}
		}

		if(block->version == 5) // Version 5 saved the release points incorrectly, this is fixed in ver 6
		{
			for(int i = 0; i < FT_MAX_SEQUENCES; i++)
			{
				for(int j = 0; j < FT_SEQ_CNT; j++)
				{
					Sint32 release_point = 0;
					Uint32 setting = 0;
					read_uint32(f, (Uint32*)&release_point);
					read_uint32(f, &setting);

					for(int k = 0; k < FT_MAX_INSTRUMENTS; k++)
					{
						if(ft_instruments[k].seq_indices[j] == i && ft_instruments[k].type == INST_2A03)
						{
							ft_instruments[k].macros[j].release = release_point;
							ft_instruments[k].macros[j].setting = setting;
						}
					}
				}
			}
		}

		if(block->version >= 6) // Read release points correctly stored
		{
			for(int i = 0; i < seq_count; i++)
			{
				Sint32 release_point = 0;
				Uint32 setting = 0;
				read_uint32(f, (Uint32*)&release_point);
				read_uint32(f, &setting);

				for(int j = 0; j < FT_MAX_INSTRUMENTS; j++)
				{
					if(ft_instruments[j].seq_indices[Types[i]] == Indices[i] && ft_instruments[j].type == INST_2A03)
					{
						ft_instruments[j].macros[Types[i]].release = release_point;
						ft_instruments[j].macros[Types[i]].setting = setting;
					}
				}
			}
		}

		free(Indices);
		free(Types);
	}

	free(temp_macro);
	
	return success;
}

bool ft_process_frames_block(FILE* f, ftm_block* block)
{
	bool success = true;

	//debug("Processing song pattern orders");

	if(block->version == 1)
	{
		Uint32 seq_len = 0;
		Uint32 num_ch = 0;

		read_uint32(f, &seq_len);
		read_uint32(f, &num_ch);

		sequence_length = seq_len;

		mused.song.num_channels = num_ch; // maybe in version 1 there isn't params block and we read it from here?

		for(int i = 0; i < seq_len; i++)
		{
			for(int j = 0; j < num_ch; j++)
			{
				Uint8 pattern = 0;
				fread(&pattern, sizeof(pattern), 1, f);

				ft_sequence[i][j] = pattern;
			}
		}
	}

	if(block->version >= 1)
	{
		for(int i = 0; i < num_subsongs; i++)
		{
			debug("Subsong %d", i + 1);

			Uint32 seq_len = 0;
			Uint32 speed = 0;

			read_uint32(f, &seq_len);
			read_uint32(f, &speed);

			if(i == selected_subsong)
			{
				sequence_length = seq_len;
			}

			if(block->version >= 3)
			{
				Uint32 tempo = 0;

				read_uint32(f, &tempo);

				if(i == selected_subsong)
				{
					mused.song.song_rate = (Uint32)tempo * 50 / 125;

					mused.song.song_speed = mused.song.song_speed2 = speed;

					if(mused.song.song_rate == 0)
					{
						if(machine == FT_MACHINE_NTSC)
						{
							mused.song.song_rate = 60;
						}
						
						else
						{
							mused.song.song_rate = 50;
						}
					}
				}
			}

			else
			{
				if(i == selected_subsong)
				{
					if(speed < 20)
					{
						mused.song.song_rate = ((machine == FT_MACHINE_NTSC) ? 60 : 50);
						mused.song.song_speed = mused.song.song_speed2 = speed;

						if(mused.song.song_rate == 0)
						{
							if(machine == FT_MACHINE_NTSC)
							{
								mused.song.song_rate = 60;
							}
							
							else
							{
								mused.song.song_rate = 50;
							}
						}
					}

					else
					{
						mused.song.song_speed = mused.song.song_speed2 = 6;
						//mused.song.song_rate = 50;

						mused.song.song_rate = (Uint32)speed * 50 / 125;

						if(mused.song.song_rate == 0)
						{
							if(machine == FT_MACHINE_NTSC)
							{
								mused.song.song_rate = 60;
							}
							
							else
							{
								mused.song.song_rate = 50;
							}
						}
					}
				}
			}

			Uint32 pat_len = 0;
			read_uint32(f, &pat_len);

			if(i == selected_subsong)
			{
				pattern_length = pat_len;
			}

			for(int j = 0; j < seq_len; j++)
			{
				for(int k = 0; k < mused.song.num_channels; k++)
				{
					Uint8 pattern = 0;
					fread(&pattern, sizeof(pattern), 1, f);

					if(i == selected_subsong)
					{
						ft_sequence[j][k] = pattern;
					}
				}
			}
		}
	}
	
	return success;
}

void ft_convert_command(Uint16 val, MusStep* step, Uint16 song_pos, Uint8 channel, Uint8 column, Uint8 row)
{
	Uint16 param = (val & 0xff);

	switch(val >> 8)
	{
		case FT_EF_SPEED:
		{
			if(param < 0x20)
			{
				step->command[column] = MUS_FX_SET_SPEED | param;
			}

			else
			{
				step->command[column] = MUS_FX_SET_RATE | (param * 50 / 125);
			}

			break;
		}

		case FT_EF_JUMP:
		{
			step->command[column] = MUS_FX_JUMP_SEQUENCE_POSITION | param;
			break;
		}

		case FT_EF_SKIP:
		{
			step->command[column] = MUS_FX_SKIP_PATTERN | param;
			break;
		}

		case FT_EF_HALT:
		{
			//debug("song_pos %d", song_pos);
			mused.song.song_length = song_pos; //song will hang there
			mused.song.loop_point = song_pos;
			break;
		}

		case FT_EF_VOLUME: //can't find anything about it lol
		{
			step->command[column] = MUS_FX_SET_VOLUME | param;
			break;
		}

		case FT_EF_PORTAMENTO:
		{
			step->command[column] = MUS_FX_FAST_SLIDE | ((param == 1) ? param : (param / 2));
			//step->command[MUS_MAX_COMMANDS - 1] = CONV_MARKER;
			break;
		}

		case FT_EF_SWEEPUP:
		{
			//step->command[column] = MUS_FX_PORTA_UP | (((param & 0x0f) << 4) / my_max(1, ((param & 0xf0) >> 4))); //TODO: more accurate conversion based on period and value
			step->command[column] = MUS_FX_PORTA_UP | ((0xf0 - ((param & 0x0f) << 4)) | ((param & 0xf0) >> 4));

			step->command[MUS_MAX_COMMANDS - 1] = CONV_MARKER_SWEEP;
			break;
		}

		case FT_EF_SWEEPDOWN:
		{
			step->command[column] = MUS_FX_PORTA_DN | ((0xf0 - ((param & 0x0f) << 4)) | ((param & 0xf0) >> 4));
			step->command[MUS_MAX_COMMANDS - 1] = CONV_MARKER_SWEEP;
			break;
		}

		case FT_EF_ARPEGGIO:
		{
			step->command[column] = MUS_FX_SET_EXT_ARP | param; //so 1000 clears the notes and stops arp
			break;
		}

		case FT_EF_VIBRATO:
		{
			if(param)
			{
				step->command[column] = MUS_FX_VIBRATO | (my_max(1, (((param & 0xf0) >> 4) / 2)) << 4) | my_max(1, ((param & 0xf) / 2));
			}

			else
			{
				step->command[column] = MUS_FX_VIBRATO;
			}

			break;
		}

		case FT_EF_TREMOLO:
		{
			step->command[column] = MUS_FX_TREMOLO | param;
			break;
		}

		case FT_EF_PITCH:
		{
			step->command[column] = MUS_FX_PITCH | param;
			break;
		}

		case FT_EF_DELAY:
		{
			step->command[column] = MUS_FX_NOTE_DELAY_EXTENDED | param;
			break;
		}

		case FT_EF_DAC:
		{
			return; //bruh
		}

		case FT_EF_PORTA_UP:
		{
			step->command[column] = MUS_FX_PORTA_UP | param; //TODO: finetune speed
			step->command[MUS_MAX_COMMANDS - 1] = CONV_MARKER;
			break;
		}

		case FT_EF_PORTA_DOWN:
		{
			step->command[column] = MUS_FX_PORTA_DN | param; //TODO: finetune speed
			step->command[MUS_MAX_COMMANDS - 1] = CONV_MARKER;
			break;
		}

		case FT_EF_TRANSPOSE:
		{
			step->command[column] = MUS_FX_DELAYED_TRANSPOSE | param;
			break;
		}

		case FT_EF_DUTY_CYCLE:
		{
			if(channel == 1 || channel == 2 || channels_to_chips[channel] == FT_SNDCHIP_MMC5) // 2A03/MMC5 pulse widths
			{
				step->command[column] = MUS_FX_PW_SET | ft_2a03_pulse_widths[param];
			}

			if(channel == 4) //noise mode
			{
				if(param)
				{
					step->command[column] = MUS_FX_EXT_SET_NOISE_MODE | 0b11; //1-bit & metal
				}

				else
				{
					step->command[column] = MUS_FX_EXT_SET_NOISE_MODE | 0b01; //metal only
				}
			}

			if(channels_to_chips[channel] == FT_SNDCHIP_VRC6)
			{
				step->command[column] = MUS_FX_PW_SET | ft_vrc6_pulse_widths[param];
			}

			if(channels_to_chips[channel] == FT_SNDCHIP_N163)
			{
				step->command[column] = MUS_FX_SET_LOCAL_SAMPLE | param;
			}

			break;
		}

		case FT_EF_SUNSOFT_ENV_TYPE:
		{
			if(channels_to_chips[channel] == FT_SNDCHIP_S5B)
			{
				Uint8 type = param & 0xf;

				if(type == 8)
				{
					step->command[column] = MUS_FX_BUZZ_SHAPE | 1;
				}

				if(type == 0xC)
				{
					step->command[column] = MUS_FX_BUZZ_SHAPE | 0;
				}

				if(type == 0xA || type == 0xE)
				{
					step->command[column] = MUS_FX_BUZZ_SHAPE | 2;
				}
			}

			break;
		}

		case FT_EF_SUNSOFT_NOISE:
		{
			if(channels_to_chips[channel] == FT_SNDCHIP_S5B)
			{
				step->command[column] = MUS_FX_SET_NOISE_CONSTANT_PITCH | s5b_noise_notes[param & 0x1f];
			}

			break;
		}

		case FT_EF_SAMPLE_OFFSET:
		{
			step->command[column] = MUS_FX_WAVETABLE_OFFSET | (param << 4); //TODO: add actual sample offset command later when we certainly know sample length
			break;
		}

		case FT_EF_SLIDE_UP:
		{
			step->command[column] = MUS_FX_SLIDE_UP_SEMITONES | param;

			if((param & 0xf0) == 0) //so speed 0 is slowest speed and still does the slide (as it works in famitracker for some reason)
			{
				step->command[column] |= 1 << 4;
			}

			break;
		}

		case FT_EF_SLIDE_DOWN:
		{
			step->command[column] = MUS_FX_SLIDE_DN_SEMITONES | param;

			if((param & 0xf0) == 0)
			{
				step->command[column] |= 1 << 4;
			}

			break;
		}

		case FT_EF_VOLUME_SLIDE:
		{
			step->command[column] = MUS_FX_FADE_VOLUME | param;
			break;
		}

		case FT_EF_NOTE_CUT:
		{
			step->command[column] = MUS_FX_NOTE_CUT_EXTENDED | param;
			break;
		}

		case FT_EF_RETRIGGER:
		{
			step->command[column] = MUS_FX_RETRIGGER_EXTENDED | param;
			break;
		}

		case FT_EF_DPCM_PITCH:
		{
			if(channels_to_chips[channel] == FT_SNDCHIP_S5B)
			{
				step->command[column] = MUS_FX_SET_NOISE_CONSTANT_PITCH | s5b_noise_notes[param]; //TODO: add separate noise notes set by ear for AY (S5B) noise
			}

			break;
		}

		case FT_EF_NOTE_RELEASE:
		{
			step->command[column] = MUS_FX_TRIGGER_RELEASE | param;
			break;
		}

		case FT_EF_PHASE_RESET:
		{
			step->command[column] = MUS_FX_PHASE_RESET | (param & 0xf);
			break;
		}

		case FT_EF_GROOVE:
		{
			step->command[column] = MUS_FX_SET_GROOVE | param;
			break;
		}

		default: break; //TODO: add at least smth for all the other effects, at least return 0 thing
	}
}

bool ft_process_patterns_block(FILE* f, ftm_block* block)
{
	bool success = true;

	if(block->version == 1)
	{
		Uint32 pat_len = 0;
		read_uint32(f, &pat_len);
		pattern_length = pat_len;
	}

	if(block->version < 5)
	{
		transpose_fds = true;
	}

	//here we process the sequence block actually
	//since we don't have the pattern length from early version block above
	//and thus can't populate klystrack sequences properly

	Uint16 ref_pattern = 0;
	Uint16 max_pattern = 0; //these are used to increment pattern number when we go to next channel since klystrack pattern indices are global
	//so we don't have intersecting sequences
	//hope 4096 unique patterns are enough for any Famitracker song

	for(int i = 0; i < mused.song.num_channels; i++)
	{
		max_pattern = 0;

		for(int j = 0; j < sequence_length; j++)
		{
			add_sequence(i, j * pattern_length, i * sequence_length + j, 0);
			resize_pattern(&mused.song.pattern[i * sequence_length + j], pattern_length);

			klystrack_sequence[j][i] = i * sequence_length + j; //ft_sequence[j][i] + ref_pattern;

			if(max_pattern < ft_sequence[j][i])
			{
				max_pattern = ft_sequence[j][i];
			}
		}

		/*if(max_pattern == 0)
		{
			max_pattern = 1; //so we don't have one pattern across multiple channels if one of the channels in sequence has just a bunch of `00` patterns
		}*/

		ref_pattern += max_pattern + 1;
	}

	mused.song.song_length = sequence_length * pattern_length;
	mused.sequenceview_steps = pattern_length;
	mused.song.sequence_step = pattern_length;

	Uint16 song_pos = 0;

	while(ftell(f) < block->position + block->length) // while (!pDocFile->BlockDone())
	{
		//bruh famitracker jank
		//can't fucking store number of patterns so go the hard way lol
		//remember if import crashes it's the famitracker jank fault most probably
		//or you have broken/corrupted/invalid file

		Uint32 subsong_index = 0;

		if(block->version > 1)
		{
			read_uint32(f, &subsong_index);
		}

		Uint32 channel = 0;
		Uint32 pattern = 0;
		Uint32 len = 0;

		read_uint32(f, &channel);
		read_uint32(f, &pattern);
		read_uint32(f, &len);

		//debug("subsong %d channel %d pattern %d pattern length %d, len %d", subsong_index, channel, pattern, pattern_length, len);

		//debug("channel %d pattern %d len %d", channel, pattern, len);

		MusPattern* pat = &mused.song.pattern[0];

		if(subsong_index == selected_subsong)
		{
			for(int j = 0; j < sequence_length; j++)
			{
				if(ft_sequence[j][channel] == pattern)
				{
					pat = &mused.song.pattern[klystrack_sequence[j][channel]];
					song_pos = mused.song.sequence[channel][j].position;

					goto proceed;
				}
			}
		}

		proceed:;

		for(int i = 0; i < len; i++)
		{
			Uint32 row = 0;

			if(block->version >= 6 || version == 0x0200)
			{
				Uint8 row_8 = 0;
				fread(&row_8, sizeof(row_8), 1, f);

				row = row_8;
			}

			else
			{
				read_uint32(f, &row);
			}

			Uint8 note = 0;
			Uint8 octave = 0;
			Uint8 inst = 0;
			Uint8 vol = 0;
			Uint8 effect[FT_MAX_EFFECT_COLUMNS] = { 0 };
			Uint8 param[FT_MAX_EFFECT_COLUMNS] = { 0 };

			fread(&note, sizeof(note), 1, f);
			fread(&octave, sizeof(octave), 1, f);
			fread(&inst, sizeof(inst), 1, f);
			fread(&vol, sizeof(vol), 1, f);

			if(subsong_index == selected_subsong)
			{
				if(note == FT_NOTE_NONE)
				{
					pat->step[row].note = MUS_NOTE_NONE;
				}

				else if (note == FT_NOTE_RELEASE)
				{
					pat->step[row].note = MUS_NOTE_RELEASE;
				}
				
				else if (note == FT_NOTE_CUT)
				{
					pat->step[row].note = MUS_NOTE_CUT;
				}

				else
				{
					pat->step[row].note = (note - 1) + (octave + 1) * 12 + C_ZERO; //C-0 in FT is C-1 in klys

					if(channel == 2)
					{
						pat->step[row].note -= 12; //looks like it's the only way to make triangle sound properly?
						//because in macros we still use absolute values hmm
					}

					if(channel == 3) //noise channel
					{
						pat->step[row].note = noise_notes[(note + octave * 12) - (12 + 5)];
					}
				}

				//=========================================================

				if(inst == FT_INSTRUMENT_NONE)
				{
					pat->step[row].instrument = MUS_NOTE_NO_INSTRUMENT;
				}

				else
				{
					pat->step[row].instrument = inst;
				}

				if(note == FT_NOTE_CUT || note == FT_NOTE_RELEASE)
				{
					pat->step[row].instrument = MUS_NOTE_NO_INSTRUMENT;
				}

				if(inst >= FT_MAX_INSTRUMENTS)
				{
					pat->step[row].instrument = MUS_NOTE_NO_INSTRUMENT;
				}

				for(int h = 0; h < FT_MAX_INSTRUMENTS; h++)
				{
					if(pat->step[row].instrument == inst_numbers_to_inst_indices[h][1])
					{
						pat->step[row].instrument = inst_numbers_to_inst_indices[h][0];
						break;
					}
				}
			}

			//=========================================================

			Uint8 num_fx = ((version == 0x0200) ? 1 : ((block->version >= 6) ? FT_MAX_EFFECT_COLUMNS : (effect_columns[subsong_index][channel] + 1)));

			//debug("num fx %d", num_fx);

			if(subsong_index == selected_subsong)
			{
				mused.command_columns[channel] = effect_columns[subsong_index][channel];
			}

			for(int j = 0; j < num_fx; j++)
			{
				fread(&effect[j], sizeof(effect[0]), 1, f);

				if(effect[j])
				{
					fread(&param[j], sizeof(param[0]), 1, f);

					if(block->version < 3)
					{
						if(effect[j] == FT_EF_PORTAOFF)
						{
							effect[j] = FT_EF_PORTAMENTO;
							param[j] = 0;
						}

						else if(effect[j] == FT_EF_PORTAMENTO)
						{
							if(param[j] < 0xFF)
							{
								param[j]++;
							}
						}
					}
				}

				else if(block->version < 6)
				{
					Uint8 unused = 0;
					fread(&unused, sizeof(unused), 1, f); // unused blank parameter
					//bruh then how do you have the actual effect?!
				}

				if(version == 0x0200)
				{
					if(j == 0 && effect[j] == FT_EF_SPEED && param[j] < 20)
					{
						param[j]++;
					}
				}

				//pat->step[row].command[j] = ((Uint16)effect << 8) + param;
			}

			if(subsong_index == selected_subsong)
			{
				if(channel != 2 && channel != 4) //no volume control on tri and DPCM
				{
					if(vol == FT_VOLUME_NONE)
					{
						pat->step[row].volume = MUS_NOTE_NO_VOLUME;
					}

					else
					{
						pat->step[row].volume = convert_volumes_16[vol];
					}

					if(version == 0x0200)
					{
						if(vol == 0)
						{
							pat->step[row].volume = MUS_NOTE_NO_VOLUME;
						}

						else
						{
							vol--;
							vol &= 0xf;
							pat->step[row].volume = convert_volumes_16[vol];
						}
					}
				}

				if((expansion_chips & FT_SNDCHIP_N163) && channels_to_chips[channel] == FT_SNDCHIP_N163)
				{
					for(int j = 0; j < FT_MAX_EFFECT_COLUMNS; j++)
					{
						if(effect[j] == FT_EF_SAMPLE_OFFSET)
						{
							effect[j] = FT_EF_N163_WAVE_BUFFER;
						}
					}
				}

				if(block->version == 3)
				{
					if((expansion_chips & FT_SNDCHIP_VRC7) && channel > 4) // Fix for VRC7 portamento
					{
						for(int j = 0; j < FT_MAX_EFFECT_COLUMNS; j++)
						{
							if(effect[j] == FT_EF_PORTA_DOWN)
							{
								effect[j] = FT_EF_PORTA_UP;
							}

							if(effect[j] == FT_EF_PORTA_UP)
							{
								effect[j] = FT_EF_PORTA_DOWN;
							}
						}
					}

					else if((expansion_chips & FT_SNDCHIP_FDS) && channels_to_chips[channel] == FT_SNDCHIP_FDS) // FDS pitch effect fix
					{
						for(int j = 0; j < FT_MAX_EFFECT_COLUMNS; j++)
						{
							if(effect[j] == FT_EF_PITCH)
							{
								if(param[j] != 0x80)
								{
									param[j] = (0x100 - param[j]);
								}
							}
						}
					}
				}

				if(version < 0x0450 || is_dn_fami_module)
				{
					for(int j = 0; j < FT_MAX_EFFECT_COLUMNS; j++)
					{
						Uint8 idx = 0;

						while(eff_conversion_050[idx][0] != 0xFF) //until the end of the array
						{
							if(effect[j] == eff_conversion_050[idx][0]) //remap the effects (0CC vs FT effects type order) bruh idk why but it should be done to correctly read some modules
							{
								effect[j] = eff_conversion_050[idx][1];

								goto stop;
							}

							idx++;
						}

						stop:;
					}
				}

				if(subsong_index == selected_subsong && effect != FT_EF_NONE)
				{
					for(int j = 0; j < FT_MAX_EFFECT_COLUMNS; j++)
					{
						ft_convert_command(((Uint16)effect[j] << 8) + param[j], &pat->step[row], song_pos + row, channel, j, row);
					}
				}
			}
		}
	
		if(subsong_index == selected_subsong)
		{
			for(int j = 0; j < sequence_length; j++)
			{
				if(ft_sequence[j][channel] == pattern && &mused.song.pattern[klystrack_sequence[j][channel]] != pat)
				{
					//pat = &mused.song.pattern[klystrack_sequence[j][channel]];
					MusPattern* dst_pat = &mused.song.pattern[klystrack_sequence[j][channel]];
					
					memcpy(dst_pat->step, pat->step, sizeof(MusStep) * pat->num_steps);
				}
			}
		}
	}
	
	return success;
}

bool ft_process_dpcm_samples_block(FILE* f, ftm_block* block)
{
	bool success = true;

	Uint8 num_samples = 0;

	fread(&num_samples, sizeof(num_samples), 1, f);

	for(int i = 0; i < num_samples; i++)
	{
		Uint8 index = 0;
		fread(&index, sizeof(index), 1, f);

		Uint32 name_len = 0;
		read_uint32(f, &name_len);
		fread(mused.song.wavetable_names[index], sizeof(char) * name_len, 1, f);

		Uint32 sample_len = 0;

		read_uint32(f, &sample_len);

		Uint32 true_size = sample_len + ((1 - (Sint32)sample_len) & 0x0f);

		Uint8* dpcm_data = (Uint8*)calloc(true_size, sizeof(Uint8));

		memset(dpcm_data, 0xAA, true_size);

		fread(dpcm_data, sizeof(Uint8) * sample_len, 1, f);

		Sint16* data = (Sint16*)calloc(true_size * 8, sizeof(Sint16));

		Sint16 delta_counter = 1;

		for(int i = 0; i < true_size * 8; i++)
		{
			if((dpcm_data[i / 8] & (1 << (i & 7))) && delta_counter < 63)
			{
				delta_counter++;
			}

			else
			{
				if(delta_counter > 0)
				{
					delta_counter--;
				}
			}

			data[i] = delta_counter * 4 * 256 - 32768;
		}

		cyd_wave_entry_init(&mused.cyd.wavetable_entries[index], data, true_size * 8, CYD_WAVE_TYPE_SINT16, 1, 1, 1);

		mused.cyd.wavetable_entries[index].sample_rate = (machine == FT_MACHINE_PAL ? dpcm_sample_rates_pal[15] : dpcm_sample_rates_ntsc[15]);
		mused.cyd.wavetable_entries[index].base_note = MIDDLE_C << 8;

		free(data);
		free(dpcm_data);
	}

	return success;
}

bool ft_process_header_block(FILE* f, ftm_block* block)
{
	bool success = true;

	if(block->version == 1)
	{
		for(int i = 0; i < mused.song.num_channels; i++)
		{
			Uint8 channel_type = 0; // unused lol why FT has so many crap left, it leaves even the klystrack behind, hey, I am personally insulted
			Uint8 effect_cols = 0;

			fread(&channel_type, sizeof(channel_type), 1, f);
			fread(&effect_cols, sizeof(effect_cols), 1, f);

			effect_columns[0][i] = effect_cols;
		}
	}

	if(block->version >= 2)
	{
		fread(&num_subsongs, sizeof(num_subsongs), 1, f);
		num_subsongs++;

		if(block->version >= 3)
		{
			for(int i = 0; i < num_subsongs; i++) //track names, fuck it, reading just to stay aligned
			{
				//pDocFile->ReadString())
				Uint8 charaa = 1;

				Uint8 sym = 0;

				do //hope this works as null-terminated string reader mockup
				{
					fread(&charaa, sizeof(charaa), 1, f);

					subsong_names[i][sym] = charaa;
					sym++;

				} while(charaa != 0);
			}
		}

		if(num_subsongs > 1)
		{
			selected_subsong = msgbox_select_subsong(domain, mused.slider_bevel, &mused.largefont, "Select subsong:");
		}

		for(int i = 0; i < mused.song.num_channels; i++)
		{
			Uint8 channel_type = 0;
			fread(&channel_type, sizeof(channel_type), 1, f);

			for(int j = 0; j < num_subsongs; j++)
			{
				Uint8 effect_cols = 0;
				fread(&effect_cols, sizeof(effect_cols), 1, f);

				effect_columns[j][i] = effect_cols;
			}
		}

		if(block->version >= 4)
		{
			Uint8 highlight_first = 0;
			Uint8 highlight_second = 0;
			
			fread(&highlight_first, sizeof(highlight_first), 1, f);
			fread(&highlight_second, sizeof(highlight_second), 1, f);
			
			mused.song.time_signature = ((((Uint32)highlight_second / (Uint32)highlight_first) & 0xff) << 8) | (((Uint32)highlight_first) & 0xff);
			
			debug("seting time signature from header block");
		}
	}
	
	return success;
}

bool ft_process_comments_block(FILE* f, ftm_block* block)
{
	bool success = true;

	display_comment = 0;
	read_uint32(f, &display_comment);

	char* comments_string = (char*)calloc(1, 1);
	Uint32 index = 0;
	Uint8 charaa = 1;

	do //hope this works as null-terminated string reader
	{
		//debug("ff");
		fread(&charaa, sizeof(char), 1, f);
		comments_string[index] = charaa;
		index++;
		comments_string = (char*)realloc(comments_string, index + 1);
	} while(charaa != 0);

	if(mused.song.song_message)
	{
		free(mused.song.song_message);
	}

	mused.song.song_message = (char*)calloc(1, sizeof(char) * (index + 1));
	memcpy(mused.song.song_message, comments_string, index);
	mused.song.song_message[index] = '\0';

	mused.song.song_message_length = strlen(mused.song.song_message) + 1;

	free(comments_string);
	
	return success;
}

bool ft_process_sequences_vrc6_block(FILE* f, ftm_block* block)
{
	bool success = true;

	//debug("Processing 2A03 sequences");

	Uint32 seq_count = 0;

	read_uint32(f, &seq_count);

	ft_inst_macro* temp_macro = (ft_inst_macro*)calloc(1, sizeof(ft_inst_macro));

	Uint8* Indices = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_SEQUENCES * FT_SEQ_CNT);
	Uint8* Types = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_SEQUENCES * FT_SEQ_CNT);

	for(int i = 0; i < seq_count; i++)
	{
		Uint32 index = 0;
		read_uint32(f, &index);
		Indices[i] = index;

		Uint32 type = 0;
		read_uint32(f, &type);
		Types[i] = type;

		Uint8 size = 0;
		fread(&size, sizeof(size), 1, f);

		memset(temp_macro, 0, sizeof(ft_inst_macro));

		temp_macro->loop = -1;
		temp_macro->release = -1;

		temp_macro->sequence_size = size;
		temp_macro->setting = 0;

		Sint32 loop_point = 0;
		Sint32 release_point = 0;

		Uint32 setting = 0;

		read_uint32(f, (Uint32*)&loop_point);

		temp_macro->loop = loop_point;

		if(block->version == 4)
		{
			read_uint32(f, (Uint32*)&release_point);
			read_uint32(f, &setting);

			temp_macro->release = release_point;
			temp_macro->setting = setting;
		}

		for(int j = 0; j < size; j++)
		{
			fread(&temp_macro->sequence[j], sizeof(Uint8), 1, f);
		}

		for(int j = 0; j < FT_MAX_INSTRUMENTS; j++)
		{
			if(ft_instruments[j].seq_indices[type] == index && ft_instruments[j].type == INST_VRC6)
			{
				memcpy(&ft_instruments[j].macros[type], temp_macro, sizeof(ft_inst_macro));

				/*debug("VRC6 Macro %d type %d", Indices[i], Types[i]);

				for(int k = 0; k < ft_instruments[j].macros[type].sequence_size; k++)
				{
					debug("%d", ft_instruments[j].macros[type].sequence[k]);
				}*/
			}
		}
	}

	if(block->version == 5) // Version 5 saved the release points incorrectly, this is fixed in ver 6
	{
		for(int i = 0; i < FT_MAX_SEQUENCES; i++)
		{
			for(int j = 0; j < FT_SEQ_CNT; j++)
			{
				Sint32 release_point = 0;
				Uint32 setting = 0;
				read_uint32(f, (Uint32*)&release_point);
				read_uint32(f, &setting);

				for(int k = 0; k < FT_MAX_INSTRUMENTS; k++)
				{
					if(ft_instruments[k].seq_indices[j] == i && ft_instruments[k].type == INST_VRC6)
					{
						ft_instruments[k].macros[j].release = release_point;
						ft_instruments[k].macros[j].setting = setting;
					}
				}
			}
		}
	}

	if(block->version >= 6) // Read release points correctly stored
	{
		for(int i = 0; i < seq_count; i++)
		{
			Sint32 release_point = 0;
			Uint32 setting = 0;
			read_uint32(f, (Uint32*)&release_point);
			read_uint32(f, &setting);

			for(int j = 0; j < FT_MAX_INSTRUMENTS; j++)
			{
				if(ft_instruments[j].seq_indices[Types[i]] == Indices[i] && ft_instruments[j].type == INST_VRC6)
				{
					ft_instruments[j].macros[Types[i]].release = release_point;
					ft_instruments[j].macros[Types[i]].setting = setting;
				}
			}
		}
	}

	free(Indices);
	free(Types);

	free(temp_macro);
	
	return success;
}

bool ft_process_sequences_n163_block(FILE* f, ftm_block* block)
{
	bool success = true;

	Uint32 seq_count = 0;

	read_uint32(f, &seq_count);

	ft_inst_macro* temp_macro = (ft_inst_macro*)calloc(1, sizeof(ft_inst_macro));

	Uint8* Indices = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_SEQUENCES * FT_SEQ_CNT);
	Uint8* Types = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_SEQUENCES * FT_SEQ_CNT);

	for(int i = 0; i < seq_count; i++)
	{
		Uint32 index = 0;
		read_uint32(f, &index);
		Indices[i] = index;

		Uint32 type = 0;
		read_uint32(f, &type);
		Types[i] = type;

		Uint8 size = 0;
		fread(&size, sizeof(size), 1, f);

		memset(temp_macro, 0, sizeof(ft_inst_macro));

		temp_macro->sequence_size = size;

		Sint32 loop_point = 0;
		Sint32 release_point = 0;

		Uint32 setting = 0;

		read_uint32(f, (Uint32*)&loop_point);

		temp_macro->loop = loop_point;

		read_uint32(f, (Uint32*)&release_point);
		read_uint32(f, &setting);

		temp_macro->release = release_point;
		temp_macro->setting = setting;

		for(int j = 0; j < size; j++)
		{
			fread(&temp_macro->sequence[j], sizeof(Uint8), 1, f);
		}

		for(int j = 0; j < FT_MAX_INSTRUMENTS; j++)
		{
			if(ft_instruments[j].seq_indices[type] == index && ft_instruments[j].type == INST_N163)
			{
				memcpy(&ft_instruments[j].macros[type], temp_macro, sizeof(ft_inst_macro));

				/*debug("N163 Macro %d type %d", Indices[i], Types[i]);

				for(int k = 0; k < ft_instruments[j].macros[type].sequence_size; k++)
				{
					debug("%d", ft_instruments[j].macros[type].sequence[k]);
				}*/
			}
		}
	}

	free(Indices);
	free(Types);

	free(temp_macro);
	
	return success;
}

bool ft_process_sequences_n106_block(FILE* f, ftm_block* block)
{
	bool success = ft_process_sequences_n163_block(f, block); // backwards compatibility?
	
	return success;
}

bool ft_process_sequences_s5b_block(FILE* f, ftm_block* block)
{
	bool success = true;

	Uint32 seq_count = 0;

	read_uint32(f, &seq_count);

	ft_inst_macro* temp_macro = (ft_inst_macro*)calloc(1, sizeof(ft_inst_macro));

	Uint8* Indices = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_SEQUENCES * FT_SEQ_CNT);
	Uint8* Types = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_SEQUENCES * FT_SEQ_CNT);

	for(int i = 0; i < seq_count; i++)
	{
		Uint32 index = 0;
		read_uint32(f, &index);
		Indices[i] = index;

		Uint32 type = 0;
		read_uint32(f, &type);
		Types[i] = type;

		Uint8 size = 0;
		fread(&size, sizeof(size), 1, f);

		memset(temp_macro, 0, sizeof(ft_inst_macro));

		temp_macro->sequence_size = size;

		Sint32 loop_point = 0;
		Sint32 release_point = 0;

		Uint32 setting = 0;

		read_uint32(f, (Uint32*)&loop_point);

		temp_macro->loop = loop_point;

		read_uint32(f, (Uint32*)&release_point);
		read_uint32(f, &setting);

		temp_macro->release = release_point;
		temp_macro->setting = setting;

		for(int j = 0; j < size; j++)
		{
			fread(&temp_macro->sequence[j], sizeof(Uint8), 1, f);
		}

		for(int j = 0; j < FT_MAX_INSTRUMENTS; j++)
		{
			if(ft_instruments[j].seq_indices[type] == index && ft_instruments[j].type == INST_S5B)
			{
				memcpy(&ft_instruments[j].macros[type], temp_macro, sizeof(ft_inst_macro));

				/*debug("S5B Macro %d type %d", Indices[i], Types[i]);

				for(int k = 0; k < ft_instruments[j].macros[type].sequence_size; k++)
				{
					debug("%d", ft_instruments[j].macros[type].sequence[k]);
				}*/
			}
		}
	}

	free(Indices);
	free(Types);

	free(temp_macro);
	
	return success;
}

bool ft_process_detunetables_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
}

bool ft_process_grooves_block(FILE* f, ftm_block* block)
{
	bool success = true;

	Uint8 num_grooves = 0;
	fread(&num_grooves, sizeof(num_grooves), 1, f);

	mused.song.num_grooves = num_grooves;

	//mused.song.flags |= MUS_USE_GROOVE;

	for(int i = 0; i < num_grooves; i++)
	{
		Uint8 index = 0;
		fread(&index, sizeof(index), 1, f);

		Uint8 size = 0;
		fread(&size, sizeof(size), 1, f);

		mused.song.groove_length[index] = size;
		//memset(&mused.song.grooves[index][0], 0, sizeof(Uint8) * MUS_MAX_GROOVE_LENGTH);

		fread(&mused.song.grooves[index][0], sizeof(Uint8) * size, 1, f);
	}

	Uint8 subsongs = 0;
	fread(&subsongs, sizeof(subsongs), 1, f);

	for(int i = 0; i < subsongs; i++)
	{
		Uint8 used = 0;
		fread(&used, sizeof(used), 1, f);

		if(used == 1 && i == selected_subsong)
		{
			mused.song.flags |= MUS_USE_GROOVE;

			Uint8 default_groove = mused.song.song_speed;

			MusPattern* pat = &mused.song.pattern[0];
			MusStep* step = &pat->step[0];

			for(int j = 0; j < MUS_MAX_COMMANDS; j++)
			{
				if(step->command[j] == 0)
				{
					step->command[j] = MUS_FX_SET_GROOVE | default_groove;
					goto end;
				}
			}
		}
	}

	end:;
	
	return success;
}

bool ft_process_bookmarks_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
}

bool ft_process_params_extra_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
}

bool (*block_parse_functions[FT_BLOCK_TYPES])(FILE* f, ftm_block* block) = 
{
	ft_process_params_block,
	ft_process_tuning_block,
	ft_process_info_block,
	ft_process_instruments_block,
	ft_process_sequences_block,
	ft_process_frames_block,
	ft_process_patterns_block,
	ft_process_dpcm_samples_block,
	ft_process_header_block,
	ft_process_comments_block,
	ft_process_sequences_vrc6_block,
	ft_process_sequences_n163_block,
	ft_process_sequences_n106_block,
	ft_process_sequences_s5b_block,
	ft_process_detunetables_block,
	ft_process_grooves_block,
	ft_process_bookmarks_block,
	ft_process_params_extra_block,
};

bool process_block(FILE* f, ftm_block* block)
{
	bool success = true;
	fseek(f, block->position, SEEK_SET);
	
	for(int i = 0; i < FT_BLOCK_TYPES; i++)
	{
		if(strcmp(block->name, block_sigs[i]) == 0)
		{
			debug("calling block processing function %d", i);
			success = (*block_parse_functions[i])(f, block);
			break;
		}
	}
	
	return success;
}

void convert_macro_value_arp(Uint8 value, Uint8 macro_num, ft_inst* ft_instrum, Uint8* macro_position, Uint16* inst_prog, Uint8* prog_unite_bits, Sint8* tone_offset)
{
	switch(ft_instrum->macros[1].setting)
	{
		case FT_SETTING_ARP_ABSOLUTE:
		{
			if(value <= 0x60)
			{
				inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (value);
				(*macro_position)++;
				return;
			}

			else
			{
				Uint8 temp = 0xff - value;
				inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (temp + 1);
				(*macro_position)++;
				return;
			}
			
			break;
		}

		case FT_SETTING_ARP_FIXED:
		{
			inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_ABS | (value + C_ZERO);
			(*macro_position)++;
			return;
			break;
		}

		case FT_SETTING_ARP_RELATIVE:
		{
			if(value <= 0x60)
			{
				inst_prog[*(macro_position)] = MUS_FX_PORTA_UP_SEMI | (value);
				(*macro_position)++;
				return;
			}

			else
			{
				Uint8 temp = 0xff - value;
				inst_prog[*(macro_position)] = MUS_FX_PORTA_DN_SEMI | (temp + 1);
				(*macro_position)++;
				return;
			}

			break;
		}

		case FT_SETTING_ARP_SCHEME: //oh God it would be hard
		{
			if(value < 0x40) //without x or y
			{
				if((*tone_offset) != 0)
				{
					inst_prog[*(macro_position)] = ((*tone_offset) >= 0 ? (MUS_FX_PORTA_DN_SEMI | abs(*tone_offset)) : (MUS_FX_PORTA_UP_SEMI | abs(*tone_offset)));
					prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);
					(*macro_position)++;
				}

				if(value <= 0x24)
				{
					inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (value);
				}

				else
				{
					inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (0x3f - value + 1);
				}
				
				(*macro_position)++;
				(*tone_offset) = 0;

				return;
			}

			if(value >= 0x40 && value < 0x80) //x
			{
				if(value <= 0x64)
				{
					Sint8 new_tone_offset = value - 0x40;
					Sint8 delta = new_tone_offset - (*tone_offset);
					
					if(delta != 0)
					{
						inst_prog[*(macro_position)] = ((delta) >= 0 ? (MUS_FX_PORTA_UP_SEMI | abs(delta)) : (MUS_FX_PORTA_DN_SEMI | abs(delta)));
						prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);
						(*macro_position)++;
					}

					inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | 0xf0; //1st external arp note

					(*macro_position)++;

					(*tone_offset) = new_tone_offset;
				}

				else
				{
					Sint8 new_tone_offset = -1 * (0x80 - value);
					Sint8 delta = new_tone_offset - (*tone_offset);

					if(delta != 0)
					{
						inst_prog[*(macro_position)] = ((delta) >= 0 ? (MUS_FX_PORTA_UP_SEMI | abs(delta)) : (MUS_FX_PORTA_DN_SEMI | abs(delta)));
						prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);
						(*macro_position)++;
					}

					inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | 0xf0; //1st external arp note

					(*macro_position)++;

					(*tone_offset) = new_tone_offset;
				}

				return;
			}

			if(value >= 0x80 && value < 0xc0) //y
			{
				if(value <= 0xa4)
				{
					Sint8 new_tone_offset = value - 0x80;
					Sint8 delta = new_tone_offset - (*tone_offset);
					
					if(delta != 0)
					{
						inst_prog[*(macro_position)] = ((delta) >= 0 ? (MUS_FX_PORTA_UP_SEMI | abs(delta)) : (MUS_FX_PORTA_DN_SEMI | abs(delta)));
						prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);
						(*macro_position)++;
					}

					inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | 0xf1; //2nd external arp note

					(*macro_position)++;

					(*tone_offset) = new_tone_offset;
				}

				else
				{
					Sint8 new_tone_offset = -1 * (0xc0 - value);
					Sint8 delta = new_tone_offset - (*tone_offset);

					if(delta != 0)
					{
						inst_prog[*(macro_position)] = ((delta) >= 0 ? (MUS_FX_PORTA_UP_SEMI | abs(delta)) : (MUS_FX_PORTA_DN_SEMI | abs(delta)));
						prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);
						(*macro_position)++;
					}

					inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | 0xf1; //2nd external arp note

					(*macro_position)++;

					(*tone_offset) = new_tone_offset;
				}

				return;
			}

			break;
		}

		default: break;
	}
}

void convert_macro_value(Uint8 value, Uint8 macro_num, ft_inst* ft_instrum, Uint8* macro_position, Uint16* inst_prog, Uint8* prog_unite_bits, Sint8* tone_offset)
{
	switch(ft_instrum->type)
	{
		case INST_2A03:
		{
			switch(macro_num)
			{
				case 0: //volume
				{
					inst_prog[*(macro_position)] = MUS_FX_SET_VOLUME_FROM_PROGRAM | convert_volumes_16[value];
					(*macro_position)++;
					return;
					break;
				}

				case 1: //arpeggio
				{
					convert_macro_value_arp(value, macro_num, ft_instrum, macro_position, inst_prog, prog_unite_bits, tone_offset);

					break;
				}

				case 2: //pitch
				{
					switch(ft_instrum->macros[2].setting)
					{
						case FT_SETTING_PITCH_RELATIVE:
						{
							if(value < 0x80 && value > 0)
							{
								inst_prog[*(macro_position)] = MUS_FX_PORTA_DN | my_min(0xff, (abs(value) * 6));
							}

							if(value >= 0x80)
							{
								Uint8 temp = 0x100 - value;
								inst_prog[*(macro_position)] = MUS_FX_PORTA_UP | my_min(0xff, (abs(temp) * 6));
							}

							(*macro_position)++;
							return;
							break;
						}
						
						case FT_SETTING_PITCH_ABSOLUTE:
						{
							if(value < 0x80 && value > 0)
							{
								Uint16 temp = value * 6 * 2;

								prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);

								if((temp >> 8) < 0x80)
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (Uint8)(temp >> 8);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 - (temp & 0xff));
								}

								else
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (Uint8)((temp >> 8) + 1);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 + (temp & 0xff));
								}
							}

							if(value >= 0x80) //going up
							{
								Uint8 tempp = 0x100 - value;

								Uint16 temp = tempp * 6 * 2;
								
								prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);

								if((temp >> 8) < 0x80)
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (Uint8)(temp >> 8);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 + (temp & 0xff));
								}

								else
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (Uint8)((temp >> 8) + 1);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 - (temp & 0xff));
								}
							}

							(*macro_position)++;
							return;
							break;
						}

						default: break;
					}
				}

				case 3: //hi-pitch
				{
					if(value < 0x80 && value > 0)
					{
						inst_prog[*(macro_position)] = MUS_FX_PORTA_DN_SEMI | my_min(0xff, (abs(value) * 2));
					}

					if(value >= 0x80)
					{
						Uint8 temp = 0x100 - value;
						inst_prog[*(macro_position)] = MUS_FX_PORTA_UP_SEMI | my_min(0xff, (abs(temp) * 2));
					}

					(*macro_position)++;
					return;
					break;
				}

				case 4: // duty/noise
				{ //noise mode will be treated separately when duplicating instruments for different channels
					inst_prog[*(macro_position)] = MUS_FX_PW_SET | ft_2a03_pulse_widths[value];
					(*macro_position)++;
					return;
					break;
				}

				default: break;
			}

			break;
		}

		case INST_VRC6:
		{
			switch(macro_num)
			{
				case 0: //volume
				{
					switch(ft_instrum->macros[0].setting)
					{
						case FT_SETTING_VOL_64_STEPS:
						{
							inst_prog[*(macro_position)] = MUS_FX_SET_VOLUME_FROM_PROGRAM | convert_volumes_64[value];
							(*macro_position)++;
							return;
							break;
						}

						case FT_SETTING_VOL_16_STEPS:
						{
							inst_prog[*(macro_position)] = MUS_FX_SET_VOLUME_FROM_PROGRAM | convert_volumes_16[value];
							(*macro_position)++;
							return;
							break;
						}

						default: break;
					}
					
					break;
				}

				case 1: //arpeggio
				{
					convert_macro_value_arp(value, macro_num, ft_instrum, macro_position, inst_prog, prog_unite_bits, tone_offset);

					break;
				}

				case 2: //pitch
				{
					switch(ft_instrum->macros[2].setting)
					{
						case FT_SETTING_PITCH_RELATIVE:
						{
							if(value < 0x80 && value > 0)
							{
								inst_prog[*(macro_position)] = MUS_FX_PORTA_DN | my_min(0xff, (abs(value) * 6));
							}

							if(value >= 0x80)
							{
								Uint8 temp = 0x100 - value;
								inst_prog[*(macro_position)] = MUS_FX_PORTA_UP | my_min(0xff, (abs(temp) * 6));
							}

							(*macro_position)++;
							return;
							break;
						}
						
						case FT_SETTING_PITCH_ABSOLUTE:
						{
							if(value < 0x80 && value > 0)
							{
								Uint16 temp = value * 6 * 2;

								prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);

								if((temp >> 8) < 0x80)
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (Uint8)(temp >> 8);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 - (temp & 0xff));
								}

								else
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (Uint8)((temp >> 8) + 1);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 + (temp & 0xff));
								}
							}

							if(value >= 0x80) //going up
							{
								Uint8 tempp = 0x100 - value;

								Uint16 temp = tempp * 6 * 2;
								
								prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);

								if((temp >> 8) < 0x80)
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (Uint8)(temp >> 8);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 + (temp & 0xff));
								}

								else
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (Uint8)((temp >> 8) + 1);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 - (temp & 0xff));
								}
							}

							(*macro_position)++;
							return;
							break;
						}

						default: break;
					}
				}

				case 3: //hi-pitch
				{
					if(value < 0x80 && value > 0)
					{
						inst_prog[*(macro_position)] = MUS_FX_PORTA_DN_SEMI | my_min(0xff, (abs(value) * 2));
					}

					if(value >= 0x80)
					{
						Uint8 temp = 0x100 - value;
						inst_prog[*(macro_position)] = MUS_FX_PORTA_UP_SEMI | my_min(0xff, (abs(temp) * 2));
					}

					(*macro_position)++;
					return;
					break;
				}

				case 4: //pulse width only
				{
					inst_prog[*(macro_position)] = MUS_FX_PW_SET | ft_vrc6_pulse_widths[value];
					(*macro_position)++;
					return;
					break;
				}

				default: break;
			}

			break;
		}

		case INST_N163:
		{
			switch(macro_num)
			{
				case 0: //volume
				{
					inst_prog[*(macro_position)] = MUS_FX_SET_VOLUME_FROM_PROGRAM | convert_volumes_16[value];
					(*macro_position)++;
					return;
					break;
					
					break;
				}

				case 1: //arpeggio
				{
					convert_macro_value_arp(value, macro_num, ft_instrum, macro_position, inst_prog, prog_unite_bits, tone_offset);

					break;
				}

				case 2: //pitch
				{
					switch(ft_instrum->macros[2].setting)
					{
						case FT_SETTING_PITCH_RELATIVE:
						{
							if(value < 0x80 && value > 0)
							{
								inst_prog[*(macro_position)] = MUS_FX_PORTA_DN | my_min(0xff, (abs(value) * 6));
							}

							if(value >= 0x80)
							{
								Uint8 temp = 0x100 - value;
								inst_prog[*(macro_position)] = MUS_FX_PORTA_UP | my_min(0xff, (abs(temp) * 6));
							}

							(*macro_position)++;
							return;
							break;
						}
						
						case FT_SETTING_PITCH_ABSOLUTE:
						{
							if(value < 0x80 && value > 0)
							{
								Uint16 temp = value * 6 * 2;

								prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);

								if((temp >> 8) < 0x80)
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (Uint8)(temp >> 8);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 - (temp & 0xff));
								}

								else
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (Uint8)((temp >> 8) + 1);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 + (temp & 0xff));
								}
							}

							if(value >= 0x80) //going up
							{
								Uint8 tempp = 0x100 - value;

								Uint16 temp = tempp * 6 * 2;
								
								prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);

								if((temp >> 8) < 0x80)
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (Uint8)(temp >> 8);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 + (temp & 0xff));
								}

								else
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (Uint8)((temp >> 8) + 1);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 - (temp & 0xff));
								}
							}

							(*macro_position)++;
							return;
							break;
						}

						default: break;
					}
				}

				case 3: //hi-pitch
				{
					if(value < 0x80 && value > 0)
					{
						inst_prog[*(macro_position)] = MUS_FX_PORTA_DN_SEMI | my_min(0xff, (abs(value) * 2));
					}

					if(value >= 0x80)
					{
						Uint8 temp = 0x100 - value;
						inst_prog[*(macro_position)] = MUS_FX_PORTA_UP_SEMI | my_min(0xff, (abs(temp) * 2));
					}

					(*macro_position)++;
					return;
					break;
				}

				case 4: //wavetable index
				{
					inst_prog[*(macro_position)] = MUS_FX_SET_LOCAL_SAMPLE | value;
					(*macro_position)++;
					return;
					break;
				}

				default: break;
			}

			break;
		}

		case INST_FDS:
		{
			switch(macro_num)
			{
				case 0: //volume
				{
					inst_prog[*(macro_position)] = MUS_FX_SET_VOLUME_FROM_PROGRAM | convert_volumes_32[value];
					(*macro_position)++;
					return;
					break;
				}

				case 1: //arpeggio
				{
					convert_macro_value_arp(value, macro_num, ft_instrum, macro_position, inst_prog, prog_unite_bits, tone_offset);

					break;
				}

				case 2: //pitch
				{
					switch(ft_instrum->macros[2].setting)
					{
						case FT_SETTING_PITCH_RELATIVE:
						{
							if(value < 0x80 && value > 0)
							{
								inst_prog[*(macro_position)] = MUS_FX_PORTA_DN | my_min(0xff, (abs(value) * 6));
							}

							if(value >= 0x80)
							{
								Uint8 temp = 0x100 - value;
								inst_prog[*(macro_position)] = MUS_FX_PORTA_UP | my_min(0xff, (abs(temp) * 6));
							}

							(*macro_position)++;
							return;
							break;
						}
						
						case FT_SETTING_PITCH_ABSOLUTE:
						{
							if(value < 0x80 && value > 0)
							{
								Uint16 temp = value * 6 * 2;

								prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);

								if((temp >> 8) < 0x80)
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (Uint8)(temp >> 8);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 - (temp & 0xff));
								}

								else
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (Uint8)((temp >> 8) + 1);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 + (temp & 0xff));
								}
							}

							if(value >= 0x80) //going up
							{
								Uint8 tempp = 0x100 - value;

								Uint16 temp = tempp * 6 * 2;
								
								prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);

								if((temp >> 8) < 0x80)
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (Uint8)(temp >> 8);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 + (temp & 0xff));
								}

								else
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (Uint8)((temp >> 8) + 1);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 - (temp & 0xff));
								}
							}

							(*macro_position)++;
							return;
							break;
						}

						default: break;
					}
				}

				default: break;
			}

			break;
		}

		case INST_S5B:
		{
			switch(macro_num)
			{
				case 0: //volume
				{
					inst_prog[*(macro_position)] = MUS_FX_SET_VOLUME_FROM_PROGRAM | convert_volumes_16[value];
					(*macro_position)++;
					return;
					break;
					
					break;
				}

				case 1: //arpeggio
				{
					convert_macro_value_arp(value, macro_num, ft_instrum, macro_position, inst_prog, prog_unite_bits, tone_offset);

					break;
				}

				case 2: //pitch
				{
					switch(ft_instrum->macros[2].setting)
					{
						case FT_SETTING_PITCH_RELATIVE:
						{
							if(value < 0x80 && value > 0)
							{
								inst_prog[*(macro_position)] = MUS_FX_PORTA_DN | my_min(0xff, (abs(value) * 6));
							}

							if(value >= 0x80)
							{
								Uint8 temp = 0x100 - value;
								inst_prog[*(macro_position)] = MUS_FX_PORTA_UP | my_min(0xff, (abs(temp) * 6));
							}

							(*macro_position)++;
							return;
							break;
						}
						
						case FT_SETTING_PITCH_ABSOLUTE:
						{
							if(value < 0x80 && value > 0)
							{
								Uint16 temp = value * 6 * 2;

								prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);

								if((temp >> 8) < 0x80)
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (Uint8)(temp >> 8);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 - (temp & 0xff));
								}

								else
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO_DOWN | (Uint8)((temp >> 8) + 1);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 + (temp & 0xff));
								}
							}

							if(value >= 0x80) //going up
							{
								Uint8 tempp = 0x100 - value;

								Uint16 temp = tempp * 6 * 2;
								
								prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);

								if((temp >> 8) < 0x80)
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (Uint8)(temp >> 8);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 + (temp & 0xff));
								}

								else
								{
									inst_prog[*(macro_position)] = MUS_FX_ARPEGGIO | (Uint8)((temp >> 8) + 1);
									(*macro_position)++;
									inst_prog[*(macro_position)] = MUS_FX_PITCH | (Uint8)(0x80 - (temp & 0xff));
								}
							}

							(*macro_position)++;
							return;
							break;
						}

						default: break;
					}
				}

				case 3: //hi-pitch
				{
					if(value < 0x80 && value > 0)
					{
						inst_prog[*(macro_position)] = MUS_FX_PORTA_DN_SEMI | my_min(0xff, (abs(value) * 2));
					}

					if(value >= 0x80)
					{
						Uint8 temp = 0x100 - value;
						inst_prog[*(macro_position)] = MUS_FX_PORTA_UP_SEMI | my_min(0xff, (abs(temp) * 2));
					}

					(*macro_position)++;
					return;
					break;
				}

				case 4: //noise/mode
				{
					if(value & S5B_ENVL)
					{
						inst_prog[*(macro_position)] = MUS_FX_BUZZ_TOGGLE | 1;

						if(!(value & (S5B_TONE | S5B_NOIS)))
						{
							prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);
							(*macro_position)++;
							inst_prog[*(macro_position)] = MUS_FX_PW_FINE_SET; //just envelope
							prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);
							(*macro_position)++;
						}

						if(value & (S5B_TONE | S5B_NOIS))
						{
							prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);
							(*macro_position)++;
						}
					}

					if(!(value & S5B_ENVL))
					{
						inst_prog[*(macro_position)] = MUS_FX_BUZZ_TOGGLE;

						prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);

						(*macro_position)++;
					}

					if(value & S5B_TONE)
					{
						if(value & S5B_NOIS)
						{
							inst_prog[*(macro_position)] = MUS_FX_SET_WAVEFORM | 3; //pulse+noise
						}

						else
						{
							inst_prog[*(macro_position)] = MUS_FX_SET_WAVEFORM | 2; //pulse
						}
						
						prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);
						(*macro_position)++;

						inst_prog[*(macro_position)] = MUS_FX_PW_FINE_SET | 0x800; //50% pulse (in case it needs to be restored)
						prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);
						(*macro_position)++;
					}

					if((value & S5B_NOIS) && !(value & S5B_TONE))
					{
						inst_prog[*(macro_position)] = MUS_FX_SET_WAVEFORM | 1; //noise
						prog_unite_bits[(*(macro_position)) / 8] |= 1 << ((*(macro_position)) & 7);
						(*macro_position)++;
					}

					Uint8 pitch = value & 0x1f; //lower 5 bits set noise pitch

					inst_prog[*(macro_position)] = MUS_FX_SET_NOISE_CONSTANT_PITCH | s5b_noise_notes[pitch];
					(*macro_position)++;
					
					return;
					break;
				}

				default: break;
			}

			break;
		}

		default: break;
	}
}

void convert_macro(MusInstrument* inst, Uint8 macro_num, ft_inst* ft_instrum, Uint8 current_program)
{
	Uint8 macro_position = 0;
	Sint8 tone_offset = 0;

	for(int p = 0; p < ft_instrum->macros[macro_num].sequence_size; p++)
	{
		if(macro_position == ft_instrum->macros[macro_num].release && ft_instrum->macros[macro_num].release > ft_instrum->macros[macro_num].loop && ft_instrum->macros[macro_num].loop != -1 && ft_instrum->macros[macro_num].release != -1) //loop point before release point
		{
			inst->program[current_program][macro_position] = MUS_FX_JUMP | ft_instrum->macros[macro_num].loop; //loop the loop part
			macro_position++;
			inst->program[current_program][macro_position] = MUS_FX_RELEASE_POINT; //jump to release part when release is triggered
			macro_position++;
		}

		if(macro_position == ft_instrum->macros[macro_num].release && ft_instrum->macros[macro_num].release != -1 && ft_instrum->macros[macro_num].loop == -1) //release point only (without loop point)
		{
			inst->program[current_program][macro_position] = MUS_FX_JUMP | macro_position; //infinitely hang here until release is triggered
			macro_position++;
			inst->program[current_program][macro_position] = MUS_FX_RELEASE_POINT;
			macro_position++;
		}

		//inst->program[current_program][macro_position] = MUS_FX_SET_VOLUME | convert_volumes_16[ft_instrum->macros[macro_num].sequence[p]];
		convert_macro_value(ft_instrum->macros[macro_num].sequence[p], macro_num, ft_instrum, &macro_position, inst->program[current_program], inst->program_unite_bits[current_program], &tone_offset);
		//macro_position++;

		if(ft_instrum->macros[macro_num].release == ft_instrum->macros[macro_num].loop && ft_instrum->macros[macro_num].loop != -1) //loop point and release point in the same position
		{
			if(p == ft_instrum->macros[macro_num].loop)
			{
				inst->program[current_program][macro_position] = MUS_FX_JUMP | macro_position; //infinitely hang here until release is triggered
				macro_position++;
				inst->program[current_program][macro_position] = MUS_FX_RELEASE_POINT;
				macro_position++;
			}

			if(p == ft_instrum->macros[macro_num].sequence_size - 1)
			{ //we have filled in last command
				inst->program[current_program][macro_position] = MUS_FX_JUMP | (ft_instrum->macros[macro_num].loop + (macro_position - p)); //jump to AFTER release point
				macro_position++;
			}
		}

		if(p == ft_instrum->macros[macro_num].sequence_size - 1 && ft_instrum->macros[macro_num].release == -1 && ft_instrum->macros[macro_num].loop != -1) //loop point only (without release point)
		{ //we have filled in last command
			inst->program[current_program][macro_position] = MUS_FX_JUMP | ft_instrum->macros[macro_num].loop; //jump to loop start
			macro_position++;
		}

		if(ft_instrum->macros[macro_num].release < ft_instrum->macros[macro_num].loop && ft_instrum->macros[macro_num].loop != -1 && ft_instrum->macros[macro_num].release != -1) //release point before loop point
		{
			if(p == ft_instrum->macros[macro_num].release)
			{
				inst->program[current_program][macro_position] = MUS_FX_JUMP | macro_position; //infinitely hang here until release is triggered
				macro_position++;
				inst->program[current_program][macro_position] = MUS_FX_RELEASE_POINT; //infinitely hang here until release is triggered
				macro_position++;
			}

			if(p == ft_instrum->macros[macro_num].sequence_size - 1)
			{
				inst->program[current_program][macro_position] = MUS_FX_JUMP | (ft_instrum->macros[macro_num].loop + (macro_position - p)); //jump to loop start
				macro_position++;
			}
		}
	}

	inst->program[current_program][macro_position] = MUS_FX_END;
}

bool search_inst_on_channel(Uint8 channel, Uint8 inst)
{
	for(int j = 0; j < sequence_length; j++)
	{
		MusPattern* pat = &mused.song.pattern[mused.song.sequence[channel][j].pattern];

		for(int s = 0; s < pat->num_steps; s++)
		{
			MusStep* step = &pat->step[s];

			Uint8 instr = step->instrument;

			if(instr == inst)
			{
				return true;
			}
		}
	}

	return false;
}

void replace_instrument(Uint8 chan, Uint8 source, Uint8 dest)
{
	for(int j = 0; j < sequence_length; j++)
	{
		MusPattern* pat = &mused.song.pattern[mused.song.sequence[chan][j].pattern];

		for(int s = 0; s < pat->num_steps; s++)
		{
			MusStep* step = &pat->step[s];

			Uint8 instr = step->instrument;

			if(instr == source)
			{
				step->instrument = dest;
			}
		}
	}
}

void copy_instrument(MusInstrument* dest, MusInstrument* source)
{
	mus_free_inst_programs(dest);
	mus_free_inst_samples(dest);

	memcpy(dest, source, sizeof(MusInstrument));

	for(int i = 0; i < source->num_macros; i++)
	{
		dest->program[i] = (Uint16*)calloc(1, sizeof(Uint16) * MUS_PROG_LEN);
		dest->program_unite_bits[i] = (Uint8*)calloc(1, sizeof(Uint8) * (MUS_PROG_LEN / 8 + 1));

		memcpy(dest->program[i], source->program[i], sizeof(Uint16) * MUS_PROG_LEN);
		memcpy(dest->program_unite_bits[i], source->program_unite_bits[i], sizeof(Uint8) * (MUS_PROG_LEN / 8 + 1));
	}

	for(int op = 0; op < CYD_FM_NUM_OPS; ++op)
	{
		for(int i = 0; i < dest->ops[op].num_macros; i++)
		{
			dest->ops[op].program[i] = (Uint16*)calloc(1, sizeof(Uint16) * MUS_PROG_LEN);
			dest->ops[op].program_unite_bits[i] = (Uint8*)calloc(1, sizeof(Uint8) * (MUS_PROG_LEN / 8 + 1));

			memcpy(dest->ops[op].program[i], source->ops[op].program[i], sizeof(Uint16) * MUS_PROG_LEN);
			memcpy(dest->ops[op].program_unite_bits[i], source->ops[op].program_unite_bits[i], sizeof(Uint8) * (MUS_PROG_LEN / 8 + 1));
		}
	}

	dest->local_samples = (CydWavetableEntry**)calloc(1, sizeof(CydWavetableEntry*) * dest->num_local_samples);
	dest->local_sample_names = (char**)calloc(1, sizeof(char*) * dest->num_local_samples);
	
	for(int j = 0; j < dest->num_local_samples; j++)
	{
		dest->local_samples[j] = (CydWavetableEntry*)calloc(1, sizeof(CydWavetableEntry));
		memset(dest->local_samples[j], 0, sizeof(CydWavetableEntry));
		cyd_wave_entry_init(dest->local_samples[j], NULL, 0, 0, 0, 0, 0);
		
		dest->local_sample_names[j] = (char*)calloc(1, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
		memset(dest->local_sample_names[j], 0, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
	}

	for(int i = 0; i < dest->num_local_samples; i++)
	{
		memcpy(dest->local_samples[i], source->local_samples[i], sizeof(CydWavetableEntry));
		memcpy(dest->local_sample_names[i], source->local_sample_names[i], sizeof(char) * MUS_WAVETABLE_NAME_LEN);

		if(source->local_samples[i]->data)
		{
			dest->local_samples[i]->data = (Sint16*)calloc(1, sizeof(Sint16) * dest->local_samples[i]->samples);
			memcpy(dest->local_samples[i]->data, source->local_samples[i]->data, sizeof(Sint16) * dest->local_samples[i]->samples);
		}
	}
}

void convert_instruments()
{
	debug("converting instruments");

	for(int i = 0; i < num_instruments; i++)
	{
		MusInstrument* inst = &mused.song.instrument[i];

		inst->flags &= ~(MUS_INST_DRUM | MUS_INST_SET_PW); //in famitracker last pulse width is preserved unless pulse width macro changes it, so we clear the flag
		inst->flags |= MUS_INST_RELATIVE_VOLUME | MUS_INST_RETRIGGER_ON_SLIDE;

		inst->cydflags &= ~(CYD_CHN_ENABLE_KEY_SYNC | CYD_CHN_ENABLE_TRIANGLE);
		inst->cydflags |= CYD_CHN_ENABLE_PULSE | CYD_CHN_ENABLE_FILTER;
		inst->pw = 0x0dff;

		inst->flttype = FLT_HP;
		inst->cutoff = 0x4;

		inst->adsr.a = 0;
		inst->adsr.d = 0;
		inst->adsr.s = 0x1f; //this is default 2A03 Pulse wave instrument

		inst->vibrato_speed = 0;
		inst->vibrato_depth = 0;
		inst->vibrato_shape = MUS_SHAPE_TRI_UP; //taken from some note visualizer; it can be also guessed because hardware playback is possible (tri easier to do than sine on 8-bit hardware)
	}

	for(int i = 0; i < num_instruments; i++)
	{
		Uint8 current_program = 0;

		MusInstrument* inst = &mused.song.instrument[i];

		ft_inst* ft_instrum = &ft_instruments[0];

		for(int j = 0; j < FT_MAX_INSTRUMENTS; j++)
		{
			if(ft_instruments[j].klystrack_instrument == i)
			{
				ft_instrum = &ft_instruments[j];
				goto next;
			}
		}

		next:;

		switch(ft_instrum->type)
		{
			case INST_2A03:
			{
				for(int j = 0; j < FT_SEQ_CNT; j++)
				{
					if(ft_instrum->seq_enable[j] && ft_instrum->macros[j].sequence_size > 0)
					{
						inst->prog_period[current_program] = 1;

						convert_macro(inst, j, ft_instrum, current_program);

						for(int k = 1; k < MUS_PROG_LEN; k++)
						{
							if((inst->program[current_program][k] & 0xff00) == MUS_FX_JUMP && inst->program[current_program][k] != MUS_FX_NOP && inst->program[current_program][k] != MUS_FX_END 
							&& (inst->program[current_program][k] & 0xff) != k) inst->program_unite_bits[current_program][(k - 1) / 8] |= 1 << ((k - 1) & 7);
						}

						strcpy(inst->program_names[current_program], sequence_names[j]);

						current_program++;

						inst->num_macros++;

						inst->program[current_program] = (Uint16*)calloc(1, sizeof(Uint16) * MUS_PROG_LEN);
						inst->program_unite_bits[current_program] = (Uint8*)calloc(1, sizeof(Uint8) * (MUS_PROG_LEN / 8 + 1));

						for(int k = 0; k < MUS_PROG_LEN; k++)
						{
							inst->program[current_program][k] = MUS_FX_NOP;
						}
					}
				}

				break;
			}

			case INST_VRC6:
			{
				for(int j = 0; j < FT_SEQ_CNT; j++)
				{
					if(ft_instrum->seq_enable[j] && ft_instrum->macros[j].sequence_size > 0)
					{
						inst->prog_period[current_program] = 1;

						convert_macro(inst, j, ft_instrum, current_program);

						for(int k = 1; k < MUS_PROG_LEN; k++)
						{
							if((inst->program[current_program][k] & 0xff00) == MUS_FX_JUMP && inst->program[current_program][k] != MUS_FX_NOP && inst->program[current_program][k] != MUS_FX_END 
							&& (inst->program[current_program][k] & 0xff) != k) inst->program_unite_bits[current_program][(k - 1) / 8] |= 1 << ((k - 1) & 7);
						}

						strcpy(inst->program_names[current_program], sequence_names_vrc6[j]);

						current_program++;

						inst->num_macros++;

						inst->program[current_program] = (Uint16*)calloc(1, sizeof(Uint16) * MUS_PROG_LEN);
						inst->program_unite_bits[current_program] = (Uint8*)calloc(1, sizeof(Uint8) * (MUS_PROG_LEN / 8 + 1));

						for(int k = 0; k < MUS_PROG_LEN; k++)
						{
							inst->program[current_program][k] = MUS_FX_NOP;
						}
					}
				}

				break;
			}

			case INST_N163:
			{
				for(int j = 0; j < FT_SEQ_CNT; j++)
				{
					if(ft_instrum->seq_enable[j] && ft_instrum->macros[j].sequence_size > 0)
					{
						inst->prog_period[current_program] = 1;

						convert_macro(inst, j, ft_instrum, current_program);

						for(int k = 1; k < MUS_PROG_LEN; k++)
						{
							if((inst->program[current_program][k] & 0xff00) == MUS_FX_JUMP && inst->program[current_program][k] != MUS_FX_NOP && inst->program[current_program][k] != MUS_FX_END 
							&& (inst->program[current_program][k] & 0xff) != k) inst->program_unite_bits[current_program][(k - 1) / 8] |= 1 << ((k - 1) & 7);
						}

						strcpy(inst->program_names[current_program], sequence_names_n163[j]);

						current_program++;

						inst->num_macros++;

						inst->program[current_program] = (Uint16*)calloc(1, sizeof(Uint16) * MUS_PROG_LEN);
						inst->program_unite_bits[current_program] = (Uint8*)calloc(1, sizeof(Uint8) * (MUS_PROG_LEN / 8 + 1));

						for(int k = 0; k < MUS_PROG_LEN; k++)
						{
							inst->program[current_program][k] = MUS_FX_NOP;
						}
					}
				}

				break;
			}

			case INST_FDS:
			{
				for(int j = 0; j < FT_SEQ_CNT; j++)
				{
					if(ft_instrum->seq_enable[j] && ft_instrum->macros[j].sequence_size > 0)
					{
						inst->prog_period[current_program] = 1;

						convert_macro(inst, j, ft_instrum, current_program);

						for(int k = 1; k < MUS_PROG_LEN; k++)
						{
							if((inst->program[current_program][k] & 0xff00) == MUS_FX_JUMP && inst->program[current_program][k] != MUS_FX_NOP && inst->program[current_program][k] != MUS_FX_END 
							&& (inst->program[current_program][k] & 0xff) != k) inst->program_unite_bits[current_program][(k - 1) / 8] |= 1 << ((k - 1) & 7);
						}

						strcpy(inst->program_names[current_program], sequence_names_fds[j]);

						current_program++;

						inst->num_macros++;

						inst->program[current_program] = (Uint16*)calloc(1, sizeof(Uint16) * MUS_PROG_LEN);
						inst->program_unite_bits[current_program] = (Uint8*)calloc(1, sizeof(Uint8) * (MUS_PROG_LEN / 8 + 1));

						for(int k = 0; k < MUS_PROG_LEN; k++)
						{
							inst->program[current_program][k] = MUS_FX_NOP;
						}
					}
				}

				break;
			}

			case INST_S5B:
			{
				for(int j = 0; j < FT_SEQ_CNT; j++)
				{
					if(ft_instrum->seq_enable[j] && ft_instrum->macros[j].sequence_size > 0)
					{
						inst->prog_period[current_program] = 1;

						convert_macro(inst, j, ft_instrum, current_program);

						for(int k = 1; k < MUS_PROG_LEN; k++)
						{
							if((inst->program[current_program][k] & 0xff00) == MUS_FX_JUMP && inst->program[current_program][k] != MUS_FX_NOP && inst->program[current_program][k] != MUS_FX_END 
							&& (inst->program[current_program][k] & 0xff) != k) inst->program_unite_bits[current_program][(k - 1) / 8] |= 1 << ((k - 1) & 7);
						}

						strcpy(inst->program_names[current_program], sequence_names_s5b[j]);

						current_program++;

						inst->num_macros++;

						inst->program[current_program] = (Uint16*)calloc(1, sizeof(Uint16) * MUS_PROG_LEN);
						inst->program_unite_bits[current_program] = (Uint8*)calloc(1, sizeof(Uint8) * (MUS_PROG_LEN / 8 + 1));

						for(int k = 0; k < MUS_PROG_LEN; k++)
						{
							inst->program[current_program][k] = MUS_FX_NOP;
						}
					}
				}

				break;
			}

			default: break;
		}

		if(ft_instrum->type == INST_2A03 || ft_instrum->type == INST_VRC6 || ft_instrum->type == INST_N163 || ft_instrum->type == INST_FDS)
		{
			for(int j = 0; j < FT_SEQ_CNT; j++)
			{
				//if(ft_instrum->seq_enable[j] && ft_instrum->macros[j].sequence_size > 0 && ft_instrum->macros[j].release != -1)
				//{
					inst->adsr.r = 0x3f;
				//}
			}
		}

		if(ft_instrum->type == INST_N163 && ft_instrum->num_n163_samples > 0)
		{
			inst->cydflags &= ~(CYD_CHN_ENABLE_PULSE);
			inst->cydflags |= CYD_CHN_ENABLE_WAVE;

			inst->flags |= MUS_INST_USE_LOCAL_SAMPLES;

			inst->local_sample = 0; //even if no macro is set we play 1st sample

			if(ft_instrum->num_n163_samples > 1)
			{
				mus_free_inst_samples(inst);

				inst->num_local_samples = ft_instrum->num_n163_samples;

				inst->local_samples = (CydWavetableEntry**)calloc(1, sizeof(CydWavetableEntry*) * inst->num_local_samples);
				inst->local_sample_names = (char**)calloc(1, sizeof(char*) * inst->num_local_samples);
				
				for(int j = 0; j < inst->num_local_samples; j++)
				{
					inst->local_samples[j] = (CydWavetableEntry*)calloc(1, sizeof(CydWavetableEntry));
					memset(inst->local_samples[j], 0, sizeof(CydWavetableEntry));
					cyd_wave_entry_init(inst->local_samples[j], NULL, 0, 0, 0, 0, 0);
					
					inst->local_sample_names[j] = (char*)calloc(1, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
					memset(inst->local_sample_names[j], 0, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
				}
			}

			Sint16* data = (Sint16*)calloc(1, sizeof(Sint16) * ft_instrum->n163_samples_len);

			for(int j = 0; j < ft_instrum->num_n163_samples; j++)
			{
				for(int k = 0; k < ft_instrum->n163_samples_len; k++)
				{
					data[k] = ((Sint16)ft_instrum->n163_samples[j][k] << 12) - 32768;
				}

				cyd_wave_entry_init(inst->local_samples[j], data, ft_instrum->n163_samples_len, CYD_WAVE_TYPE_SINT16, 1, 1, 1);

				inst->local_samples[j]->base_note = (MIDDLE_C << 8);
				inst->local_samples[j]->flags |= CYD_WAVE_LOOP | CYD_WAVE_NO_INTERPOLATION;
				inst->local_samples[j]->loop_end = ft_instrum->n163_samples_len;
				inst->local_samples[j]->sample_rate = (Uint64)get_freq(inst->local_samples[j]->base_note) / (Uint64)(32 * 2);
			}

			free(data);
		}

		if(ft_instrum->type == INST_FDS)
		{
			inst->cydflags &= ~(CYD_CHN_ENABLE_PULSE);
			inst->cydflags |= CYD_CHN_ENABLE_WAVE;

			inst->flags |= MUS_INST_USE_LOCAL_SAMPLES;

			inst->local_sample = 0; //even if no macro is set we play 1st sample

			Sint16* data = (Sint16*)calloc(1, sizeof(Sint16) * FT_FDS_WAVE_SIZE);

			for(int k = 0; k < FT_FDS_WAVE_SIZE; k++)
			{
				data[k] = ((Sint16)ft_instrum->fds_wave[k] << 10) - 32768;
			}

			cyd_wave_entry_init(inst->local_samples[0], data, FT_FDS_WAVE_SIZE, CYD_WAVE_TYPE_SINT16, 1, 1, 1);

			inst->local_samples[0]->base_note = (MIDDLE_C << 8);
			inst->local_samples[0]->flags |= CYD_WAVE_LOOP | CYD_WAVE_NO_INTERPOLATION;
			inst->local_samples[0]->loop_end = FT_FDS_WAVE_SIZE;
			inst->local_samples[0]->sample_rate = (Uint64)get_freq(inst->local_samples[0]->base_note) / (Uint64)(FT_FDS_WAVE_SIZE);

			free(data);
		}

		if(ft_instrum->type == INST_S5B)
		{
			inst->pw = 0x800;
			inst->flags |= MUS_INST_SET_PW;
		}
	}

	Uint8 curr_instrument = num_instruments; //we start from 1st empty instrument

	Uint8* inst_types = (Uint8*)calloc(1, sizeof(Uint8) * NUM_INSTRUMENTS);

	for(int i = 0; i < mused.song.num_channels; i++) //this setup only supports making duplicate instruments for different channels of ONE chip at a time
	{ //e.g. if you use same instrument on 2A03 and S5B it wouldn't be duplicated; supports duplicating across VRC6 & 2A03 tho
		for(int j = 0; j < sequence_length; j++)
		{
			MusPattern* pat = &mused.song.pattern[mused.song.sequence[i][j].pattern];

			for(int s = 0; s < pat->num_steps; s++)
			{
				MusStep* step = &pat->step[s];

				Uint8 inst = step->instrument;

				if(inst != MUS_NOTE_NO_INSTRUMENT)
				{
					if(i == 1 || i == 0 || channels_to_chips[i] == FT_SNDCHIP_MMC5) //2A03 pulse (including MMC5)
					{
						inst_types[inst] = INST_TYPE_2A03_PULSE;

						//search for occurence of this inst on tri/noise/DPCM
						bool is_tri = search_inst_on_channel(2, inst);
						bool is_noise = search_inst_on_channel(3, inst);
						bool is_dpcm = search_inst_on_channel(4, inst);

						if(is_tri) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_TRI;

							strcat(mused.song.instrument[curr_instrument].name, " (tri)");

							replace_instrument(2, inst, curr_instrument);

							curr_instrument++;
						}

						if(is_noise) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_NOISE;

							strcat(mused.song.instrument[curr_instrument].name, " (noise)");

							replace_instrument(3, inst, curr_instrument);

							curr_instrument++;
						}

						if(is_dpcm) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_DPCM;

							strcat(mused.song.instrument[curr_instrument].name, " (DPCM)");

							replace_instrument(4, inst, curr_instrument);

							memcpy(&ft_instruments[curr_instrument].dpcm_sample_map, &ft_instruments[inst].dpcm_sample_map, sizeof(ft_inst_dpcm_sample_map));

							curr_instrument++;
						}

						bool is_vrc6_saw = false; //instrument may also be used on VRC6 channel; example is "The Tower of Dreams" by nu11

						Sint8 saw_channel = -1;

						for(int ch = 0; ch < mused.song.num_channels; ch++)
						{
							if(channels_to_chips[ch] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 1] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 2] == FT_SNDCHIP_VRC6) //VRC6 saw (3rd channel)
							{
								is_vrc6_saw = search_inst_on_channel(ch, inst);
								saw_channel = ch;
							}
						}

						if(is_vrc6_saw && saw_channel != -1)
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_VRC6_SAW;

							strcat(mused.song.instrument[curr_instrument].name, " (VRC6 sawtooth)");

							replace_instrument(saw_channel, inst, curr_instrument);

							curr_instrument++;
						}

						bool is_vrc6_pulse = false;

						Uint8 vrc6_saw_chan = 0;

						for(int ch = 0; ch < mused.song.num_channels; ch++)
						{
							if(channels_to_chips[ch] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 1] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 2] == FT_SNDCHIP_VRC6) //we are on VRC6 sawtooth channel; scan two previous ones
							{
								is_vrc6_pulse = search_inst_on_channel(ch - 2, inst);

								if(is_vrc6_pulse == false)
								{
									is_vrc6_pulse = search_inst_on_channel(ch - 1, inst);
								}

								vrc6_saw_chan = ch;
							}
						}

						if(is_vrc6_pulse) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_VRC6_PULSE;

							strcat(mused.song.instrument[curr_instrument].name, " (VRC6 pulse)");

							replace_instrument(vrc6_saw_chan - 2, inst, curr_instrument);
							replace_instrument(vrc6_saw_chan - 1, inst, curr_instrument);

							curr_instrument++;
						}
					}

					if(i == 2) //2A03 triangle
					{
						//debug("tri inst %d", inst);

						inst_types[inst] = INST_TYPE_2A03_TRI;

						//search for occurence of this inst on pulse/noise/DPCM
						bool is_pulse = search_inst_on_channel(0, inst);

						if(is_pulse == false)
						{
							is_pulse = search_inst_on_channel(1, inst);
						}

						if(is_pulse == false)
						{
							for(int ch = 0; ch < mused.song.num_channels; ch++)
							{
								if(channels_to_chips[ch] == FT_SNDCHIP_MMC5)
								{
									if(is_pulse == false)
									{
										is_pulse = search_inst_on_channel(ch, inst);
									}
								}
							}
						}

						bool is_noise = search_inst_on_channel(3, inst);
						bool is_dpcm = search_inst_on_channel(4, inst);

						if(is_pulse) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_PULSE;

							strcat(mused.song.instrument[curr_instrument].name, " (2A03 pulse)");

							replace_instrument(0, inst, curr_instrument);
							replace_instrument(1, inst, curr_instrument);

							for(int ch = 0; ch < mused.song.num_channels; ch++)
							{
								if(channels_to_chips[ch] == FT_SNDCHIP_MMC5)
								{
									replace_instrument(ch, inst, curr_instrument);
								}
							}

							curr_instrument++;
						}

						if(is_noise) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_NOISE;

							strcat(mused.song.instrument[curr_instrument].name, " (noise)");

							replace_instrument(3, inst, curr_instrument);

							curr_instrument++;
						}

						if(is_dpcm) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_DPCM;

							strcat(mused.song.instrument[curr_instrument].name, " (DPCM)");

							replace_instrument(4, inst, curr_instrument);

							memcpy(&ft_instruments[curr_instrument].dpcm_sample_map, &ft_instruments[inst].dpcm_sample_map, sizeof(ft_inst_dpcm_sample_map));

							curr_instrument++;
						}

						bool is_vrc6_saw = false; //instrument may also be used on VRC6 channel; example is "The Tower of Dreams" by nu11

						Sint8 saw_channel = -1;

						for(int ch = 0; ch < mused.song.num_channels; ch++)
						{
							if(channels_to_chips[ch] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 1] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 2] == FT_SNDCHIP_VRC6) //VRC6 saw (3rd channel)
							{
								is_vrc6_saw = search_inst_on_channel(ch, inst);
								saw_channel = ch;
							}
						}

						if(is_vrc6_saw && saw_channel != -1)
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_VRC6_SAW;

							strcat(mused.song.instrument[curr_instrument].name, " (VRC6 sawtooth)");

							replace_instrument(saw_channel, inst, curr_instrument);

							curr_instrument++;
						}

						bool is_vrc6_pulse = false;

						Uint8 vrc6_saw_chan = 0;

						for(int ch = 0; ch < mused.song.num_channels; ch++)
						{
							if(channels_to_chips[ch] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 1] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 2] == FT_SNDCHIP_VRC6) //we are on VRC6 sawtooth channel; scan two previous ones
							{
								is_vrc6_pulse = search_inst_on_channel(ch - 2, inst);

								if(is_vrc6_pulse == false)
								{
									is_vrc6_pulse = search_inst_on_channel(ch - 1, inst);
								}

								vrc6_saw_chan = ch;
							}
						}

						if(is_vrc6_pulse) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_VRC6_PULSE;

							strcat(mused.song.instrument[curr_instrument].name, " (VRC6 pulse)");

							replace_instrument(vrc6_saw_chan - 2, inst, curr_instrument);
							replace_instrument(vrc6_saw_chan - 1, inst, curr_instrument);

							curr_instrument++;
						}
					}

					if(i == 3) //2A03 noise
					{
						inst_types[inst] = INST_TYPE_2A03_NOISE;

						//search for occurence of this inst on pulse/noise/DPCM
						bool is_pulse = search_inst_on_channel(0, inst);

						if(is_pulse == false)
						{
							is_pulse = search_inst_on_channel(1, inst);
						}

						if(is_pulse == false)
						{
							for(int ch = 0; ch < mused.song.num_channels; ch++)
							{
								if(channels_to_chips[ch] == FT_SNDCHIP_MMC5)
								{
									if(is_pulse == false)
									{
										is_pulse = search_inst_on_channel(ch, inst);
									}
								}
							}
						}

						bool is_tri = search_inst_on_channel(2, inst);
						bool is_dpcm = search_inst_on_channel(4, inst);

						if(is_pulse) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_PULSE;

							strcat(mused.song.instrument[curr_instrument].name, " (2A03 pulse)");

							replace_instrument(0, inst, curr_instrument);
							replace_instrument(1, inst, curr_instrument);

							for(int ch = 0; ch < mused.song.num_channels; ch++)
							{
								if(channels_to_chips[ch] == FT_SNDCHIP_MMC5)
								{
									replace_instrument(ch, inst, curr_instrument);
								}
							}

							curr_instrument++;
						}

						if(is_tri) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_TRI;

							strcat(mused.song.instrument[curr_instrument].name, " (tri)");

							replace_instrument(2, inst, curr_instrument);

							curr_instrument++;
						}

						if(is_dpcm) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_DPCM;

							strcat(mused.song.instrument[curr_instrument].name, " (DPCM)");

							replace_instrument(4, inst, curr_instrument);

							memcpy(&ft_instruments[curr_instrument].dpcm_sample_map, &ft_instruments[inst].dpcm_sample_map, sizeof(ft_inst_dpcm_sample_map));

							curr_instrument++;
						}

						bool is_vrc6_saw = false; //instrument may also be used on VRC6 channel; example is "The Tower of Dreams" by nu11

						Sint8 saw_channel = -1;

						for(int ch = 0; ch < mused.song.num_channels; ch++)
						{
							if(channels_to_chips[ch] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 1] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 2] == FT_SNDCHIP_VRC6) //VRC6 saw (3rd channel)
							{
								is_vrc6_saw = search_inst_on_channel(ch, inst);
								saw_channel = ch;
							}
						}

						if(is_vrc6_saw && saw_channel != -1)
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_VRC6_SAW;

							strcat(mused.song.instrument[curr_instrument].name, " (VRC6 sawtooth)");

							replace_instrument(saw_channel, inst, curr_instrument);

							curr_instrument++;
						}

						bool is_vrc6_pulse = false;

						Uint8 vrc6_saw_chan = 0;

						for(int ch = 0; ch < mused.song.num_channels; ch++)
						{
							if(channels_to_chips[ch] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 1] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 2] == FT_SNDCHIP_VRC6) //we are on VRC6 sawtooth channel; scan two previous ones
							{
								is_vrc6_pulse = search_inst_on_channel(ch - 2, inst);

								if(is_vrc6_pulse == false)
								{
									is_vrc6_pulse = search_inst_on_channel(ch - 1, inst);
								}

								vrc6_saw_chan = ch;
							}
						}

						if(is_vrc6_pulse) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_VRC6_PULSE;

							strcat(mused.song.instrument[curr_instrument].name, " (VRC6 pulse)");

							replace_instrument(vrc6_saw_chan - 2, inst, curr_instrument);
							replace_instrument(vrc6_saw_chan - 1, inst, curr_instrument);

							curr_instrument++;
						}
					}

					if(i == 4) //2A03 DPCM
					{
						inst_types[inst] = INST_TYPE_2A03_DPCM;

						//search for occurence of this inst on pulse/noise/DPCM
						bool is_pulse = search_inst_on_channel(0, inst);

						if(is_pulse == false)
						{
							is_pulse = search_inst_on_channel(1, inst);
						}

						if(is_pulse == false)
						{
							for(int ch = 0; ch < mused.song.num_channels; ch++)
							{
								if(channels_to_chips[ch] == FT_SNDCHIP_MMC5)
								{
									if(is_pulse == false)
									{
										is_pulse = search_inst_on_channel(ch, inst);
									}
								}
							}
						}

						bool is_tri = search_inst_on_channel(2, inst);
						bool is_noise = search_inst_on_channel(3, inst);

						if(is_pulse) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_PULSE;

							strcat(mused.song.instrument[curr_instrument].name, " (2A03 pulse)");

							replace_instrument(0, inst, curr_instrument);
							replace_instrument(1, inst, curr_instrument);

							for(int ch = 0; ch < mused.song.num_channels; ch++)
							{
								if(channels_to_chips[ch] == FT_SNDCHIP_MMC5)
								{
									replace_instrument(ch, inst, curr_instrument);
								}
							}

							curr_instrument++;
						}

						if(is_tri) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_TRI;

							strcat(mused.song.instrument[curr_instrument].name, " (tri)");

							replace_instrument(2, inst, curr_instrument);

							curr_instrument++;
						}

						if(is_noise) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_2A03_NOISE;

							strcat(mused.song.instrument[curr_instrument].name, " (noise)");

							replace_instrument(3, inst, curr_instrument);

							curr_instrument++;
						}

						bool is_vrc6_saw = false; //instrument may also be used on VRC6 channel; example is "The Tower of Dreams" by nu11

						Sint8 saw_channel = -1;

						for(int ch = 0; ch < mused.song.num_channels; ch++)
						{
							if(channels_to_chips[ch] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 1] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 2] == FT_SNDCHIP_VRC6) //VRC6 saw (3rd channel)
							{
								is_vrc6_saw = search_inst_on_channel(ch, inst);
								saw_channel = ch;
							}
						}

						if(is_vrc6_saw && saw_channel != -1)
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_VRC6_SAW;

							strcat(mused.song.instrument[curr_instrument].name, " (VRC6 sawtooth)");

							replace_instrument(saw_channel, inst, curr_instrument);

							curr_instrument++;
						}

						bool is_vrc6_pulse = false;

						Uint8 vrc6_saw_chan = 0;

						for(int ch = 0; ch < mused.song.num_channels; ch++)
						{
							if(channels_to_chips[ch] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 1] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 2] == FT_SNDCHIP_VRC6) //we are on VRC6 sawtooth channel; scan two previous ones
							{
								is_vrc6_pulse = search_inst_on_channel(ch - 2, inst);

								if(is_vrc6_pulse == false)
								{
									is_vrc6_pulse = search_inst_on_channel(ch - 1, inst);
								}

								vrc6_saw_chan = ch;
							}
						}

						if(is_vrc6_pulse) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_VRC6_PULSE;

							strcat(mused.song.instrument[curr_instrument].name, " (VRC6 pulse)");

							replace_instrument(vrc6_saw_chan - 2, inst, curr_instrument);
							replace_instrument(vrc6_saw_chan - 1, inst, curr_instrument);

							curr_instrument++;
						}
					}

					if(channels_to_chips[i] == FT_SNDCHIP_VRC6 && channels_to_chips[i + 1] == FT_SNDCHIP_VRC6) //VRC6 pulse (1st/2nd channel)
					{
						bool is_vrc6_saw = false;

						Sint8 saw_channel = -1;

						inst_types[inst] = INST_TYPE_VRC6_PULSE;

						for(int ch = 0; ch < mused.song.num_channels; ch++)
						{
							if(channels_to_chips[ch] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 1] == FT_SNDCHIP_VRC6 && channels_to_chips[ch - 2] == FT_SNDCHIP_VRC6) //VRC6 saw (3rd channel)
							{
								is_vrc6_saw = search_inst_on_channel(ch, inst);
								saw_channel = ch;
							}
						}

						if(is_vrc6_saw && saw_channel != -1)
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_VRC6_SAW;

							strcat(mused.song.instrument[curr_instrument].name, " (VRC6 sawtooth)");

							replace_instrument(saw_channel, inst, curr_instrument);

							curr_instrument++;
						}
					}

					if(channels_to_chips[i] == FT_SNDCHIP_VRC6 && channels_to_chips[i - 1] == FT_SNDCHIP_VRC6 && channels_to_chips[i - 2] == FT_SNDCHIP_VRC6) //VRC6 saw (3rd channel)
					{
						inst_types[inst] = INST_TYPE_VRC6_SAW;

						bool is_vrc6_pulse = search_inst_on_channel(i - 2, inst);

						if(is_vrc6_pulse == false)
						{
							is_vrc6_pulse = search_inst_on_channel(i - 1, inst);
						}

						if(is_vrc6_pulse) //we make a copy of instrument and leave it for other channels, for tri instrument we keep source instrument but change its waveform
						{
							copy_instrument(&mused.song.instrument[curr_instrument], &mused.song.instrument[inst]);

							inst_types[curr_instrument] = INST_TYPE_VRC6_PULSE;

							strcat(mused.song.instrument[curr_instrument].name, " (VRC6 pulse)");

							replace_instrument(i - 2, inst, curr_instrument);
							replace_instrument(i - 1, inst, curr_instrument);

							curr_instrument++;
						}
					}
				}
			}
		}
	}

	bool need_tri = false;
	bool need_vrc6_saw = false;

	Uint8 nes_tri = 0;
	Uint8 vrc6_saw = 0;

	for(int i = 0; i < curr_instrument; i++)
	{
		//debug("inst types %d %d", i, inst_types[i]);

		if(inst_types[i] == INST_TYPE_2A03_TRI)
		{
			need_tri = true;
		}

		if(inst_types[i] == INST_TYPE_VRC6_SAW)
		{
			need_vrc6_saw = true;
		}
	}

	if(need_tri)
	{
		for(int i = 0; i < CYD_WAVE_MAX_ENTRIES; i++)
		{
			if(mused.cyd.wavetable_entries[i].data == NULL && strcmp(mused.song.wavetable_names[i], "") == 0)
			{
				Sint16* data = (Sint16*)calloc(1, sizeof(Sint16) * 32);

				for(int j = 0; j < 32; j++)
				{
					if(j < 16)
					{
						data[j] = -32768 + 65536 * j / 16;
					}

					else
					{
						data[j] = 32767 - 65536 * (j - 16) / 16;
					}
				}

				mused.cyd.wavetable_entries[i].flags |= CYD_WAVE_NO_INTERPOLATION | CYD_WAVE_LOOP | CYD_WAVE_ACC_NO_RESET;
				mused.cyd.wavetable_entries[i].base_note = MIDDLE_C << 8;
				
				cyd_wave_entry_init(&mused.cyd.wavetable_entries[i], data, 32, CYD_WAVE_TYPE_SINT16, 1, 1, 1);

				mused.cyd.wavetable_entries[i].loop_begin = 0;
				mused.cyd.wavetable_entries[i].loop_end = 32;

				mused.cyd.wavetable_entries[i].sample_rate = get_freq(MIDDLE_C << 8) * 32 / 1024;

				strcpy(mused.song.wavetable_names[i], "Ricoh 2A03/2A07 triangle wave");

				free(data);

				nes_tri = i;

				goto saw;
			}
		}
	}

	saw:;

	if(need_vrc6_saw)
	{
		for(int i = 0; i < CYD_WAVE_MAX_ENTRIES; i++)
		{
			if(mused.cyd.wavetable_entries[i].data == NULL && strcmp(mused.song.wavetable_names[i], "") == 0)
			{
				Sint16* data = (Sint16*)calloc(1, sizeof(Sint16) * 8);

				for(int j = 0; j < 8; j++)
				{
					data[j] = -32767 - 65536 * j / 8;
				}

				mused.cyd.wavetable_entries[i].flags |= CYD_WAVE_NO_INTERPOLATION | CYD_WAVE_LOOP | CYD_WAVE_ACC_NO_RESET;
				mused.cyd.wavetable_entries[i].base_note = MIDDLE_C << 8;
				
				cyd_wave_entry_init(&mused.cyd.wavetable_entries[i], data, 8, CYD_WAVE_TYPE_SINT16, 1, 1, 1);

				mused.cyd.wavetable_entries[i].loop_begin = 0;
				mused.cyd.wavetable_entries[i].loop_end = 8;

				mused.cyd.wavetable_entries[i].sample_rate = get_freq(MIDDLE_C << 8) * 8 / 1024;

				strcpy(mused.song.wavetable_names[i], "Konami VRC6 sawtooth wave");

				free(data);

				vrc6_saw = i;

				goto insts;
			}
		}
	}

	insts:;

	for(int i = 0; i < curr_instrument; i++) //this setup only supports making duplicate instruments for different channels of ONE chip at a time
	{
		MusInstrument* inst = &mused.song.instrument[i];

		switch(inst_types[i])
		{
			case INST_TYPE_2A03_TRI:
			{
				for(int j = 0; j < inst->num_macros; j++)
				{
					if(strcmp(inst->program_names[j], sequence_names[0]) == 0) //volume macro
					{
						for(int k = 0; k < MUS_PROG_LEN; k++) //leave only full volume and 0 volume commands (that's how it works in fami)
						{
							if((inst->program[j][k] & 0xff00) == MUS_FX_SET_VOLUME_FROM_PROGRAM)
							{
								if((inst->program[j][k] & 0xff))
								{
									inst->program[j][k] = MUS_FX_SET_VOLUME_FROM_PROGRAM | MAX_VOLUME;
								}
							}
						}
					}
				}

				inst->cydflags &= ~(CYD_CHN_ENABLE_PULSE);
				inst->flags &= ~(MUS_INST_USE_LOCAL_SAMPLES);
				//inst->cydflags |= CYD_CHN_ENABLE_TRIANGLE; //TODO: add faithful NES tri
				inst->cydflags |= CYD_CHN_ENABLE_WAVE;

				inst->wavetable_entry = nes_tri;

				inst->flags &= ~(MUS_INST_USE_LOCAL_SAMPLES | MUS_INST_BIND_LOCAL_SAMPLES_TO_NOTES);

				break;
			}

			case INST_TYPE_2A03_NOISE:
			{
				inst->cydflags &= ~(CYD_CHN_ENABLE_PULSE);
				inst->cydflags |= CYD_CHN_ENABLE_NOISE | CYD_CHN_ENABLE_1_BIT_NOISE;

				for(int j = 0; j < inst->num_macros; j++)
				{
					if(strcmp(inst->program_names[j], sequence_names[4]) == 0) //convert pulse widths to noise modes
					{
						for(int s = 0; s < MUS_PROG_LEN; s++)
						{
							if((inst->program[j][s] & 0xff00) == MUS_FX_PW_SET)
							{
								if((inst->program[j][s] & 0xff) == ft_2a03_pulse_widths[1] || (inst->program[j][s] & 0xff) == ft_2a03_pulse_widths[3])
								{
									inst->program[j][s] = MUS_FX_EXT_SET_NOISE_MODE | 0b11;
								}

								else
								{
									inst->program[j][s] = MUS_FX_EXT_SET_NOISE_MODE | 0b01;
								}
							}
						}
					}

					if(strcmp(inst->program_names[j], sequence_names[1]) == 0) //arpeggio
					{
						for(int s = 0; s < MUS_PROG_LEN; s++)
						{
							if((inst->program[j][s] & 0xff00) == MUS_FX_ARPEGGIO)
							{
								inst->program[j][s] = MUS_FX_ARPEGGIO | my_min(FREQ_TAB_SIZE - 1, ((inst->program[j][s] & 0xff) << 2));
							}
						}
					}
				}

				inst->flags &= ~(MUS_INST_USE_LOCAL_SAMPLES | MUS_INST_BIND_LOCAL_SAMPLES_TO_NOTES);

				break;
			}

			case INST_TYPE_2A03_DPCM:
			{
				for(int s = 0; s < MUS_PROG_LEN; s++)
				{
					inst->program[0][s] = MUS_FX_NOP;
				}

				memset(inst->program_unite_bits[0], 0, MUS_PROG_LEN / 8 + 1);

				for(int j = 1; j < inst->num_macros; j++)
				{
					free(inst->program[j]);
					inst->program[j] = NULL;
					free(inst->program_unite_bits[j]);
					inst->program_unite_bits[j] = NULL;
				}

				inst->num_macros = 1;

				inst->cydflags &= ~(CYD_CHN_ENABLE_PULSE);
				inst->cydflags |= CYD_CHN_ENABLE_WAVE;
				inst->flags |= MUS_INST_BIND_LOCAL_SAMPLES_TO_NOTES | MUS_INST_WAVE_LOCK_NOTE | MUS_INST_USE_LOCAL_SAMPLES;
				break;
			}

			case INST_TYPE_VRC6_SAW:
			{
				inst->cydflags &= ~(CYD_CHN_ENABLE_PULSE);
				inst->flags &= ~(MUS_INST_USE_LOCAL_SAMPLES);

				inst->cydflags |= CYD_CHN_ENABLE_WAVE;

				inst->wavetable_entry = vrc6_saw;

				for(int j = 0; j < inst->num_macros; j++)
				{
					if(strcmp(inst->program_names[j], sequence_names_vrc6[4]) == 0) //get rid of pulse widths
					{
						if(inst->program[j])
						{
							free(inst->program[j]);
							inst->program[j] = NULL;
						}
						
						if(inst->program_unite_bits[j])
						{
							free(inst->program_unite_bits[j]);
							inst->program_unite_bits[j] = NULL;
						}

						inst->num_macros -= 2;
					}
				}

				inst->flags &= ~(MUS_INST_USE_LOCAL_SAMPLES | MUS_INST_BIND_LOCAL_SAMPLES_TO_NOTES);

				break;
			}

			default: break;
		}
	}

	Uint8** local_samples_pitches = (Uint8**)calloc(1, sizeof(Uint8*) * CYD_WAVE_MAX_ENTRIES);

	for(int i = 0; i < CYD_WAVE_MAX_ENTRIES; i++)
	{
		local_samples_pitches[i] = (Uint8*)calloc(1, sizeof(Uint8) * 32); //0-15 unlooped pitches, 16-31 looped pitches
	}

	for(int i = 0; i < curr_instrument; i++) //for DPCM samples make copies of samples with proper sample rate and loop settings
	{
		if(inst_types[i] == INST_TYPE_2A03_DPCM)
		{
			ft_inst* ft_ins = &ft_instruments[i];
			MusInstrument* inst = &mused.song.instrument[i];

			inst->adsr.r = 0;

			Uint8 current_local_sample = 0;

			for(int j = 0; j < CYD_WAVE_MAX_ENTRIES; j++)
			{
				memset(local_samples_pitches[j], MUS_NOTE_TO_SAMPLE_NONE, sizeof(Uint8) * 32);
			}

			for(int j = 0; j < 8 * 12; j++)
			{
				if(ft_ins->dpcm_sample_map.sample[j] != MUS_NOTE_TO_SAMPLE_NONE && inst->note_to_sample_array[j + C_ZERO].sample != MUS_NOTE_TO_SAMPLE_NONE && ft_ins->dpcm_sample_map.pitch[j] != 15 && ft_ins->dpcm_sample_map.pitch[j] != 255) //if there is sample which settings aren't "max sample rate & unlooped"; 255 is from file? idk it crashes the thing
				{
					Uint8 pitch = (ft_ins->dpcm_sample_map.pitch[j] >= 128) ? (ft_ins->dpcm_sample_map.pitch[j] - 128 + 16) : ft_ins->dpcm_sample_map.pitch[j];

					if(local_samples_pitches[inst->note_to_sample_array[j + C_ZERO].sample][pitch] == MUS_NOTE_TO_SAMPLE_NONE) //we don't have this sample variant
					{
						if(current_local_sample > 0)
						{
							inst->num_local_samples++;

							inst->local_samples = (CydWavetableEntry**)realloc(inst->local_samples, sizeof(CydWavetableEntry*) * inst->num_local_samples);
							inst->local_sample_names = (char**)realloc(inst->local_sample_names, sizeof(char*) * inst->num_local_samples);

							inst->local_samples[inst->num_local_samples - 1] = (CydWavetableEntry*)calloc(1, sizeof(CydWavetableEntry));
							inst->local_sample_names[inst->num_local_samples - 1] = (char*)calloc(1, sizeof(char) * MUS_WAVETABLE_NAME_LEN);
						}

						memcpy(inst->local_samples[current_local_sample], &mused.cyd.wavetable_entries[inst->note_to_sample_array[j + C_ZERO].sample], sizeof(CydWavetableEntry));
						memcpy(inst->local_sample_names[current_local_sample], mused.song.wavetable_names[inst->note_to_sample_array[j + C_ZERO].sample], sizeof(char) * MUS_WAVETABLE_NAME_LEN);

						inst->local_samples[current_local_sample]->data = (Sint16*)calloc(1, sizeof(Sint16) * inst->local_samples[current_local_sample]->samples);
						memcpy(inst->local_samples[current_local_sample]->data, mused.cyd.wavetable_entries[inst->note_to_sample_array[j + C_ZERO].sample].data, sizeof(Sint16) * inst->local_samples[current_local_sample]->samples);

						inst->note_to_sample_array[j + C_ZERO].sample = current_local_sample;
						inst->note_to_sample_array[j + C_ZERO].flags = 0; //local
						
						if(pitch & 16)
						{
							inst->local_samples[current_local_sample]->flags |= CYD_WAVE_LOOP;
							inst->local_samples[current_local_sample]->loop_end = inst->local_samples[current_local_sample]->samples;
						}

						inst->local_samples[current_local_sample]->sample_rate = ((machine == FT_MACHINE_PAL) ? dpcm_sample_rates_pal[pitch & 15] : dpcm_sample_rates_ntsc[pitch & 15]);

						local_samples_pitches[inst->note_to_sample_array[j + C_ZERO].sample][pitch] = current_local_sample;

						current_local_sample++;
					}

					else //we already have sample variant
					{
						for(int k = 0; k < 12 * 8; k++)
						{
							if(ft_ins->dpcm_sample_map.sample[k] == ft_ins->dpcm_sample_map.sample[j] && ft_ins->dpcm_sample_map.pitch[k] == ft_ins->dpcm_sample_map.pitch[j])
							{
								memcpy(&inst->note_to_sample_array[j + C_ZERO], &inst->note_to_sample_array[k + C_ZERO], sizeof(MusInstNoteToSample));
							}
						}
					}
				}
			}
		}
	}

	for(int i = 0; i < curr_instrument; i++) //arpeggio macro (if there isn't arp macro at all)
	{
		MusInstrument* inst = &mused.song.instrument[i];

		bool is_arp_macro = false;

		for(int j = 0; j < inst->num_macros; j++)
		{
			if(strcmp(inst->program_names[j], sequence_names[1]) == 0 || 
			strcmp(inst->program_names[j], sequence_names_vrc6[1]) == 0 || 
			strcmp(inst->program_names[j], sequence_names_n163[1]) == 0 || 
			strcmp(inst->program_names[j], sequence_names_fds[1]) == 0 ||
			strcmp(inst->program_names[j], sequence_names_s5b[1]) == 0) //if there is arp macro
			{
				is_arp_macro = true;
			}
		}

		if(is_arp_macro == false) //we make simple arp macro so e.g. 047 command actually leads to arpeggio
		{
			if(inst->num_macros > 1)
			{
				inst->program[inst->num_macros] = (Uint16*)calloc(1, sizeof(Uint16) * MUS_PROG_LEN);
				inst->program_unite_bits[inst->num_macros] = (Uint8*)calloc(1, sizeof(Uint8) * (MUS_PROG_LEN / 8 + 1));

				for(int k = 0; k < MUS_PROG_LEN; k++)
				{
					inst->program[inst->num_macros][k] = MUS_FX_NOP;
				}
			}

			else
			{
				inst->num_macros--; //so we index macro №0 if no additional macros were allocated
			}

			inst->program[inst->num_macros][0] = MUS_FX_ARPEGGIO;
			inst->program[inst->num_macros][1] = MUS_FX_ARPEGGIO | 0xf0; //1st ext note
			inst->program[inst->num_macros][2] = MUS_FX_ARPEGGIO | 0xf1; //2nd external note
			inst->program[inst->num_macros][3] = MUS_FX_JUMP; //jump to step 0

			inst->prog_period[inst->num_macros] = 1;

			strcpy(inst->program_names[inst->num_macros], "pat.eff. arpeggio");

			inst->num_macros++; //return back to 1 if only macro #0 was there; if there were more macros we allocated a new one so still increment
		}
	}

	for(int i = 0; i < CYD_WAVE_MAX_ENTRIES; i++)
	{
		free(local_samples_pitches[i]);
	}

	free(local_samples_pitches);

	free(inst_types);
}

void add_volumes() //Famitracker saves last volume, klystrack does not, so we force last volume for all consecutive notes until new volume is encountered
{
	for(int i = 0; i < mused.song.num_channels; i++)
	{
		Uint8 vol = MUS_NOTE_NO_VOLUME;

		for(int j = 0; j < sequence_length; j++)
		{
			MusPattern* pat = &mused.song.pattern[mused.song.sequence[i][j].pattern];

			for(int s = 0; s < pat->num_steps; s++)
			{
				MusStep* step = &pat->step[s];

				if(step->note != MUS_NOTE_NONE && step->note != MUS_NOTE_RELEASE && step->note != MUS_NOTE_CUT && step->note != MUS_NOTE_MACRO_RELEASE && step->note != MUS_NOTE_RELEASE_WITHOUT_MACRO)
				{
					if(step->volume != MUS_NOTE_NO_VOLUME)
					{
						vol = step->volume;
					}

					else
					{
						step->volume = vol;
					}
				}

				else
				{
					if(step->volume != MUS_NOTE_NO_VOLUME)
					{
						vol = step->volume;
					}
				}
			}
		}
	}
}

void set_channel_volumes()
{
	for(int i = 0; i < mused.song.num_channels; i++)
	{
		mused.song.default_volume[i] = 0x30;

		if(channels_to_chips[i] == FT_SNDCHIP_FDS)
		{
			mused.song.default_volume[i] = 0x40;
		}

		if(i > 2)
		{
			if(channels_to_chips[i] == FT_SNDCHIP_VRC6 && channels_to_chips[i - 1] == FT_SNDCHIP_VRC6 && channels_to_chips[i - 2] == FT_SNDCHIP_VRC6)
			{
				mused.song.default_volume[i] = 0x40; //sawtooth
			}
		}
	}

	mused.song.default_volume[4] = 0x80;
	mused.song.default_volume[2] = 0x50;
	mused.song.default_volume[3] = 0x1c;
}

//here we finish slide, vibrato, etc. commands
void finish_convert_effects() //this does not account for instrument macros changing pitch :(
{
	Uint8 song_speed = 0; //for calculating portamento note change per pattern row

	if(mused.song.flags & MUS_USE_GROOVE)
	{
		MusPattern* pat = &mused.song.pattern[0];
		MusStep* step = &pat->step[0];

		Uint8 groove = 0;

		for(int j = 0; j < MUS_MAX_COMMANDS; j++)
		{
			if((step->command[j] & 0xff00) == MUS_FX_SET_GROOVE)
			{
				groove = step->command[j] & 0xff;
			}

			Uint16 sum = 0;

			for(int i = 0; i < mused.song.groove_length[groove]; i++)
			{
				sum += mused.song.grooves[groove][i];
			}

			song_speed = sum / mused.song.groove_length[groove];
		}
	}

	else
	{
		song_speed = (mused.song.song_speed + mused.song.song_speed2) / 2;
	}

	if(song_speed == 0)
	{
		song_speed = 6;
	}

	for(int i = 0; i < mused.song.num_channels; i++) //add "default" pulse width commands for MMC5/2A03/VRC6 pulse channels so instruments that don't set pulse width play correctly at the beginning
	{
		MusPattern* pat = &mused.song.pattern[mused.song.sequence[i][0].pattern];
		MusStep* step = &pat->step[0];

		if(i == 0 || i == 1 || channels_to_chips[i] == FT_SNDCHIP_MMC5) //2A03 & MMC5 pulse channels
		{
			step->command[find_empty_command_column(step)] = MUS_FX_PW_SET | ft_2a03_pulse_widths[0];
		}

		if(i < mused.song.num_channels - 1) //so we don't go out of bounds
		{
			if(channels_to_chips[i] == FT_SNDCHIP_VRC6 && channels_to_chips[i + 1] == FT_SNDCHIP_VRC6) //VRC6 pulse channels
			{
				step->command[find_empty_command_column(step)] = MUS_FX_PW_SET | ft_vrc6_pulse_widths[0];
			}
		}
	}

	for(int i = 0; i < mused.song.num_channels; i++)
	{
		Uint16 current_note = 0xffff;
		bool do_portup = false;
		bool do_portdn = false;
		bool do_volume_slide = false;

		bool do_vibrato = false;

		bool place_command = true; //if you need to place command; reset on 1st command entry (e.g. 0220, not 0200) to avoid duplicate commands

		bool is_hardware_sweep = false; //if it is hardware sweep we stop when note is lower than C-0 or higher than idk B-8

		bool is_slide = false; //if need to fill in slide commands

		Uint8 volume_slide_param = 0;
		Uint8 port_speed = 0;
		Uint8 slide_speed = 0;

		for(int j = 0; j < sequence_length; j++)
		{
			MusPattern* pat = &mused.song.pattern[mused.song.sequence[i][j].pattern];

			for(int s = 0; s < pat->num_steps; s++)
			{
				MusStep* step = &pat->step[s];

				if(step->note < FREQ_TAB_SIZE) //an actual note, not note cut, note release or whatever
				{
					current_note = (Uint16)step->note << 8; //don't care about slide commands bruh

					if(is_hardware_sweep)
					{
						do_portup = false;
						do_portdn = false;
						is_hardware_sweep = false;
					}
				}

				bool proceed = true;

				place_command = true;

				for(int c = 0; c < MUS_MAX_COMMANDS; c++)
				{
					if(step->command[c] != 0)
					{
						Uint16 command = step->command[c];

						switch(step->command[c] & 0xff00)
						{
							case MUS_FX_SET_SPEED:
							{
								song_speed = command & 0xf; //keep track of speed changes
								break;
							}

							case MUS_FX_SET_GROOVE:
							{
								Uint8 groove = command & 0xff;

								Uint16 sum = 0;

								for(int i = 0; i < mused.song.groove_length[groove]; i++)
								{
									sum += mused.song.grooves[groove][i];
								}

								song_speed = sum / mused.song.groove_length[groove]; //keep track of groove changes

								break;
							}

							case MUS_FX_PORTA_UP:
							{
								if((step->command[MUS_MAX_COMMANDS - 1] == CONV_MARKER || step->command[MUS_MAX_COMMANDS - 1] == CONV_MARKER_SWEEP) && proceed)
								{
									if(command & 0xff)
									{
										do_portup = true;
										do_portdn = false;
										port_speed = command & 0xff;

										place_command = false;
									}

									else
									{
										do_portup = false;
										do_portdn = false;
										port_speed = 0;
									}

									if(step->command[MUS_MAX_COMMANDS - 1] == CONV_MARKER_SWEEP)
									{
										is_hardware_sweep = true;
										do_portdn = false;
									}

									else
									{
										is_hardware_sweep = false;
									}

									step->command[MUS_MAX_COMMANDS - 1] = 0;

									proceed = false;
								}

								break;
							}

							case MUS_FX_PORTA_DN:
							{
								if((step->command[MUS_MAX_COMMANDS - 1] == CONV_MARKER || step->command[MUS_MAX_COMMANDS - 1] == CONV_MARKER_SWEEP) && proceed)
								{
									if(command & 0xff)
									{
										do_portdn = true;
										do_portup = false;
										port_speed = command & 0xff;

										place_command = false;
									}

									else
									{
										do_portdn = false;
										do_portup = false;
										port_speed = 0;
									}

									if(step->command[MUS_MAX_COMMANDS - 1] == CONV_MARKER_SWEEP)
									{
										is_hardware_sweep = true;
										do_portup = false;
									}

									else
									{
										is_hardware_sweep = false;
									}

									step->command[MUS_MAX_COMMANDS - 1] = 0;

									proceed = false;
								}

								break;
							}

							case MUS_FX_VIBRATO:
							{
								if(command & 0xff)
								{
									do_vibrato = true;
								}

								else
								{
									do_vibrato = false;
								}

								break;
							}

							case MUS_FX_FADE_VOLUME:
							{
								if(command & 0xff)
								{
									do_volume_slide = true;
									volume_slide_param = command & 0xff;

									place_command = false;
								}

								else
								{
									do_volume_slide = false;
								}

								break;
							}

							case MUS_FX_FAST_SLIDE:
							{
								if(command & 0xff)
								{
									is_slide = true;
									slide_speed = command & 0xff;

									do_portdn = false;
									do_portup = false;
									port_speed = 0;

									//step->command[c] = 0; //delete the slide command so 1st note trigger is normal
								}

								else
								{
									is_slide = false;
								}

								break;
							}

							case MUS_FX_SLIDE_UP_SEMITONES:
							case MUS_FX_SLIDE_DN_SEMITONES:
							{
								is_slide = false;
								do_portdn = false;
								do_portup = false;

								break;
							}

							default: break;
						}
					}
				}

				if(song_speed == 0)
				{
					song_speed = 6;
				}

				if(place_command)
				{
					if(do_portup)
					{
						if(is_hardware_sweep)
						{
							if(current_note < ((12 * 7 + 11 + C_ZERO) << 8))
							{
								step->command[find_empty_command_column(step)] = MUS_FX_PORTA_UP | port_speed;
							}

							else
							{
								step->note = MUS_NOTE_CUT;
								do_portup = false;
								is_hardware_sweep = false;
							}
						}

						else
						{
							step->command[find_empty_command_column(step)] = MUS_FX_PORTA_UP | port_speed;
						}
					}

					if(do_portdn)
					{
						if(is_hardware_sweep)
						{
							if(current_note > ((C_ZERO) << 8))
							{
								step->command[find_empty_command_column(step)] = MUS_FX_PORTA_DN | port_speed;
							}

							else
							{
								step->note = MUS_NOTE_CUT;
								do_portdn = false;
								is_hardware_sweep = false;
							}
						}

						else
						{
							step->command[find_empty_command_column(step)] = MUS_FX_PORTA_DN | port_speed;
						}
					}

					if(do_volume_slide)
					{
						step->command[find_empty_command_column(step)] = MUS_FX_FADE_VOLUME | volume_slide_param;
					}
				}

				if(do_vibrato)
				{
					step->ctrl |= MUS_CTRL_VIB;
				}

				if(is_slide)
				{
					bool need_slide = true;

					for(int i = 0; i < MUS_MAX_COMMANDS; i++)
					{
						if((step->command[i] & 0xff00) == MUS_FX_FAST_SLIDE)
						{
							need_slide = false;
						}
					}

					if(step->note >= FREQ_TAB_SIZE)
					{
						need_slide = false;
					}

					if(need_slide)
					{
						step->command[find_empty_command_column(step)] = MUS_FX_FAST_SLIDE | slide_speed;
					}
				}

				for(int c = 0; c < MUS_MAX_COMMANDS; c++) //keep track of current note
				{
					if(step->command[c] != 0)
					{
						Uint16 command = step->command[c];

						switch(step->command[c] & 0xff00)
						{
							case MUS_FX_PORTA_UP:
							{
								current_note += (command & 0xff) * 4 * song_speed;
								break;
							}

							case MUS_FX_PORTA_DN:
							{
								current_note -= (command & 0xff) * 4 * song_speed;
								break;
							}

							default: break;
						}
					}
				}
			}
		}
	}
}

void do_fds_transpose()
{
	debug("Doing FDS transpose");

	int fds_ch = -1;

	for(int i = 0; i < mused.song.num_channels; i++)
	{
		if(channels_to_chips[i] == FT_SNDCHIP_FDS) //FDS channel
		{
			fds_ch = i;
		}
	}

	if(fds_ch == -1)
	{
		return;
	}

	for(int i = 0; i < num_instruments; i++)
	{
		MusInstrument* inst = &mused.song.instrument[i];

		ft_inst* ft_instrum = &ft_instruments[0];

		for(int j = 0; j < FT_MAX_INSTRUMENTS; j++)
		{
			if(ft_instruments[j].klystrack_instrument == i)
			{
				ft_instrum = &ft_instruments[j];
				goto next;
			}
		}

		next:;

		if(ft_instrum->type == INST_FDS)
		{
			for(int j = 0; j < inst->num_macros; j++)
			{
				for(int p = 0; p < MUS_PROG_LEN; p++)
				{
					if((inst->program[j][p] & 0xff00) == MUS_FX_ARPEGGIO_ABS)
					{
						Uint8 param = (inst->program[j][p] & 0xff) + 12 * 2;
						inst->program[j][p] &= 0xff00;
						inst->program[j][p] |= param;
					}
				}
			}
		}
	}

	Uint16 first = mused.song.sequence[fds_ch][0].pattern;
	Uint16 last = mused.song.sequence[fds_ch][mused.song.num_sequences[fds_ch] - 1].pattern;

	for(int i = first; i <= last; i++)
	{
		MusPattern* pat = &mused.song.pattern[i];

		for(int s = 0; s < pat->num_steps; s++)
		{
			MusStep* step = &pat->step[s];

			if(step->note < FREQ_TAB_SIZE)
			{
				step->note += 12 * 2;
			}
		}
	}
}

bool import_ft_old(FILE* f, int type, ftm_block* blocks, bool is_dn_ft_sig)
{
	bool success = true;
	
	return success;
}

bool import_ft_new(FILE* f, int type, ftm_block* blocks, bool is_dn_ft_sig)
{
	debug("using new module import routine");
	
	bool success = true;

	inst_numbers_to_inst_indices = (Uint8**)calloc(1, sizeof(Uint8*) * FT_MAX_INSTRUMENTS);
	
	for(int i = 0; i < FT_MAX_INSTRUMENTS; i++)
	{
		inst_numbers_to_inst_indices[i] = (Uint8*)calloc(1, sizeof(Uint8) * 2);
	}

	ft_sequence = (Uint8**)calloc(1, sizeof(Uint8*) * FT_MAX_FRAMES);

	for(int i = 0; i < FT_MAX_FRAMES; i++)
	{
		ft_sequence[i] = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_CHANNELS);
	}

	klystrack_sequence = (Uint16**)calloc(1, sizeof(Uint16*) * FT_MAX_FRAMES);

	for(int i = 0; i < FT_MAX_FRAMES; i++)
	{
		klystrack_sequence[i] = (Uint16*)calloc(1, sizeof(Uint16) * FT_MAX_CHANNELS);
	}

	initial_delta_counter_positions = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_INSTRUMENTS);

	effect_columns = (Uint8**)calloc(1, sizeof(Uint8*) * FT_MAX_SUBSONGS);

	for(int i = 0; i < FT_MAX_SUBSONGS; i++)
	{
		effect_columns[i] = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_CHANNELS);
	}

	ft_instruments = (ft_inst*)calloc(1, sizeof(ft_inst) * (FT_MAX_INSTRUMENTS * 4));

	for(int i = 0; i < FT_MAX_INSTRUMENTS; i++)
	{
		ft_instruments[i].klystrack_instrument = 0xff;
	}

	subsong_names = (char**)calloc(1, sizeof(char*) * FT_MAX_SUBSONGS);

	for(int i = 0; i < FT_MAX_SUBSONGS; i++)
	{
		subsong_names[i] = (char*)calloc(1, sizeof(char) * 4096);
		memset(subsong_names[i], 0, 4096);
	}

	channels_to_chips = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_CHANNELS);
	
	Uint16 num_blocks = 1;
	Uint16 current_block = 0;
	
	bool must_continue = true;
	
	while(must_continue)
	{
		memset(blocks[current_block].name, 0, FAMITRACKER_BLOCK_SIGNATURE_LENGTH + 1);
		Uint8 len = fread(blocks[current_block].name, 1, FAMITRACKER_BLOCK_SIGNATURE_LENGTH, f);
		
		if(len != FAMITRACKER_BLOCK_SIGNATURE_LENGTH)
		{
			must_continue = false;
		}
		
		if(memcmp(blocks[current_block].name, FT_END_SIG, strlen(FT_END_SIG)) == 0)
		{
			debug("END encountered");
			goto end;
		}
		
		read_uint32(f, &blocks[current_block].version);
		read_uint32(f, &blocks[current_block].length);
		
		blocks[current_block].position = ftell(f);
		
		fseek(f, blocks[current_block].length, SEEK_CUR);
		
		num_blocks++;
		current_block++;
		
		debug("BLOCK: sig \"%s\", size %d, version %X, position %X", blocks[current_block - 1].name, blocks[current_block - 1].length, blocks[current_block - 1].version, blocks[current_block - 1].position);
	}

	end:;

	debug("Got %d blocks", num_blocks);
	
	for(int i = 0; i < num_blocks; i++)
	{
		//debug("processing block %d", i);
		success = process_block(f, &blocks[i]);
	}

	convert_instruments();

	add_volumes();

	set_channel_volumes();

	finish_convert_effects();

	if(transpose_fds)
	{
		do_fds_transpose();
	}
	
	debug("finished processing blocks");

	mused.song.flags |= MUS_USE_OLD_SAMPLE_LOOP_BEHAVIOUR | MUS_USE_OLD_EFFECTS_BEHAVIOUR;

	return success;
}

int import_famitracker(FILE *f, int type)
{
	debug("starting famitracker module import...");
	
	bool success = true;
	bool is_dn_ft_sig = false;

	num_instruments = 0;

	selected_subsong = 0;
	sequence_length = 0;

	version = 0;
	namco_channels = 0;
	expansion_chips = 0;
	pattern_length = 0;
	num_subsongs = 1; // at least 1
	machine = 0;
	display_comment = 0;

	initial_delta_counter_positions = NULL;

	effect_columns = NULL;

	ft_instruments = NULL;

	inst_numbers_to_inst_indices = NULL;

	channels_to_chips = NULL;

	ft_sequence = NULL;
	klystrack_sequence = NULL;

	subsong_names = NULL;

	transpose_fds = false;
	
	Uint32 version = 0;

	is_dn_fami_module = false;
	
	ftm_block* blocks = NULL;
	blocks = (ftm_block*)calloc(1, sizeof(ftm_block) * 20);
	
	char header[256] = { 0 };
	
	fread(header, 1, strlen(FT_HEADER_SIG), f);
	
	if(strcmp(header, FT_HEADER_SIG) != 0)
	{
		fseek(f, 0, SEEK_SET);
		
		fread(header, 1, strlen(DN_FT_HEADER_SIG), f);
		
		if(strcmp(header, DN_FT_HEADER_SIG) != 0)
		{
			debug("No valid file signature found");
			success = false;
			goto end;
		}
		
		else
		{
			debug("Dn-FamiTracker signature found, proceeding...");
			is_dn_ft_sig = true;
			is_dn_fami_module = true;
		}
	}
	
	else
	{
		debug("FamiTracker signature found, proceeding...");
	}
	
	read_uint32(f, &version);
	
	debug("file version 0x%04X", version);
	
	if(version > FT_FILE_VER)
	{
		debug("file version too new!");
		success = false;
		goto end;
	}
	
	if(version < FT_FILE_LOWEST_VER)
	{
		debug("file version too old!");
		success = false;
		goto end;
	}
	
	if(version < 0x0200)
	{
		success = import_ft_old(f, type, blocks, is_dn_ft_sig);
	}
	
	else
	{
		success = import_ft_new(f, type, blocks, is_dn_ft_sig);
	}
	
	end:;
	
	free(blocks);

	if(initial_delta_counter_positions)
	{
		free(initial_delta_counter_positions);
	}

	if(ft_instruments)
	{
		free(ft_instruments);
	}

	if(effect_columns)
	{
		for(int i = 0; i < FT_MAX_SUBSONGS; i++)
		{
			free(effect_columns[i]);
		}
		
		free(effect_columns);
	}

	if(inst_numbers_to_inst_indices)
	{
		for(int i = 0; i < FT_MAX_INSTRUMENTS; i++)
		{
			free(inst_numbers_to_inst_indices[i]);
		}

		free(inst_numbers_to_inst_indices);
	}

	if(ft_sequence)
	{
		for(int i = 0; i < FT_MAX_FRAMES; i++)
		{
			free(ft_sequence[i]);
		}

		free(ft_sequence);
	}

	if(klystrack_sequence)
	{
		for(int i = 0; i < FT_MAX_FRAMES; i++)
		{
			free(klystrack_sequence[i]);
		}

		free(klystrack_sequence);
	}

	if(subsong_names)
	{
		for(int i = 0; i < FT_MAX_SUBSONGS; i++)
		{
			free(subsong_names[i]);
		}

		free(subsong_names);
	}

	if(channels_to_chips)
	{
		free(channels_to_chips);
	}

	if(success == false)
	{
		new_song();
	}

	else
	{
		if(display_comment)
		{
			//open_songmessage(0, 0, 0);
			mused.open_song_message = true;
		}
	}
	
	return ((success == true) ? 1 : 0);
}