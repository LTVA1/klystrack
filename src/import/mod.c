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

#include "mod.h"
#include "edit.h"
#include "mused.h"
#include "event.h"
#include "SDL_endian.h"
#include "snd/freqs.h"
#include <assert.h>
#include <string.h>

#include "importutil.h"

extern Mused mused;

static Uint8 find_note(Uint16 period)
{
	static const Uint16 periods[] = 
	{
		856,808,762,720,678,640,604,570,538,508,480,453,
		428,404,381,360,339,320,302,285,269,254,240,226,
		214,202,190,180,170,160,151,143,135,127,120,113,
		0
	};
	
	if (period == 0) return MUS_NOTE_NONE;
	
	for (int i = 0; periods[i]; ++i)
		if (periods[i] == period) return i + MIDDLE_C - 12;
		
	return MUS_NOTE_NONE;
}


int import_mod(FILE *f)
{
	char ver[4];
	
	fseek(f, 1080, SEEK_SET);
	fread(ver, 1, sizeof(ver), f);
	
	int channels = 0, instruments = 15;
	
	static const struct { int chn, inst; char *sig; } specs[] =
	{
		{ 4, 31, "M.K." },
		{ 4, 31, "M!K!" },
		{ 4, 31, "FLT4" },
		{ 8, 31, "FLT8" },
		{ 4, 31, "4CHN" },
		{ 6, 31, "6CHN" },
		{ 8, 31, "8CHN" },
		{ 0 }
	};
	
	for (int i = 0; specs[i].chn; ++i)
	{
		if (strncmp(specs[i].sig, ver, 4) == 0)
		{
			channels = specs[i].chn;
			instruments = specs[i].inst;
			break;
		}
	}
	
	if (channels == 0) 
	{
		warning("No file signature found: assuming Soundtracker format");
		channels = 4;
	}
	
	fseek(f, 0, SEEK_SET);
	fread(mused.song.title, 20, sizeof(char), f);
	
	Uint16 sample_length[32], loop_begin[32], loop_len[32];
	Sint8 fine[32];
	
	int wt_e = 0;
	
	for (int i = 0; i < instruments; ++i)
	{
		mused.song.instrument[i].flags = 0;
		mused.song.instrument[i].cydflags = 0;
	
		char name[MUS_INSTRUMENT_NAME_LEN + 1] = { 0 };
		fread(name, 1, 22, f);
		name[22] = '\0';
		strcpy(mused.song.instrument[i].name, name);
		
		fread(&sample_length[i], 2, 1, f);
		fread(&fine[i], 1, 1, f);
		
		fine[i] = ((fine[i] & 0xf) << 4);
		
		sample_length[i] = SDL_SwapBE16(sample_length[i]) * 2;
		
		if (sample_length[i] > 1)
		{
			mused.song.instrument[i].cydflags |= CYD_CHN_ENABLE_WAVE | CYD_CHN_WAVE_OVERRIDE_ENV | CYD_CHN_ENABLE_KEY_SYNC;
			mused.song.instrument[i].flags |= MUS_INST_SET_PW | MUS_INST_SET_CUTOFF;
			mused.song.instrument[i].wavetable_entry = wt_e++;
		}
		
		//mused.song.instrument[i].prog_period[0] = 1;
		//mused.song.instrument[i].program[0][0] = MUS_FX_ARPEGGIO;
		//mused.song.instrument[i].program[0][1] = MUS_FX_ARPEGGIO | 0xf0;
		//mused.song.instrument[i].program[0][2] = MUS_FX_ARPEGGIO | 0xf1; //external arp notes for arp effects

		//mused.song.instrument[i].program_unite_bits[0][2 / 8] |= 1 << (2 & 7);

		//mused.song.instrument[i].program[0][3] = MUS_FX_JUMP;

		fread(&mused.song.instrument[i].volume, 1, 1, f);
		mused.song.instrument[i].volume *= 2;
		
		fread(&loop_begin[i], 2, 1, f);
		fread(&loop_len[i], 2, 1, f);
	}
	
	Uint8 temp;
	
	fread(&temp, 1, sizeof(temp), f);
	mused.song.song_length = (Uint16)temp * 64;
	fread(&temp, 1, sizeof(temp), f);
	mused.song.loop_point = 0; //(Uint16)temp * 64;
	
	Uint8 sequence[128];
	fread(sequence, 1, 128, f);
	
	if (instruments > 15) fseek(f, 4, SEEK_CUR); // skip id sig if not a soundtracker module
	
	int pat = 0;
	int patterns = 0;
	
	for (int i = 0; i * 64 < mused.song.song_length; ++i)
	{
		patterns = my_max(patterns, sequence[i]);
		for (int c = 0; c < channels; ++c)
			add_sequence(c, i * 64, sequence[i] * channels + c, 0);
	}
	
	int sl[32] = { 0 }, lp[32][16];
	
	memset(lp, 0, sizeof(lp));
	
	assert(32 >= channels);
	
	for (Uint8 i = 0; i <= patterns; ++i)
	{
		for (int c = 0; c < channels; ++c)
		{
			pat = i * channels + c;
			resize_pattern(&mused.song.pattern[pat], 64);
			memset(mused.song.pattern[pat].step, 0, sizeof(mused.song.pattern[pat].step[0]) * 64);
		}
		
		for (int s = 0; s < 64; ++s)
		{
			pat = i * channels;
		
			for (int c = 0; c < channels; ++c)
			{
				Uint16 period;
				fread(&period, 1, sizeof(period), f);
				Uint8 inst, param;
				fread(&inst, 1, sizeof(inst), f);
				fread(&param, 1, sizeof(param), f);
				
				mused.song.pattern[pat].step[s].note = find_note(SDL_SwapBE16(period) & 0xfff);
				mused.song.pattern[pat].step[s].instrument = ((inst >> 4) | ((SDL_SwapBE16(period) & 0xf000) >> 8)) - 1;
				mused.song.pattern[pat].step[s].volume = MUS_NOTE_NO_VOLUME;
				
				if (mused.song.pattern[pat].step[s].instrument != MUS_NOTE_NO_INSTRUMENT && mused.song.pattern[pat].step[s].instrument != 0)
					sl[c] = sample_length[mused.song.pattern[pat].step[s].instrument];
					
				Uint8 command = inst & 0xf;
				
				if (command == 5)
				{
					command = 0xa;
					mused.song.pattern[pat].step[s].ctrl |= MUS_CTRL_SLIDE | MUS_CTRL_LEGATO;
				}
					
				if (command == 6)
				{
					command = 0xa;
					mused.song.pattern[pat].step[s].ctrl |= MUS_CTRL_VIB;
				}
					
				if (param != 0) //(command != 0 && param != 0)
					lp[c][command] = param;
				
				mused.song.pattern[pat].step[s].command[0] = find_command_pt(param | ((Uint16)command << 8), sl[c]);
				++pat;
			}
		}
	}
	
	Sint8 *sample_data = calloc(1, 65536 * sizeof(sample_data[0]));
	
	wt_e = 0;
	
	for (int i = 0; i < instruments; ++i)
	{
		if (sample_length[i] > 1)
		{
			debug("Reading sample %d (%d bytes)", i, sample_length[i]);
			fread(sample_data, sample_length[i], 1, f);
			
			sample_data[0] = sample_data[1] = 0;
			
			cyd_wave_entry_init(&mused.mus.cyd->wavetable_entries[wt_e], sample_data, sample_length[i], CYD_WAVE_TYPE_SINT8, 1, 1, 1);
			
			mused.mus.cyd->wavetable_entries[wt_e].loop_begin = SDL_SwapBE16(loop_begin[i]) * 2;
			mused.mus.cyd->wavetable_entries[wt_e].loop_end = (SDL_SwapBE16(loop_begin[i]) + SDL_SwapBE16(loop_len[i])) * 2;
			
			mused.mus.cyd->wavetable_entries[wt_e].loop_begin = my_min(mused.mus.cyd->wavetable_entries[wt_e].loop_begin, mused.mus.cyd->wavetable_entries[wt_e].samples - 1);
			mused.mus.cyd->wavetable_entries[wt_e].loop_end = my_min(mused.mus.cyd->wavetable_entries[wt_e].loop_end, mused.mus.cyd->wavetable_entries[wt_e].samples);
			
			if (SDL_SwapBE16(loop_len[i]) > 1)
			{
				mused.mus.cyd->wavetable_entries[wt_e].flags |= CYD_WAVE_LOOP;
			}
			
			mused.mus.cyd->wavetable_entries[wt_e].flags |= CYD_WAVE_NO_INTERPOLATION; //crispy!
			
			/* assuming PAL timing i.e. C-2 = 8287 Hz */
			mused.mus.cyd->wavetable_entries[wt_e].base_note = (MIDDLE_C << 8) - ((Sint16)fine[i] << 1);
			mused.mus.cyd->wavetable_entries[wt_e].sample_rate = (Uint32)(7093789.2 / 856.0);
			
			++wt_e;
		}
	}
	
	free(sample_data);
	
	mused.sequenceview_steps = 64;
	mused.song.num_patterns = patterns * channels;
	
	mused.song.flags |= MUS_USE_OLD_SAMPLE_LOOP_BEHAVIOUR | MUS_USE_OLD_EFFECTS_BEHAVIOUR;
	
	if (channels == 4)
	{
		// Amiga panning
		// not completely panned to left and right
		mused.song.default_panning[0] = -96; //was -48 or 48
		mused.song.default_panning[1] = 96;
		mused.song.default_panning[2] = 96;
		mused.song.default_panning[3] = -96;
	}
	
	return 1;
}
