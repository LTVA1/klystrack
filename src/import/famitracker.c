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

bool** seq_inst_enable;
Uint8** seq_inst_index;

Uint8 num_instruments;

Uint8** inst_numbers_to_inst_indices;

Uint8* initial_delta_counter_positions;

Uint8* instrument_types;

ft_inst* ft_instruments;

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

	debug("loading 2A03 instrument");

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

	debug("loading VRC6 instrument");

	load_sequence_indices(f, inst_num);

	return success;
}

bool load_inst_vrc7(FILE* f, ftm_block* block, MusInstrument* inst, Uint8 inst_num)
{
	bool success = true;

	debug("loading VRC7 instrument");

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

	debug("loading FDS instrument");

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

	debug("loading N163 instrument");

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

	debug("loading S5B instrument");

	load_sequence_indices(f, inst_num);

	return success;
}

bool ft_process_params_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	debug("processing params block");
	
	Uint32 song_speed = 0;
	Uint32 machine = 0;
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
	
	read_uint32(f, &machine);
	
	if(machine == FT_MACHINE_NTSC)
	{
		mused.song.song_rate = 60;
	}
	
	else if(machine == FT_MACHINE_PAL)
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

	debug("song has %d instruments", num_insts);

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

		debug("instrument type %d", inst_type);
		
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
		
		debug("inst name len %d", inst_name_len);

		fread(mused.song.instrument[i].name, inst_name_len, 1, f);
	}
	
	end:;
	
	return success;
};

bool ft_process_sequencies_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_frames_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_patterns_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
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
			debug("calling block processing function");
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

	initial_delta_counter_positions = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_INSTRUMENTS);

	instrument_types = (Uint8*)calloc(1, sizeof(Uint8) * FT_MAX_INSTRUMENTS);

	ft_instruments = (Uint8*)calloc(1, sizeof(ft_inst) * FT_MAX_INSTRUMENTS);
	
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
		debug("processing block %d", i);
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

	initial_delta_counter_positions = NULL;
	instrument_types = NULL;

	ft_instruments = NULL;

	inst_numbers_to_inst_indices = NULL;
	
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

	if(success == false)
	{
		new_song();
	}
	
	return ((success == true) ? 1 : 0);
}