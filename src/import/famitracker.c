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

bool ft_process_params_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	debug("processing params block");
	
	Uint32 song_speed = 0;
	Uint32 machine = 0;
	Uint32 num_channels = 0;
	expansion_chips = 0;
	
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
	
	return success;
};

bool ft_process_tuning_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_info_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
	return success;
};

bool ft_process_instruments_block(FILE* f, ftm_block* block)
{
	bool success = true;
	
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
	
	Uint16 num_blocks = 1;
	Uint16 current_block = 0;
	
	bool must_continue = true;
	
	while(must_continue)
	{
		Uint64 len = fread(blocks[current_block].name, 1, FAMITRACKER_BLOCK_SIGNATURE_LENGTH, f);
		
		if(len != FAMITRACKER_BLOCK_SIGNATURE_LENGTH)
		{
			must_continue = false;
		}
		
		if(strcmp(blocks[current_block].name, END_SIG) == 0)
		{
			debug("END encountered");
			break;
		}
		
        read_uint32(f, &blocks[current_block].version);
		read_uint32(f, &blocks[current_block].length);
		
		blocks[current_block].position = ftell(f);
		
		fseek(f, blocks[current_block].length, SEEK_CUR);
		
		num_blocks++;
		current_block++;
		
		blocks = (ftm_block*)realloc(blocks, sizeof(ftm_block) * num_blocks);
		
		debug("BLOCK: sig \"%s\", size %d, version %X, position %X", blocks[current_block - 1].name, blocks[current_block - 1].length, blocks[current_block - 1].version, blocks[current_block - 1].position);
    }
	
	for(int i = 0; i < num_blocks; i++)
	{
		success = process_block(f, &blocks[i]);
		debug("processing block %d", i);
	}
	
	return success;
}

int import_famitracker(FILE *f, int type)
{
	debug("starting famitracker module import...");
	
	bool success = true;
	bool is_dn_ft_sig = false;
	
	Uint32 version = 0;
	
	ftm_block* blocks = (ftm_block*)calloc(1, sizeof(ftm_block));
	memset(blocks, 0, sizeof(ftm_block));
	
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
	
	if(blocks)
	{
		free(blocks);
	}
	
	return success ? 1 : 0;
}