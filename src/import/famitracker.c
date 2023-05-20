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

bool** seq_inst_enable;
Uint8** seq_inst_index;

Uint8 num_instruments;

Uint8** inst_numbers_to_inst_indices;

Uint8* initial_delta_counter_positions;

Uint8* instrument_types;

Uint8* effect_columns;

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

		seq_inst_enable[inst_num][i] = enable;

		Uint8 index = 0;
		fread(&index, 1, 1, f);

		seq_inst_index[inst_num][i] = index;
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

	return success;
}

bool load_inst_s5b(FILE* f, ftm_block* block, MusInstrument* inst, Uint8 inst_num)
{
	bool success = true;

	//debug("loading S5B instrument");

	load_sequence_indices(f, inst_num);

	return success;
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
};

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
};

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
};

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

		instrument_types[i] = inst_type;

		inst_numbers_to_inst_indices[i][0] = i;
		inst_numbers_to_inst_indices[i][1] = inst_index; //so we can remap inst numbers in patterns later
		
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
};

bool ft_process_sequencies_block(FILE* f, ftm_block* block)
{
	bool success = true;

	//debug("Processing 2A03 sequencies");

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
				if(seq_inst_index[j][0] == index && instrument_types[j] == INST_2A03) //idk bruh no macro type is provided in version 1 so maybe it's volume?
				{
					//TODO: reorder sequencies or whatever shit
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
				if(seq_inst_index[j][type] == index && instrument_types[j] == INST_2A03) //finally we were blessed with sequence type!
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
				if(seq_inst_index[j][type] == index && instrument_types[j] == INST_2A03)
				{
					memcpy(&ft_instruments[j].macros[type], temp_macro, sizeof(ft_inst_macro));

					/*debug("Macro %d type %d", Indices[i], Types[i]);

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
						if(seq_inst_index[k][j] == i && instrument_types[k] == INST_2A03)
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
					if(seq_inst_index[j][Types[i]] == Indices[i] && instrument_types[j] == INST_2A03)
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
};

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

			sequence_length = seq_len;

			if(block->version >= 3)
			{
				Uint32 tempo = 0;

				read_uint32(f, &tempo);

				mused.song.song_speed = mused.song.song_speed2 = speed;
			}

			else
			{
				if(speed < 20)
				{
					mused.song.song_rate = ((machine == FT_MACHINE_NTSC) ? 60 : 50);
					mused.song.song_speed = mused.song.song_speed2 = speed;
				}

				else
				{
					mused.song.song_speed = mused.song.song_speed2 = 6;
					mused.song.song_rate = 50;
				}
			}

			Uint32 pat_len = 0;
			read_uint32(f, &pat_len);

			pattern_length = pat_len;

			for(int j = 0; j < seq_len; j++)
			{
				for(int k = 0; k < mused.song.num_channels; k++)
				{
					Uint8 pattern = 0;
					fread(&pattern, sizeof(pattern), 1, f);

					ft_sequence[j][k] = pattern;
				}
			}
		}
	}
	
	return success;
};

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
	//and thus can't populate klystrack sequencies properly

	Uint16 ref_pattern = 0;
	Uint16 max_pattern = 0; //these are used to increment pattern number when we go to next channel since klystrack pattern indices are global
	//so we don't have intersecting sequencies
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

		if(max_pattern == 0)
		{
			max_pattern = 1; //so we don't have one pattern across multiple channels if one of the channels in sequence has just a bunch of `00` patterns
		}

		ref_pattern += max_pattern;
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

		debug("channel %d pattern %d len %d", channel, pattern, len);

		MusPattern* pat = &mused.song.pattern[0];

		for(int j = 0; j < sequence_length; j++)
		{
			if(ft_sequence[j][channel] == pattern)
			{
				pat = &mused.song.pattern[klystrack_sequence[j][channel]];
				break;
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

			//=========================================================

			if(vol == FT_VOLUME_NONE)
			{
				pat->step[row].volume = MUS_NOTE_NO_VOLUME;
			}

			else
			{
				pat->step[row].volume = convert_volumes_16[vol];
			}

			//=========================================================

			Uint8 num_fx = ((version == 0x0200) ? 1 : ((block->version >= 6) ? FT_MAX_EFFECT_COLUMNS : (effect_columns[channel] + 1)));

			//debug("num fx %d", num_fx);

			for(int j = 0; j < num_fx; j++)
			{
				fread(&effect, sizeof(effect), 1, f);

				if(effect)
				{
					fread(&param, sizeof(param), 1, f);
				}

				else if(block->version < 6)
				{
					Uint8 unused = 0;
					fread(&unused, sizeof(unused), 1, f); // unused blank parameter
					//bruh then how do you have the actual effect?!
				}

				pat->step[row].command[j] = ((Uint16)effect << 8) + param;
				//TODO: add effect conversion
			}
		}
	}
	
	return success;
};

bool ft_process_dpcm_samples_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

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

			effect_columns[i] = effect_cols;
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

				effect_columns[i] = effect_cols;
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
};

bool ft_process_comments_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_sequencies_vrc6_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_sequencies_n163_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_sequencies_n106_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_sequencies_s5b_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_detunetables_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_grooves_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_bookmarks_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_params_extra_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool (*block_parse_functions[FT_BLOCK_TYPES])(FILE* f, ftm_block* block) = 
{
	ft_process_params_block,
	ft_process_tuning_block,
	ft_process_info_block,
	ft_process_instruments_block,
	ft_process_sequencies_block,
	ft_process_frames_block,
	ft_process_patterns_block,
	ft_process_dpcm_samples_block,
	ft_process_header_block,
	ft_process_comments_block,
	ft_process_sequencies_vrc6_block,
	ft_process_sequencies_n163_block,
	ft_process_sequencies_n106_block,
	ft_process_sequencies_s5b_block,
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

bool import_ft_old(FILE* f, int type, ftm_block* blocks, bool is_dn_ft_sig)
{
	bool success = true;
	
	return success;
}

bool import_ft_new(FILE* f, int type, ftm_block* blocks, bool is_dn_ft_sig)
{
	debug("using new module import routine");
	
	bool success = true;

	seq_inst_enable = (bool**)calloc(1, sizeof(bool*) * FT_MAX_INSTRUMENTS);
	seq_inst_index = (Uint8**)calloc(1, sizeof(Uint8*) * FT_MAX_INSTRUMENTS);

	inst_numbers_to_inst_indices = (Uint8**)calloc(1, sizeof(Uint8*) * FT_MAX_INSTRUMENTS);
	
	for(int i = 0; i < FT_MAX_INSTRUMENTS; i++)
	{
		seq_inst_enable[i] = (bool*)calloc(1, sizeof(bool) * FT_SEQ_CNT);
		seq_inst_index[i] = (Uint8*)calloc(1, sizeof(Uint8) * FT_SEQ_CNT);

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

	instrument_types = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_INSTRUMENTS);

	effect_columns = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_CHANNELS);

	ft_instruments = (ft_inst*)calloc(1, sizeof(ft_inst) * FT_MAX_INSTRUMENTS);
	
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
	
	debug("finished processing blocks");

	return success;
}

int import_famitracker(FILE *f, int type)
{
	debug("starting famitracker module import...");
	
	bool success = true;
	bool is_dn_ft_sig = false;

	seq_inst_enable = NULL;
	seq_inst_index = NULL;
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
	instrument_types = NULL;

	effect_columns = NULL;

	ft_instruments = NULL;

	inst_numbers_to_inst_indices = NULL;

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

	if(instrument_types)
	{
		free(instrument_types);
	}

	if(ft_instruments)
	{
		free(ft_instruments);
	}

	if(effect_columns)
	{
		free(effect_columns);
	}

	if(seq_inst_enable)
	{
		for(int i = 0; i < FT_MAX_INSTRUMENTS; i++)
		{
			free(seq_inst_enable[i]);
		}

		free(seq_inst_enable);
	}

	if(seq_inst_index)
	{
		for(int i = 0; i < FT_MAX_INSTRUMENTS; i++)
		{
			free(seq_inst_index[i]);
		}

		free(seq_inst_index);
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

	if(success == false)
	{
		new_song();
	}
	
	return ((success == true) ? 1 : 0);
}