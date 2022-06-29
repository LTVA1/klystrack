/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)

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


#include "optimize.h"
#include "edit.h"
#include "macros.h"
#include "mused.h"
#include <string.h>

#include "event.h" //wasn't there

bool is_pattern_used(const MusSong *song, int p)
{
	for (int c = 0; c < song->num_channels; ++c)
	{
		for (int s = 0; s < song->num_sequences[c]; ++s)
		{
			if (song->sequence[c][s].pattern == p)
			{
				return true;
			}
		}
	}

	return false;
}


static void replace_pattern(MusSong *song, int from, int to)
{
	for (int c = 0; c < song->num_channels; ++c)
	{
		for (int s = 0; s < song->num_sequences[c]; ++s)
		{
			if (song->sequence[c][s].pattern == from)
			{
				song->sequence[c][s].pattern = to;
			}
		}
	}
}


bool is_pattern_equal(const MusPattern *a, const MusPattern *b)
{
	if (b->num_steps != a->num_steps)
		return false;
		
	for (int i = 0; i < a->num_steps; ++i)
		if (a->step[i].note != b->step[i].note 
			|| a->step[i].instrument != b->step[i].instrument
			|| a->step[i].volume != b->step[i].volume
			|| a->step[i].ctrl != b->step[i].ctrl)
			return false;
		
		else
		{
			for(int i1 = 0; i1 < MUS_MAX_COMMANDS; ++i1)
			{
				if(a->step[i].command[i1] != b->step[i].command[i1])
				{
					return false;
				}
			}
		}

	return true;
}


bool is_pattern_empty(const MusPattern *a)
{
	for (int i = 0; i < a->num_steps; ++i)
		if (a->step[i].note != MUS_NOTE_NONE
			|| a->step[i].instrument != MUS_NOTE_NO_INSTRUMENT
			|| a->step[i].volume != MUS_NOTE_NO_VOLUME
			|| a->step[i].ctrl != 0)
			return false;
		
		else
		{
			for(int i1 = 0; i1 < MUS_MAX_COMMANDS; ++i1)
			{
				if(a->step[i].command[i1] != 0)
				{
					return false;
				}
			}
		}

	return true;
}


bool is_instrument_used(const MusSong *song, int instrument)
{
	for (int p = 0; p < song->num_patterns; ++p)
	{
		for (int i = 0; i < song->pattern[p].num_steps; ++i)
		{
			if (song->pattern[p].step[i].instrument == instrument)
				return true;
		}
	}
	
	return false;
}


static void remove_instrument(MusSong *song, int instrument)
{
	for (int p = 0; p < song->num_patterns; ++p)
	{
		for (int i = 0; i < song->pattern[p].num_steps; ++i)
		{
			if (song->pattern[p].step[i].instrument != MUS_NOTE_NO_INSTRUMENT && song->pattern[p].step[i].instrument > instrument)
				song->pattern[p].step[i].instrument--;
		}
	}
	
	for (int i = instrument; i < song->num_instruments - 1; ++i)
		memcpy(&song->instrument[i], &song->instrument[i + 1], sizeof(song->instrument[i]));
	
	kt_default_instrument(&song->instrument[song->num_instruments - 1]);
}


bool is_wavetable_used(const MusSong *song, int wavetable)
{
	for (int i = 0; i < song->num_instruments; ++i)
	{
		if (song->instrument[i].wavetable_entry == wavetable)
		{
			debug("Wavetable %x used by instrument %x wave", wavetable, i);
			return true;
		}
		
		if (song->instrument[i].fm_wave == wavetable)
		{
			debug("Wavetable %x used by instrument %x FM", wavetable, i);
			return true;
		}
		
		for (int p = 0; p < MUS_PROG_LEN; ++p)
			if ((song->instrument[i].program[p] & 0xffff) == (MUS_FX_SET_WAVETABLE_ITEM | wavetable))
			{
				debug("Wavetable %x used by instrument %x program (step %d)", wavetable, i, p);
				return true;
			}
	}
	
	for (int p = 0; p < song->num_patterns; ++p)
	{
		for (int i = 0; i < song->pattern[p].num_steps; ++i)
		{
			for(int i1 = 0; i1 < MUS_MAX_COMMANDS; ++i1)
			{
				if ((song->pattern[p].step[i].command[i1]) == (MUS_FX_SET_WAVETABLE_ITEM | wavetable))
				{
					debug("Wavetable %x used by pattern %x command %d (step %d)", wavetable, p, i1, i);
					return true;
				}
			}
		}
	}
	
	return false;
}


static void remove_wavetable(MusSong *song, CydEngine *cyd, int wavetable)
{
	debug("Removing wavetable item %d", wavetable);
	
	for (int i = 0; i < song->num_instruments; ++i)
	{
		if (song->instrument[i].wavetable_entry > wavetable)
			song->instrument[i].wavetable_entry--;
		
		if (song->instrument[i].fm_wave == wavetable)
			song->instrument[i].fm_wave--;
		
		for (int p = 0; p < MUS_PROG_LEN; ++p)
			if ((song->instrument[i].program[p] & 0xff00) == MUS_FX_SET_WAVETABLE_ITEM)
			{
				Uint8 param = song->instrument[i].program[p] & 0xff;
				
				if (param > wavetable)
					song->instrument[i].program[p] = (song->instrument[i].program[p] & 0x8000) | MUS_FX_SET_WAVETABLE_ITEM | (param - 1);
			}
	}
	
	for (int p = 0; p < song->num_patterns; ++p)
	{
		for (int i = 0; i < song->pattern[p].num_steps; ++i)
		{
			for(int i1 = 0; i1 < MUS_MAX_COMMANDS; ++i1)
			{
				if ((song->pattern[p].step[i].command[i1] & 0xff00) == MUS_FX_SET_WAVETABLE_ITEM)
				{
					Uint8 param = song->pattern[p].step[i].command[i1] & 0xff;
					
					if (param > wavetable)
						song->pattern[p].step[i].command[i1] = MUS_FX_SET_WAVETABLE_ITEM | (param - 1);
				}
			}
		}
	}
	
	for (int i = wavetable; i < song->num_wavetables - 1; ++i)
	{
		strcpy(song->wavetable_names[i], song->wavetable_names[i + 1]);
		
		cyd->wavetable_entries[i].flags = cyd->wavetable_entries[i + 1].flags;
		cyd->wavetable_entries[i].sample_rate = cyd->wavetable_entries[i + 1].sample_rate;
		cyd->wavetable_entries[i].samples = cyd->wavetable_entries[i + 1].samples;
		cyd->wavetable_entries[i].loop_begin = cyd->wavetable_entries[i + 1].loop_begin;
		cyd->wavetable_entries[i].loop_end = cyd->wavetable_entries[i + 1].loop_end;
		cyd->wavetable_entries[i].base_note = cyd->wavetable_entries[i + 1].base_note;
		cyd->wavetable_entries[i].data = realloc(cyd->wavetable_entries[i].data, cyd->wavetable_entries[i + 1].samples * sizeof(Sint16));
		memcpy(cyd->wavetable_entries[i].data, cyd->wavetable_entries[i + 1].data, cyd->wavetable_entries[i + 1].samples * sizeof(Sint16));
	}
	
	strcpy(song->wavetable_names[song->num_wavetables - 1], "");
	cyd_wave_entry_init(&cyd->wavetable_entries[song->num_wavetables - 1], NULL, 0, 0, 0, 0, 0);
}


static void remove_pattern(MusSong *song, int p)
{
	void * temp = song->pattern[p].step;
	
	for (int i = 0; i < song->pattern[p].num_steps; ++i)
		zero_step(&song->pattern[p].step[i]);

	for (int a = p; a < song->num_patterns - 1; ++a)
	{
		memcpy(&song->pattern[a], &song->pattern[a + 1], sizeof(song->pattern[a]));
		replace_pattern(song, a + 1, a);
	}
	
	if (song->num_patterns >= 1)
		song->pattern[song->num_patterns - 1].step = temp;
	
	--song->num_patterns;
}


void optimize_duplicate_patterns(MusSong *song)
{
	debug("Kill unused patterns");
	
	int orig_count = song->num_patterns;

	for (int a = 0; a < song->num_patterns; ++a)
	{	
		if (is_pattern_used(song, a))
		{
			for (int b = a + 1; b < song->num_patterns; ++b)
			{	
				if (is_pattern_used(song, b) && is_pattern_equal(&song->pattern[a], &song->pattern[b]))
				{
					replace_pattern(song, b, a);
				}
			}
		}
	}
	
	for (int a = 0; a < song->num_patterns; )
	{	
		if (!is_pattern_used(song, a))
		{
			remove_pattern(song, a);
		}
		else 
			++a;
	}
	
	set_info_message("Reduced number of patterns from %d to %d", orig_count, song->num_patterns);
	
	song->num_patterns = NUM_PATTERNS;
}



void kill_empty_patterns(MusSong *song, void* no_confirm) //wasn't there
{
	if (!CASTPTR(int, no_confirm) && !confirm(domain, mused.slider_bevel, &mused.largefont, "Kill all empty patterns (no undo)?"))
		return;
	
	debug("Kill empty patterns");
	
	int orig_count = song->num_patterns;
	
	for (int a = 0; a < song->num_patterns; a++)
	{	
		if(is_pattern_empty(&(song->pattern[a])))
		{
			for(int track = 0; track < MUS_MAX_CHANNELS; track++)
			{
				if (song->num_sequences[track] != 0)
				{
					for (int i = 0; i < song->num_sequences[track]; ++i)
					{
						if (is_pattern_equal(&song->pattern[song->sequence[track][i].pattern], &song->pattern[a]))
						{
							song->sequence[track][i].position = 0xffff;
						}
					}
					
					qsort(song->sequence[track], song->num_sequences[track], sizeof(song->sequence[track][0]), seqsort);

					while (song->num_sequences[track] > 0 && song->sequence[track][song->num_sequences[track]-1].position == 0xffff) --song->num_sequences[track];	
				}
			}
		}
	}
	
	optimize_duplicate_patterns(song);
	
	/*for (int a = 0; a < song->num_patterns; )
	{	
		if (!is_pattern_used(song, a))
		{
			remove_pattern(song, a);
		}
		else 
			++a;
	}*/
	
	set_info_message("Reduced number of patterns from %d to %d by killing empty patterns", orig_count, song->num_patterns);
	
	//song->num_patterns = NUM_PATTERNS;
}


void optimize_patterns_brute(MusSong *song) //wasn't there
{
	if (!confirm(domain, mused.slider_bevel, &mused.largefont, "Kill all patterns' redundant data (no undo)?"))
		return;
	
	debug("Brute pattern optimizing");
	
	if (confirm(domain, mused.slider_bevel, &mused.largefont, "Kill all instruments on empty rows (no undo)?"))
	{
		for(int i = 0; i < song->num_patterns; i++)
		{
			for(int j = 0; j < song->pattern[i].num_steps; j++)
			{
				if(song->pattern[i].step[j].volume == 0x80)
				{
					song->pattern[i].step[j].volume = MUS_NOTE_NO_VOLUME;
				}
				
				if(song->pattern[i].step[j].note == MUS_NOTE_RELEASE)
				{
					song->pattern[i].step[j].instrument = MUS_NOTE_NO_INSTRUMENT;
				}
				
				if(song->pattern[i].step[j].note == MUS_NOTE_NONE && song->pattern[i].step[j].instrument != MUS_NOTE_NO_INSTRUMENT)
				{
					song->pattern[i].step[j].instrument = MUS_NOTE_NO_INSTRUMENT;
				}
			}
		}
	}
	
	if (confirm(domain, mused.slider_bevel, &mused.largefont, "Kill all instruments on rows with notes (can silent instruments with volfade, no undo)?"))
	{
		for(int i = 0; i < song->num_patterns; i++)
		{
			for(int j = 0; j < song->pattern[i].num_steps; j++)
			{
				Uint8 flag = 0;
				
				for(int k = j + 1; k < song->pattern[i].num_steps; k++)
				{
					if(song->pattern[i].step[j].instrument == song->pattern[i].step[k].instrument && flag == 0 && song->pattern[i].step[j].note != MUS_NOTE_NONE && song->pattern[i].step[k].note != MUS_NOTE_NONE)
					{
						song->pattern[i].step[k].instrument = MUS_NOTE_NO_INSTRUMENT;
					}
						
					if(song->pattern[i].step[j].instrument != song->pattern[i].step[k].instrument && song->pattern[i].step[j].note != MUS_NOTE_NONE && song->pattern[i].step[k].note != MUS_NOTE_NONE && song->pattern[i].step[k].instrument != MUS_NOTE_NO_INSTRUMENT)
					{
						flag = 1;
					}
				}
			}
		}
	}
		
	if (confirm(domain, mused.slider_bevel, &mused.largefont, "Cut patterns without SKIP_PATTERN command to the last non-empty row (no undo)?"))
	{
		for(int i = 0; i < song->num_patterns; i++)
		{
			Uint8 flag = 0;
			
			Uint8 cut_flag = 0;
			
			for(int j = 0; j < song->pattern[i].num_steps; j++)
			{
				for(int i1 = 0; i1 < MUS_MAX_COMMANDS; ++i1)
				{
					if((song->pattern[i].step[j].command[i1] & 0xff00) == MUS_FX_SKIP_PATTERN)
					{
						cut_flag = 1;
					}
				}
			}
			
			int temp = song->pattern[i].num_steps;
			
			if(cut_flag == 0)
			{
				for(int j = temp - 1; j >= 0; j--)
				{
					//for(int i1 = 0; i1 < MUS_MAX_COMMANDS; ++i1)
					//{
						if (song->pattern[i].step[j].note == MUS_NOTE_NONE
						&& song->pattern[i].step[j].instrument == MUS_NOTE_NO_INSTRUMENT
						&& song->pattern[i].step[j].volume == MUS_NOTE_NO_VOLUME
						&& song->pattern[i].step[j].ctrl == 0
						&& song->pattern[i].step[j].command[0] == 0x0000 
						&& song->pattern[i].step[j].command[1] == 0x0000 
						&& song->pattern[i].step[j].command[2] == 0x0000 
						&& song->pattern[i].step[j].command[3] == 0x0000 
						&& song->pattern[i].step[j].command[4] == 0x0000 
						&& song->pattern[i].step[j].command[5] == 0x0000 
						&& song->pattern[i].step[j].command[6] == 0x0000 
						&& song->pattern[i].step[j].command[7] == 0x0000 && flag == 0)
						{
							song->pattern[i].num_steps--;
						}
						
						if(song->pattern[i].step[j].note != MUS_NOTE_NONE
						|| song->pattern[i].step[j].instrument != MUS_NOTE_NO_INSTRUMENT
						|| song->pattern[i].step[j].volume != MUS_NOTE_NO_VOLUME
						|| song->pattern[i].step[j].ctrl != 0
						|| song->pattern[i].step[j].command[0] != 0x0000
						|| song->pattern[i].step[j].command[1] != 0x0000
						|| song->pattern[i].step[j].command[2] != 0x0000
						|| song->pattern[i].step[j].command[3] != 0x0000
						|| song->pattern[i].step[j].command[4] != 0x0000
						|| song->pattern[i].step[j].command[5] != 0x0000
						|| song->pattern[i].step[j].command[6] != 0x0000
						|| song->pattern[i].step[j].command[7] != 0x0000)
						{
							flag = 1;
						}
					//}
				}
			}
		}
	}
	
	set_info_message("Removed unused pattern data with brute force");
}



void optimize_unused_instruments(MusSong *song)
{
	int removed = 0;
	
	debug("Kill unused instruments");
	
	for (int i = 0; i < song->num_instruments; ++i)
		if (!is_instrument_used(song, i))
		{
			remove_instrument(song, i);
			++removed;
		}
		
	set_info_message("Removed %d unused instruments", removed);
}


void optimize_unused_wavetables(MusSong *song, CydEngine *cyd)
{
	int removed = 0;
	
	debug("Kill unused wavetables");
	
	for (int i = 0; i < song->num_wavetables; ++i)
		if (!is_wavetable_used(song, i))
		{
			remove_wavetable(song, cyd, i);
			++removed;
		}
		
	set_info_message("Removed %d unused wavetables", removed);
}

void kill_duplicate_wavetables(MusSong *song, CydEngine *cyd) //wasn't there
{
	int removed = 0;
	
	debug("Kill duplicate wavetables");
	debug("Wavetables: %d", song->num_wavetables);
	
	for (int i = 0; i <= song->num_wavetables; ++i)
	{
		for(int j = 0; j <= song->num_wavetables; ++j)
		{
			if(i != j)
			{
				if(cyd->wavetable_entries[i].flags == cyd->wavetable_entries[j].flags &&
					cyd->wavetable_entries[i].sample_rate == cyd->wavetable_entries[j].sample_rate &&
					cyd->wavetable_entries[i].samples == cyd->wavetable_entries[j].samples &&
					cyd->wavetable_entries[i].loop_begin == cyd->wavetable_entries[j].loop_begin &&
					cyd->wavetable_entries[i].loop_end == cyd->wavetable_entries[j].loop_end &&
					cyd->wavetable_entries[i].base_note == cyd->wavetable_entries[j].base_note && 
					cyd->wavetable_entries[i].sample_rate != 0 && 
					cyd->wavetable_entries[i].samples != 0 &&
					(strcmp(song->wavetable_names[j], song->wavetable_names[i]) == 0 || (song->wavetable_names[j] == NULL && song->wavetable_names[i] == 0)))
				{
					Uint8 flag = 0;
					
					for(int k = 0; k < cyd->wavetable_entries[j].samples; k++)
					{
						if(*(cyd->wavetable_entries[i].data + k) != *(cyd->wavetable_entries[j].data + k))
						{
							flag++;
						}
					}
					
					if(flag == 0)
					{
						debug("Killing wavetable number %d", j);
						
						for (int h = 0; h < song->num_instruments; ++h)
						{
							if (song->instrument[h].wavetable_entry == j)
								song->instrument[h].wavetable_entry = i;
							
							if (song->instrument[h].fm_wave == j)
								song->instrument[h].fm_wave = i;
							
							for(int q = 0; q < CYD_FM_NUM_OPS; ++q)
							{
								if (song->instrument[h].ops[q].wavetable_entry == j)
								song->instrument[h].ops[q].wavetable_entry = i;
							}
							
							for (int p = 0; p < MUS_PROG_LEN; ++p)
							{
								if ((song->instrument[h].program[p] & 0x3B00) == 0x3B00 && (song->instrument[h].program[p] & 0x00FF) == j && (song->instrument[h].program[p] & 0xFF00) != 0xFF00)
								{
									song->instrument[h].program[p] = 0x3B00 + i;
								}
							}
							
							for(int q = 0; q < CYD_FM_NUM_OPS; ++q)
							{
								for (int p = 0; p < MUS_PROG_LEN; ++p)
								{
									if ((song->instrument[h].ops[q].program[p] & 0x3B00) == 0x3B00 && (song->instrument[h].ops[q].program[p] & 0x00FF) == j && (song->instrument[h].ops[q].program[p] & 0xFF00) != 0xFF00)
									{
										song->instrument[h].ops[q].program[p] = 0x3B00 + i;
									}
								}
							}
						}
						
						for (int p = 0; p < song->num_patterns; ++p)
						{
							for (int w = 0; w < song->pattern[p].num_steps; ++w)
							{
								for(int i1 = 0; i1 < MUS_MAX_COMMANDS; ++i1)
								{
									if ((song->pattern[p].step[w].command[i1] & 0x3B00) == 0x3B00 && (song->pattern[p].step[w].command[i1] & 0x00FF) == j && (song->pattern[p].step[w].command[i1] & 0xFF00) != 0xFF00)
									{
										song->pattern[p].step[w].command[i1] = 0x3B00 + i;
									}
								}
							}
						}
						
						strcpy(song->wavetable_names[i], song->wavetable_names[j]);
						
						cyd_wave_entry_init(&cyd->wavetable_entries[j], NULL, 0, 0, 0, 0, 0);

						strcpy(mused.song.wavetable_names[j], "");
			
						removed++;
					}
				}
			}
		}
	}
	
	debug("Removed %d duplicate wavetables", removed);
	set_info_message("Removed %d duplicate wavetables", removed);
}

void optimize_song(MusSong *song)
{
	debug("Optimizing song");
	
	kill_empty_patterns(&mused.song, NULL); //wasn't there
	
	optimize_duplicate_patterns(song);	
	optimize_unused_instruments(&mused.song);
	optimize_unused_wavetables(&mused.song, &mused.cyd);
	
	kill_duplicate_wavetables(&mused.song, &mused.cyd); //wasn't there
	
	set_info_message("Removed unused song data");
}

void optimize_patterns_action(void *unused1, void *unused2, void *unused3)
{
	optimize_duplicate_patterns(&mused.song);
}

void optimize_empty_patterns_action(void *no_confirm, void *unused2, void *unused3) //wasn't there
{
	kill_empty_patterns(&mused.song, no_confirm);
}

void optimize_instruments_action(void *unused1, void *unused2, void *unused3)
{
	optimize_unused_instruments(&mused.song);
}

void optimize_wavetables_action(void *unused1, void *unused2, void *unused3)
{
	optimize_unused_wavetables(&mused.song, &mused.cyd);
}

void duplicate_wavetables_action(void *unused1, void *unused2, void *unused3) //wasn't there
{
	kill_duplicate_wavetables(&mused.song, &mused.cyd);
}

void optimize_patterns_brute_action(void *unused1, void *unused2, void *unused3)  //wasn't there
{
	optimize_patterns_brute(&mused.song);
}