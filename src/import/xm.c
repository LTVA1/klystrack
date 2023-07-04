/*
Copyright (c) 2009-2011 Tero Lindeman (kometbomb)

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

#include "xm.h"
#include "mod.h"
#include "edit.h"
#include "mused.h"
#include "event.h"
#include "SDL_endian.h"
#include "snd/freqs.h"
#include <assert.h>
#include <string.h>

extern Mused mused;

typedef struct
{
	Uint16 x, y;
} vol_env_point;

void find_command_xm(Uint16 command, MusStep* step)
{
	if ((command & 0xff00) == 0x0000 && (command & 0xff) != 0)
	{
		step->command[0] = MUS_FX_SET_EXT_ARP | (command & 0xff); 
		return;
	}
	
	else if ((command & 0xff00) == 0x0100 || (command & 0xff00) == 0x0200 || (command & 0xff00) == 0x0300)
	{
		step->command[0] = (command & 0xff00) | my_min(0xff, (command & 0xff)); // assuming linear xm freqs
		return;
	}
	
	else if ((command & 0xff00) == 0x0400)
	{
		step->command[0] = MUS_FX_VIBRATO | (command & 0xff);
		return;
	}
	
	else if ((command & 0xff00) == 0x0500)
	{
		step->ctrl |= MUS_CTRL_SLIDE;
		step->command[0] = MUS_FX_FADE_VOLUME | (my_min(0xf, (command & 0x0f) * 2)) | (my_min(0xf, ((command & 0xf0) >> 4) * 2) << 4);
		return;
	}
	
	else if ((command & 0xff00) == 0x0600)
	{
		step->ctrl |= MUS_CTRL_VIB;
		step->command[0] = MUS_FX_FADE_VOLUME | (my_min(0xf, (command & 0x0f) * 2)) | (my_min(0xf, ((command & 0xf0) >> 4) * 2) << 4);
		return;
	}
	
	else if ((command & 0xff00) == 0x0700)
	{
		step->command[0] = MUS_FX_TREMOLO | (command & 0xff);
		return;
	}
	
	else if ((command & 0xff00) == 0x0800)
	{
		step->command[0] = MUS_FX_SET_PANNING | (command & 0xff);
		return;
	}
	
	if ((command & 0xff00) == 0x0900)
	{ //the conversion will be finished later
		step->command[0] = MUS_FX_WAVETABLE_OFFSET | (command & 0xff);
		return;
	}
	
	else if ((command & 0xff00) == 0x0a00)
	{
		step->command[0] = MUS_FX_FADE_VOLUME | (my_min(0xf, (command & 0x0f) * 2)) | (my_min(0xf, ((command & 0xf0) >> 4) * 2) << 4);
		return;
	}
	
	//Bxx not currently supported in klystrack
	else if ((command & 0xff00) == 0x0b00) //Bxx finally :euphoria:
	{
		step->command[0] = MUS_FX_JUMP_SEQUENCE_POSITION | (command & 0xff);
		return ;
	}
	
	if ((command & 0xff00) == 0x0c00)
	{
		step->command[0] = MUS_FX_SET_VOLUME | ((command & 0xff) * 2);
		return;
	}
	
	else if ((command & 0xff00) == 0x0d00)
	{
		//step->command[0] = MUS_FX_SKIP_PATTERN | (command & 0xff);
		//command = MUS_FX_SKIP_PATTERN | (command & 0xff);

		Uint8 new_param = (command & 0xf) + ((command & 0xff) >> 4) * 10; //hex to decimal, Protracker (and XM too!) lol
		step->command[0] = MUS_FX_SKIP_PATTERN | new_param;
		return;
	}
	
	else if ((command & 0xfff0) == 0x0e10)
	{
		step->command[0] = MUS_FX_EXT_PORTA_UP | (command & 0xf);
		return;
	}
	
	else if ((command & 0xfff0) == 0x0e20)
	{
		step->command[0] = MUS_FX_EXT_PORTA_DN | (command & 0xf);
		return;
	}

	else if ((command & 0xfff0) == 0x0e30)
	{
		step->command[0] = MUS_FX_GLISSANDO_CONTROL | (command & 0xf);
		return;
	}
	
	else if ((command & 0xfff0) == 0x0e60)
	{
		step->command[0] = MUS_FX_FT2_PATTERN_LOOP | (command & 0xf);
		return;
	}
	
	else if ((command & 0xfff0) == 0x0e90)
	{
		step->command[0] = MUS_FX_EXT_RETRIGGER | (command & 0xf);
		return;
	}
	
	else if ((command & 0xfff0) == 0x0ea0 || (command & 0xfff0) == 0x0eb0)
	{
		step->command[0] = ((command & 0xfff0) == 0x0ea0 ? 0x0eb0 : 0x0ea0) | (my_min(0xf, (command & 0xf) * 2));
		return;
	}
	
	else if ((command & 0xfff0) == 0x0ec0)
	{
		step->command[0] = MUS_FX_EXT_NOTE_CUT | (command & 0xf);
		return;
	}
	
	else if ((command & 0xfff0) == 0x0ed0)
	{
		step->command[0] = MUS_FX_EXT_NOTE_DELAY | (command & 0xf);
		return;
	}
	
	else if ((command & 0xff00) == 0x0f00 && (command & 0xff) < 32)
	{
		step->command[0] = MUS_FX_SET_SPEED | my_min(0xf, (command & 0xff));
		return;
	}
	
	else if ((command & 0xff00) == 0x0f00 && (command & 0xff) >= 32)
	{
		step->command[0] = MUS_FX_SET_RATE | (((command & 0xff)) * 50 / 125);
		return;
	}
	
	else if ((command >> 8) == 'G')
	{
		step->command[0] = MUS_FX_SET_GLOBAL_VOLUME | ((command & 0xff) * 2);
		return;
	}
	
	else if ((command >> 8) == 'H')
	{
		step->command[0] = MUS_FX_FADE_GLOBAL_VOLUME | ((command & 0xff) * 2);
		return;
	}
	
	else if ((command >> 8) == 'K')
	{
		step->command[0] = MUS_FX_TRIGGER_RELEASE | (command & 0xff);
		return;
	}
	
	else if ((command >> 8) == 'P')
	{
		if(command & 0xf0) //pan right
		{
			step->command[0] = MUS_FX_PAN_RIGHT | ((command & 0xf0) >> 4);
			return;
		}
		
		if(command & 0xf) //pan left
		{
			if(step->command[0] == 0)
			{
				step->command[0] = MUS_FX_PAN_LEFT | (command & 0xf);
			}

			else
			{
				step->command[1] = MUS_FX_PAN_LEFT | (command & 0xf); //so we can have both commands if e.g. P8f is the orig command (makes no sense but theoretically such command can be encountered)
			}

			return;
		}
	}
	
	else if ((command >> 8) == 'X')
	{
		if(((command & 0xf0) >> 4) == 1)
		{
			step->command[0] = MUS_FX_EXT_FINE_PORTA_UP | (command & 0xf);
			return;
		}
		
		if(((command & 0xf0) >> 4) == 2)
		{
			step->command[0] = MUS_FX_EXT_FINE_PORTA_DN | (command & 0xf);
			return;
		}
	}
	
	else
	{
		command = 0;
		return;
	}
}


int import_xm(FILE *f)
{
	struct 
	{
		char sig[17];
		char name[20];
		Uint8 _1a;
		char tracker_name[20];
		Uint16 version;
		Uint32 header_size;
		Uint16 song_length;
		Uint16 restart_position;
		Uint16 num_channels;
		Uint16 num_patterns;
		Uint16 num_instruments;
		Uint16 flags;
		Uint16 default_tempo;
		Uint16 default_bpm;
		Uint8 pattern_order[256];
	} header;
	
	fread(&header.sig[0], 1, sizeof(header.sig), f);
	fread(&header.name[0], 1, sizeof(header.name), f);
	fread(&header._1a, 1, sizeof(header._1a), f);
	fread(&header.tracker_name[0], 1, sizeof(header.tracker_name), f);
	fread(&header.version, 1, sizeof(header.version), f);
	
	fread(&header.header_size, 1, sizeof(header.header_size), f);
	fread(&header.song_length, 1, sizeof(header.song_length), f);
	fread(&header.restart_position, 1, sizeof(header.restart_position), f);
	fread(&header.num_channels, 1, sizeof(header.num_channels), f);
	fread(&header.num_patterns, 1, sizeof(header.num_patterns), f);
	fread(&header.num_instruments, 1, sizeof(header.num_instruments), f);
	fread(&header.flags, 1, sizeof(header.flags), f);
	fread(&header.default_tempo, 1, sizeof(header.default_tempo), f);
	fread(&header.default_bpm, 1, sizeof(header.default_bpm), f);
	fread(&header.pattern_order[0], 1, sizeof(header.pattern_order), f);
	
	if (strncmp("Extended Module: ", header.sig, 17) != 0)
	{
		fatal("Not a FastTracker II module (sig: '%-17s')", header.sig);
		return 0;
	}
	
	FIX_ENDIAN(header.version);
	FIX_ENDIAN(header.header_size);
	FIX_ENDIAN(header.song_length);
	FIX_ENDIAN(header.restart_position);
	FIX_ENDIAN(header.num_channels);
	FIX_ENDIAN(header.num_instruments);
	FIX_ENDIAN(header.num_patterns);
	FIX_ENDIAN(header.flags);
	FIX_ENDIAN(header.default_tempo);
	FIX_ENDIAN(header.default_bpm);
	
	if (header.version != 0x0104)
	{
		fatal("XM version 0x%x not supported", header.version);
		return 0;
	}
	
	if ((int)header.num_channels * (int)header.num_patterns > NUM_PATTERNS)
	{
		fatal("Resulting song would have over %d patterns", NUM_PATTERNS);
		return 0;
	}
	
	if ((int)header.num_channels * (int)header.song_length > NUM_SEQUENCES)
	{
		fatal("Resulting song would have over %d sequence patterns", NUM_SEQUENCES);
		return 0;
	}
	
	int pos = 0;
	
	int pattern_length[256];
	
	for (int p = 0; p < header.num_patterns; ++p)
	{
		struct
		{
			Uint32 header_length;
			Uint8 packing_type;
			Uint16 num_rows;
			Uint16 data_size;
		} pattern_hdr;
	
		fread(&pattern_hdr.header_length, 1, sizeof(pattern_hdr.header_length), f);
		fread(&pattern_hdr.packing_type, 1, sizeof(pattern_hdr.packing_type), f);
		fread(&pattern_hdr.num_rows, 1, sizeof(pattern_hdr.num_rows), f);
		fread(&pattern_hdr.data_size, 1, sizeof(pattern_hdr.data_size), f);
		
		FIX_ENDIAN(pattern_hdr.data_size);
		FIX_ENDIAN(pattern_hdr.num_rows);
		FIX_ENDIAN(pattern_hdr.header_length);
		
		pattern_length[p] = pattern_hdr.num_rows;
		
		Uint8 data[256 * 32 * 5];
		
		debug("num_rows = %d", pattern_hdr.num_rows);
		
		fread(&data[0], 1, pattern_hdr.data_size, f);
		
		for (int c = 0; c < header.num_channels; ++c)
		{
			int pat = p * header.num_channels + c;
			resize_pattern(&mused.song.pattern[pat], pattern_hdr.num_rows);
		}
		
		Uint8 *ptr = &data[0];
		
		for (int r = 0; r < pattern_hdr.num_rows; ++r)
		{
			for (int c = 0; c < header.num_channels; ++c)
			{
				Uint8 note = *ptr++;
				Uint8 instrument = 0, volume = 0, fx = 0, param = 0;
				
				if (note & 0x80)
				{
					Uint8 flags = note;
					note = 0;
					
					if (flags & 1)
						note = *ptr++;
					
					if (flags & 2)
						instrument = *ptr++;
						
					if (flags & 4)
						volume = *ptr++;
						
					if (flags & 8)
						fx = *ptr++;
					
					if (flags & 16)
						param = *ptr++;
				}
				
				else
				{
					instrument = *ptr++;
					volume = *ptr++;
					fx = *ptr++;
					param = *ptr++;
				}
				
				int pat = p * header.num_channels + c;
				MusStep *step = &mused.song.pattern[pat].step[r];
				step->ctrl = 0;
				
				if (note != 0 && note != 97)
					step->note = (note - 1) + 12 * 5;
				else if (note == 97)
					step->note = MUS_NOTE_RELEASE;
				else
					step->note = MUS_NOTE_NONE;
				
				step->instrument = instrument != 0 ? instrument - 1 : MUS_NOTE_NO_INSTRUMENT;
				step->volume = MUS_NOTE_NO_VOLUME;
				
				if (volume >= 0x10 && volume <= 0x50)
				{
					step->volume = (volume - 0x10) * 2;
					goto next;
				}
				
				else if (volume >= 0xc0 && volume <= 0xcf)
				{
					step->volume = MUS_NOTE_VOLUME_SET_PAN | (volume & 0xf);
					goto next;
				}
				
				else if (volume >= 0xd0 && volume <= 0xdf)
				{
					step->volume = MUS_NOTE_VOLUME_PAN_LEFT | (volume & 0xf);
					goto next;
				}

				else if (volume >= 0xe0 && volume <= 0xef)
				{
					step->volume = MUS_NOTE_VOLUME_PAN_RIGHT | (volume & 0xf);
					goto next;
				}
				
				else if (volume >= 0x60 && volume <= 0x6f)
				{
					step->volume = MUS_NOTE_VOLUME_FADE_DN | (volume & 0xf);
					goto next;
				}

				else if (volume >= 0x70 && volume <= 0x7f)
				{
					step->volume = MUS_NOTE_VOLUME_FADE_UP | (volume & 0xf);
					goto next;
				}

				else if (volume >= 0x80 && volume <= 0x8f)
				{
					step->volume = MUS_NOTE_VOLUME_FADE_DN_FINE | (volume & 0xf);
					goto next;
				}

				else if (volume >= 0x90 && volume <= 0x9f)
				{
					if(volume & 0xf)
					{
						step->volume = MUS_NOTE_VOLUME_FADE_UP_FINE | (volume & 0xf);
					}

					goto next;
				}
				
				else if (volume >= 0xf0 && volume <= 0xff)
				{
					step->ctrl = MUS_CTRL_SLIDE|MUS_CTRL_LEGATO;
					goto next;
				}

				else if (volume >= 0xb0 && volume <= 0xbf)
				{
					step->ctrl = MUS_CTRL_VIB;
					goto next;
				}
				
				next:;
					
				find_command_xm((fx << 8) | param, step);
				
				if (volume >= 0x80 && volume <= 0x8f)
				{
					Uint16 vol_command = MUS_FX_EXT_FADE_VOLUME_DN | (volume & 0xf);
					
					if(step->command[0] == 0)
					{
						step->command[0] = vol_command;
						goto next2;
					}
					
					else if(step->command[1] == 0)
					{
						step->command[1] = vol_command;
						goto next2;
					}
					
					else if(step->command[2] == 0)
					{
						step->command[2] = vol_command;
						goto next2;
					}
				}
				
				else if (volume >= 0x90 && volume <= 0x9f)
				{
					Uint16 vol_command = MUS_FX_EXT_FADE_VOLUME_UP | (volume & 0xf);
					
					if(step->command[0] == 0)
					{
						step->command[0] = vol_command;
						goto next2;
					}
					
					else if(step->command[1] == 0)
					{
						step->command[1] = vol_command;
						goto next2;
					}
					
					else if(step->command[2] == 0)
					{
						step->command[2] = vol_command;
						goto next2;
					}
				}
				
				next2:;
			}
		}
	}
	
	for (int s = 0; s < header.song_length; ++s)
	{
		if (s == header.restart_position)
			mused.song.loop_point = pos;
	
		for (int c = 0; c < header.num_channels; ++c)
		{
			add_sequence(c, pos, header.pattern_order[s] * header.num_channels + c, 0);
		}
		
		pos += pattern_length[header.pattern_order[s]];
	}
	
	strncpy(mused.song.title, header.name, 20);
	mused.song.song_length = pos;
	mused.song.song_speed = mused.song.song_speed2 = header.default_tempo;
	mused.song.song_rate = (int)((double)header.default_bpm * 50.0 / 125.0);
	mused.song.num_channels = header.num_channels;
	mused.sequenceview_steps = 64;
	mused.song.num_patterns = header.num_patterns * header.num_channels;
	
	mused.song.flags |= MUS_USE_OLD_SAMPLE_LOOP_BEHAVIOUR;
	
	int wt_e = 0;
	
	for (int i = 0; i < header.num_instruments; ++i)
	{
		struct {
			Uint32 size;
			char name[22];
			Uint8 type;
			Uint16 num_samples;
		} instrument_hdr;
		
		struct {
			Uint32 size;
			Uint8 sample[96];
			//Uint8 vol_env[48];
			//Uint8 pan_env[48];
			
			vol_env_point vol_points[12];
			vol_env_point pan_points[12];
			
			Uint8 num_volume;
			Uint8 num_panning;
			Uint8 vol_sustain;
			Uint8 vol_loop_start, vol_loop_end;
			Uint8 pan_sustain;
			Uint8 pan_loop_start, pan_loop_end;
			Uint8 vol_type;
			Uint8 pan_type;
			Uint8 vib_type, vib_sweep, vib_depth, vib_rate;
			Uint16 vol_fadeout;
			Uint8 reserved[2];
		} instrument_ext_hdr;

		size_t si = ftell(f);
		fread(&instrument_hdr.size, 1, sizeof(instrument_hdr.size), f);
		fread(&instrument_hdr.name[0], 1, sizeof(instrument_hdr.name), f);
		fread(&instrument_hdr.type, 1, sizeof(instrument_hdr.type), f);
		fread(&instrument_hdr.num_samples, 1, sizeof(instrument_hdr.num_samples), f);
		
		FIX_ENDIAN(instrument_hdr.size);
		FIX_ENDIAN(instrument_hdr.num_samples);
		
		if (instrument_hdr.num_samples > 0)
		{
			fread(&instrument_ext_hdr.size, 1, sizeof(instrument_ext_hdr.size), f);
			fread(&instrument_ext_hdr.sample[0], 1, sizeof(instrument_ext_hdr.sample), f);
			fread(&instrument_ext_hdr.vol_points[0], 1, sizeof(instrument_ext_hdr.vol_points), f);
			fread(&instrument_ext_hdr.pan_points[0], 1, sizeof(instrument_ext_hdr.pan_points), f);
			fread(&instrument_ext_hdr.num_volume, 1, sizeof(instrument_ext_hdr.num_volume), f);
			fread(&instrument_ext_hdr.num_panning, 1, sizeof(instrument_ext_hdr.num_panning), f);
			fread(&instrument_ext_hdr.vol_sustain, 1, sizeof(instrument_ext_hdr.vol_sustain), f);
			fread(&instrument_ext_hdr.vol_loop_start, 1, sizeof(instrument_ext_hdr.vol_loop_start), f); 
			fread(&instrument_ext_hdr.vol_loop_end, 1, sizeof(instrument_ext_hdr.vol_loop_end), f);
			fread(&instrument_ext_hdr.pan_sustain, 1, sizeof(instrument_ext_hdr.pan_sustain), f);
			fread(&instrument_ext_hdr.pan_loop_start, 1, sizeof(instrument_ext_hdr.pan_loop_start), f); 
			fread(&instrument_ext_hdr.pan_loop_end, 1, sizeof(instrument_ext_hdr.pan_loop_end), f);
			fread(&instrument_ext_hdr.vol_type, 1, sizeof(instrument_ext_hdr.vol_type), f);
			fread(&instrument_ext_hdr.pan_type, 1, sizeof(instrument_ext_hdr.pan_type), f);
			fread(&instrument_ext_hdr.vib_type, 1, sizeof(instrument_ext_hdr.vib_type), f);
			fread(&instrument_ext_hdr.vib_sweep, 1, sizeof(instrument_ext_hdr.vib_sweep), f);
			fread(&instrument_ext_hdr.vib_depth, 1, sizeof(instrument_ext_hdr.vib_depth), f);
			fread(&instrument_ext_hdr.vib_rate, 1, sizeof(instrument_ext_hdr.vib_rate), f);
			fread(&instrument_ext_hdr.vol_fadeout, 1, sizeof(instrument_ext_hdr.vol_fadeout), f);
			fread(&instrument_ext_hdr.reserved[0], 1, sizeof(instrument_ext_hdr.reserved), f);
			
			if(instrument_ext_hdr.vol_type & 1) //if volume envelope is used
			{
				mused.song.instrument[i].flags |= MUS_INST_USE_VOLUME_ENVELOPE;
				mused.song.instrument[i].vol_env_fadeout = instrument_ext_hdr.vol_fadeout;
				mused.song.instrument[i].num_vol_points = instrument_ext_hdr.num_volume;
				mused.song.instrument[i].vol_env_loop_start = instrument_ext_hdr.vol_loop_start;
				mused.song.instrument[i].vol_env_loop_end = instrument_ext_hdr.vol_loop_end;
				mused.song.instrument[i].vol_env_sustain = instrument_ext_hdr.vol_sustain;
				mused.song.instrument[i].vol_env_flags = (instrument_ext_hdr.vol_type >> 1);
				
				for(int j = 0; j < mused.song.instrument[i].num_vol_points; ++j)
				{
					mused.song.instrument[i].volume_envelope[j].x = (Uint16)((double)(instrument_ext_hdr.vol_points[j].x << 1) * 50.0 / mused.song.song_rate);
					mused.song.instrument[i].volume_envelope[j].y = instrument_ext_hdr.vol_points[j].y << 1;
				}
			}
			
			if(instrument_ext_hdr.pan_type & 1) //if panning envelope is used
			{
				mused.song.instrument[i].flags |= MUS_INST_USE_PANNING_ENVELOPE;
				
				mused.song.instrument[i].num_pan_points = instrument_ext_hdr.num_panning;
				mused.song.instrument[i].pan_env_loop_start = instrument_ext_hdr.pan_loop_start;
				mused.song.instrument[i].pan_env_loop_end = instrument_ext_hdr.pan_loop_end;
				mused.song.instrument[i].pan_env_sustain = instrument_ext_hdr.pan_sustain;
				mused.song.instrument[i].pan_env_flags = (instrument_ext_hdr.pan_type >> 1);
				
				for(int j = 0; j < mused.song.instrument[i].num_pan_points; ++j)
				{
					mused.song.instrument[i].panning_envelope[j].x = (Uint16)((double)(instrument_ext_hdr.pan_points[j].x << 1) * 50.0 / mused.song.song_rate);
					mused.song.instrument[i].panning_envelope[j].y = instrument_ext_hdr.pan_points[j].y << 1;
				}
			}
			
			fseek(f, si + instrument_hdr.size, SEEK_SET);
			
			FIX_ENDIAN(instrument_ext_hdr.vol_fadeout);
			
			Uint32 first_length = 0, total_length = 0, type = 0;
			Sint32 fine = 0, loop_begin = 0, loop_len = 0;
			
			for (int s = 0; s < instrument_hdr.num_samples; ++s)
			{
				struct 
				{
					Uint32 sample_length;
					Uint32 sample_loop_start;
					Uint32 sample_loop_length;
					Uint8 volume;
					Sint8 finetune;
					Uint8 type;
					Uint8 panning;
					Uint8 relative_note;
					Uint8 reserved;
					char name[22];
				} sample_hdr;
				
				fread(&sample_hdr.sample_length, 1, sizeof(sample_hdr.sample_length), f);
				fread(&sample_hdr.sample_loop_start, 1, sizeof(sample_hdr.sample_loop_start), f);
				fread(&sample_hdr.sample_loop_length, 1, sizeof(sample_hdr.sample_loop_length), f);
				fread(&sample_hdr.volume, 1, sizeof(sample_hdr.volume), f);
				fread(&sample_hdr.finetune, 1, sizeof(sample_hdr.finetune), f);
				fread(&sample_hdr.type, 1, sizeof(sample_hdr.type), f);
				fread(&sample_hdr.panning, 1, sizeof(sample_hdr.panning), f);
				fread(&sample_hdr.relative_note, 1, sizeof(sample_hdr.relative_note), f);
				fread(&sample_hdr.reserved, 1, sizeof(sample_hdr.reserved), f);
				fread(&sample_hdr.name[0], 1, sizeof(sample_hdr.name), f);
				
				FIX_ENDIAN(sample_hdr.sample_length);
				FIX_ENDIAN(sample_hdr.sample_loop_start);
				FIX_ENDIAN(sample_hdr.sample_loop_length);
				
				total_length += sample_hdr.sample_length;
				
				if (s > 0) continue; // read only first sample
				
				first_length = sample_hdr.sample_length;
				type = sample_hdr.type;
				
				mused.song.instrument[i].volume = sample_hdr.volume * 2;
				mused.song.instrument[i].base_note = MIDDLE_C + sample_hdr.relative_note;
				
				fine = sample_hdr.finetune;
				
				loop_begin = sample_hdr.sample_loop_start;
				loop_len = sample_hdr.sample_loop_length;
			}
			
			if (first_length > 0)
			{
				Sint8 *smp = calloc(1, first_length);
				
				fread(smp, 1, first_length, f);
				
				if (type & 16)
				{
					debug("16-bit sample");
					
					int x = 0;
					for (int idx = 0; idx < first_length / 2; ++idx)
					{
						x += ((Uint16*)smp)[idx];
						((Uint16*)smp)[idx] = x;
					}
					
					cyd_wave_entry_init(&mused.mus.cyd->wavetable_entries[wt_e], smp, first_length / 2, CYD_WAVE_TYPE_SINT16, 1, 1, 1);
				}
				
				else
				{
					debug("8-bit sample");
					
					int x = 0;
					for (int idx = 0; idx < first_length; ++idx)
					{
						x += smp[idx];
						smp[idx] = x;
					}
					
					cyd_wave_entry_init(&mused.mus.cyd->wavetable_entries[wt_e], smp, first_length, CYD_WAVE_TYPE_SINT8, 1, 1, 1);
				}
				
				free(smp);
				
				mused.mus.cyd->wavetable_entries[wt_e].loop_begin = loop_begin;
				mused.mus.cyd->wavetable_entries[wt_e].loop_end = loop_begin + loop_len;
				
				mused.mus.cyd->wavetable_entries[wt_e].loop_begin = my_min(mused.mus.cyd->wavetable_entries[wt_e].loop_begin, mused.mus.cyd->wavetable_entries[wt_e].samples - 1);
				mused.mus.cyd->wavetable_entries[wt_e].loop_end = my_min(mused.mus.cyd->wavetable_entries[wt_e].loop_end, mused.mus.cyd->wavetable_entries[wt_e].samples);
				
				mused.song.instrument[i].cydflags = CYD_CHN_ENABLE_WAVE | CYD_CHN_WAVE_OVERRIDE_ENV | CYD_CHN_ENABLE_KEY_SYNC;
				mused.song.instrument[i].flags |= MUS_INST_SET_PW | MUS_INST_SET_CUTOFF;
				mused.song.instrument[i].flags &= ~(MUS_INST_DRUM);
				
				if(mused.song.instrument[i].flags & MUS_INST_USE_VOLUME_ENVELOPE)
				{
					mused.song.instrument[i].cydflags &= ~(CYD_CHN_WAVE_OVERRIDE_ENV);
				}
				
				mused.song.instrument[i].vibrato_speed = instrument_ext_hdr.vib_rate;
				mused.song.instrument[i].vibrato_depth = instrument_ext_hdr.vib_depth;
				mused.song.instrument[i].vibrato_delay = instrument_ext_hdr.vib_sweep;
				
				// from mod.c
				mused.mus.cyd->wavetable_entries[wt_e].base_note = (MIDDLE_C << 8) - (Sint16)fine;
				mused.mus.cyd->wavetable_entries[wt_e].sample_rate = 7093789.2 / 856;
				
				switch (type & 3)
				{
					case 0: mused.mus.cyd->wavetable_entries[wt_e].flags &= ~CYD_WAVE_LOOP; break;
					case 1: mused.mus.cyd->wavetable_entries[wt_e].flags |= CYD_WAVE_LOOP; break;
					case 2: mused.mus.cyd->wavetable_entries[wt_e].flags |= CYD_WAVE_LOOP | CYD_WAVE_PINGPONG; break;
				}
				
				mused.song.instrument[i].wavetable_entry = wt_e++;
				
				strncpy(mused.song.instrument[i].name, instrument_hdr.name, 22);
			}
			
			fseek(f, total_length - first_length, SEEK_CUR);
		}
		
		else 
		{
			fseek(f, si + instrument_hdr.size, SEEK_SET);
		}
	}
	
	for(int pat = 0; pat < mused.song.num_patterns; ++pat) //now we know sample length
	{ //in FT2 `901` means 0x100 steps offset from sample start
		for(int s = 0; s < mused.song.pattern[pat].num_steps; ++s)
		{ //but in klystrack `5001` means 1/(0x1000)th of sample length so we need to recalculate
			MusStep *step = &mused.song.pattern[pat].step[s];
			
			//if the instrument is specified and it actually has a sample
			if((step->command[0] & 0xf000) == MUS_FX_WAVETABLE_OFFSET && step->instrument != MUS_NOTE_NO_INSTRUMENT && mused.mus.cyd->wavetable_entries[mused.song.instrument[step->instrument].wavetable_entry].data)
			{
				Uint8 init_offset = step->command[0] & 0xff;
				
				Uint16 klystrack_offset = (Uint64)(init_offset) * 256 * 0x1000 / (Uint64)mused.mus.cyd->wavetable_entries[mused.song.instrument[step->instrument].wavetable_entry].samples;
				
				step->command[0] = MUS_FX_WAVETABLE_OFFSET | (klystrack_offset & 0xfff);
			}
		}
	}
	
	return 1;
}
