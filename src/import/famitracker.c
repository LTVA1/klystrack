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

const Uint8 noise_notes[16] = 
{ 
	C_ZERO - 3, C_ZERO + 9, C_ZERO + 12 + 9, C_ZERO + 12 * 2 + 1, C_ZERO + 12 * 2 + 9, C_ZERO + 12 * 3 + 2, 
	C_ZERO + 12 * 3 + 5, C_ZERO + 12 * 3 + 11, C_ZERO + 12 * 4 + 3, C_ZERO + 12 * 4 + 5, C_ZERO + 12 * 4 + 11, 
	C_ZERO + 12 * 5 + 5, C_ZERO + 12 * 5 + 11, C_ZERO + 12 * 6 + 7, C_ZERO + 12 * 7 + 11, C_ZERO + 12 * 9 + 11
};

void read_uint32(FILE *f, Uint32* number) //little-endian
{
	*number = 0;
	Uint8 temp = 0;
	fread(&temp, 1, 1, f);
	*number |= temp;
	fread(&temp, 1, 1, f);
	*number |= ((Uint32)temp << 8);
	fread(&temp, 1, 1, f);
	*number |= ((Uint32)temp << 16);
	fread(&temp, 1, 1, f);
	*number |= ((Uint32)temp << 24);
}

Uint8 expansion_chips;
Uint8 namco_channels;
Uint32 version;

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
	}
}

void load_dpcm_sample_map(Uint8 note, Uint8 octave, MusInstrument* inst, FILE* f, ftm_block* block) //const auto ReadAssignment = [&] (int Octave, int Note)
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

	fread(&pitch, 1, 1, f);

	if(block->version > 5)
	{
		fread(&initial_delta_counter_position, 1, 1, f);
		initial_delta_counter_positions[index] = initial_delta_counter_position;
	}

	inst->note_to_sample_array[C_ZERO + octave * 12 + note].sample = index;
	inst->note_to_sample_array[C_ZERO + octave * 12 + note].flags |= MUS_NOTE_TO_SAMPLE_GLOBAL;
}

bool load_inst_2a03(FILE* f, ftm_block* block, MusInstrument* inst, Uint8 inst_num)
{
	bool success = true;

	//debug("loading 2A03 instrument");

	load_sequence_indices(f, inst_num);

	if(block->version >= 7)
	{
		Uint32 num_notes = 0;

		read_uint32(f, &num_notes);

		for(int i = 0; i < num_notes; i++)
		{
			Uint8 note = 0;

			fread(&note, 1, 1, f);

			load_dpcm_sample_map(note % 12, note / 12, inst, f, block);
		}
	}

	else
	{
		for(int i = 0; i < (block->version == 1 ? 6 * 12 : 8 * 12); i++)
		{
			load_dpcm_sample_map(i % 12, i / 12, inst, f, block);
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

		if(block->version > 2)
		{
			ft_load_fds_sequence(f, inst_num, FT_SEQ_PITCH);
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
		bool sweep_reset = false;
		
		Uint32 temp = 0;
		read_uint32(f, &temp);
		
		sweep_reset = temp != 0 ? true : false; //unused in klystrack
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
				success = load_inst_2a03(f, block, &mused.song.instrument[i], inst_index);
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

			for(int j = 0; j < num_instruments; j++)
			{
				if(ft_instruments[j].seq_indices[type] == index && ft_instruments[j].type == INST_2A03)
				{
					memcpy(&ft_instruments[j].macros[type], temp_macro, sizeof(ft_inst_macro));

					if(ft_instruments[j].seq_enable[type])
					{
						/*debug("Macro %d type %d instrument %d klystrack instrument %d", Indices[i], Types[i], j, ft_instruments[j].klystrack_instrument);

						for(int k = 0; k < ft_instruments[j].macros[type].sequence_size; k++)
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

					for(int k = 0; k < num_instruments; k++)
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

				for(int j = 0; j < num_instruments; j++)
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

bool ft_process_patterns_block(FILE* f, ftm_block* block)
{
	bool success = true;

	if(block->version == 1)
	{
		Uint32 pat_len = 0;
		read_uint32(f, &pat_len);
		pattern_length = pat_len;
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
			add_sequence(i, j * pattern_length, ft_sequence[j][i] + ref_pattern, 0);
			resize_pattern(&mused.song.pattern[ft_sequence[j][i] + ref_pattern], pattern_length);

			klystrack_sequence[j][i] = ft_sequence[j][i] + ref_pattern;

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
					break;
				}
			}
		}

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
			Uint8 effect = 0;
			Uint8 param = 0;

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
					pat->step[row].note = (note - 1) + octave * 12 + C_ZERO;
				}

				if(channel == 3) //noise channel
				{
					pat->step[row].note = noise_notes[(note + octave * 12) - (12 + 5)];
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
				pat->command_columns = effect_columns[subsong_index][channel];
			}

			for(int j = 0; j < num_fx; j++)
			{
				fread(&effect, sizeof(effect), 1, f);

				if(effect)
				{
					fread(&param, sizeof(param), 1, f);

					if(block->version < 3)
					{
						if(effect == FT_EF_PORTAOFF)
						{
							effect = FT_EF_PORTAMENTO;
							param = 0;
						}

						else if(effect == FT_EF_PORTAMENTO)
						{
							if(param < 0xFF)
							{
								param++;
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
					if(j == 0 && effect == FT_EF_SPEED && param < 20)
					{
						param++;
					}
				}

				if(subsong_index == selected_subsong && effect != FT_EF_NONE)
				{
					pat->step[row].command[j] = ((Uint16)effect << 8) + param;
					//TODO: add effect conversion
				}
			}

			if(subsong_index == selected_subsong)
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

				if((expansion_chips & FT_SNDCHIP_N163) && channels_to_chips[channel] == FT_SNDCHIP_N163)
				{
					for(int j = 0; j < FT_MAX_EFFECT_COLUMNS; j++)
					{
						if((pat->step[row].command[j] >> 8) == FT_EF_SAMPLE_OFFSET)
						{
							pat->step[row].command[j] &= 0x00ff;
							pat->step[row].command[j] |= (FT_EF_N163_WAVE_BUFFER << 8);
						}
					}
				}

				if(block->version == 3)
				{
					if((expansion_chips & FT_SNDCHIP_VRC7) && channel > 4) // Fix for VRC7 portamento
					{
						for(int j = 0; j < FT_MAX_EFFECT_COLUMNS; j++)
						{
							if((pat->step[row].command[j] >> 8) == FT_EF_PORTA_DOWN)
							{
								pat->step[row].command[j] &= 0x00ff;
								pat->step[row].command[j] |= (FT_EF_PORTA_UP << 8);
							}

							if((pat->step[row].command[j] >> 8) == FT_EF_PORTA_UP)
							{
								pat->step[row].command[j] &= 0x00ff;
								pat->step[row].command[j] |= (FT_EF_PORTA_DOWN << 8);
							}
						}
					}

					else if((expansion_chips & FT_SNDCHIP_FDS) && channels_to_chips[channel] == FT_SNDCHIP_FDS) // FDS pitch effect fix
					{
						for(int j = 0; j < FT_MAX_EFFECT_COLUMNS; j++)
						{
							if((pat->step[row].command[j] >> 8) == FT_EF_PITCH)
							{
								if((pat->step[row].command[j] & 0xff) != 0x80)
								{
									Uint8 param = pat->step[row].command[j];
									pat->step[row].command[j] &= 0xff00;
									pat->step[row].command[j] |= (0x100 - param);
								}
							}
						}
					}
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

		mused.cyd.wavetable_entries[index].sample_rate = machine == FT_MACHINE_PAL ? dpcm_sample_rates_pal[15] : dpcm_sample_rates_ntsc[15];
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
		//TODO: add subsong selection dialog, import sequence and patterns accordingly

		if(block->version >= 3)
		{
			for(int i = 0; i < num_subsongs; i++) //track names, fuck it, reading just to stay aligned
			{
				//pDocFile->ReadString())
				Uint8 charaa = 1;

				do //hope this works as null-terminated string reader mockup
				{
					fread(&charaa, sizeof(charaa), 1, f);
				} while(charaa != 0);
			}
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

	Uint8 display_comment = 0;
	fread(&display_comment, sizeof(display_comment), 1, f);

	char* comments_string = (char*)calloc(1, 1);
	Uint32 index = 0;

	do //hope this works as null-terminated string reader
	{
		fread(&comments_string[index], sizeof(char), 1, f);
		index++;
		comments_string = (char*)realloc(comments_string, index + 1);
	} while(comments_string[index] != 0);


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

		for(int j = 0; j < num_instruments; j++)
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

				for(int k = 0; k < num_instruments; k++)
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

			for(int j = 0; j < num_instruments; j++)
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

		for(int j = 0; j < num_instruments; j++)
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

		for(int j = 0; j < num_instruments; j++)
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

	mused.song.flags |= MUS_USE_GROOVE;

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

void convert_instruments()
{
	debug("converting instruments");

	for(int i = 0; i < num_instruments; i++)
	{
		MusInstrument* inst = &mused.song.instrument[i];

		inst->flags &= ~(MUS_INST_DRUM);
		inst->flags |= MUS_INST_RELATIVE_VOLUME;

		inst->cydflags &= ~(CYD_CHN_ENABLE_KEY_SYNC | CYD_CHN_ENABLE_TRIANGLE);
		inst->cydflags |= CYD_CHN_ENABLE_PULSE;
		inst->pw = 0x0dff;

		inst->adsr.a = 0;
		inst->adsr.d = 0;
		inst->adsr.s = 0x1f;
	}

	for(int i = 0; i < num_instruments; i++)
	{
		Uint8 current_program = 0;
		Uint8 ft_inst_index = 0;

		MusInstrument* inst = &mused.song.instrument[i];

		ft_inst* ft_instrum = &ft_instruments[0];

		for(int j = 0; j < FT_MAX_INSTRUMENTS; j++)
		{
			if(ft_instruments[j].klystrack_instrument == i)
			{
				ft_instrum = &ft_instruments[j];
				ft_inst_index = j;
				goto next;
			}
		}

		next:;

		switch(ft_instrum->type)
		{
			case INST_2A03:
			{
				if(ft_instrum->seq_enable[0] && ft_instrum->macros[0].sequence_size > 0)
				{
					for(int p = 0; p < ft_instrum->macros[0].sequence_size; p++)
					{
						inst->program[current_program][p] = MUS_FX_SET_VOLUME | convert_volumes_16[ft_instrum->macros[0].sequence[p]];
					}

					strcpy(inst->program_names[current_program], sequence_names[0]);
				}

				break;
			}

			case INST_VRC6:
			{
				if(ft_instrum->seq_enable[0] && ft_instrum->macros[0].sequence_size > 0)
				{
					if(ft_instrum->macros[0].setting == FT_SETTING_VOL_64_STEPS)
					{
						for(int p = 0; p < ft_instrum->macros[0].sequence_size; p++)
						{
							inst->program[current_program][p] = MUS_FX_SET_VOLUME | convert_volumes_64[ft_instrum->macros[0].sequence[p]];
						}
					}

					else
					{
						for(int p = 0; p < ft_instrum->macros[0].sequence_size; p++)
						{
							inst->program[current_program][p] = MUS_FX_SET_VOLUME | convert_volumes_16[ft_instrum->macros[0].sequence[p]];
						}
					}

					strcpy(inst->program_names[current_program], sequence_names_vrc6[0]);
				}

				break;
			}

			default: break;
		}

		for(int j = 0; j < FT_SEQ_CNT; j++)
		{
			
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

	ft_instruments = (ft_inst*)calloc(1, sizeof(ft_inst) * FT_MAX_INSTRUMENTS);

	for(int i = 0; i < FT_MAX_INSTRUMENTS; i++)
	{
		ft_instruments[i].klystrack_instrument = 0xff;
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
	
	debug("finished processing blocks");

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

	initial_delta_counter_positions = NULL;

	effect_columns = NULL;

	ft_instruments = NULL;

	inst_numbers_to_inst_indices = NULL;

	channels_to_chips = NULL;

	ft_sequence = NULL;
	klystrack_sequence = NULL;
	
	Uint32 version = 0;
	
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

	if(channels_to_chips)
	{
		free(channels_to_chips);
	}

	if(success == false)
	{
		new_song();
	}
	
	return ((success == true) ? 1 : 0);
}