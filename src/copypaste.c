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

#include "copypaste.h"
#include "clipboard.h"
#include "mused.h"
#include "event.h"

#include "theme.h"

#define swap(a,b) { a ^= b; b ^= a; a ^= b; }

extern Mused mused;
extern Uint32 colors[];

void copy()
{
	cp_clear(&mused.cp);

	switch (mused.focus)
	{
		case EDITPATTERN:
		{
			if (mused.selection.start == mused.selection.end && get_current_pattern())
			{
				cp_copy_items(&mused.cp, CP_PATTERN, get_current_pattern()->step, sizeof(MusStep), get_current_pattern()->num_steps, mused.selection.start);
			}
			
			else if (get_pattern(mused.selection.start, mused.current_sequencetrack) != -1)
			{
				cp_copy_items(&mused.cp, CP_PATTERNSEGMENT, &mused.song.pattern[get_pattern(mused.selection.start, mused.current_sequencetrack)].step[get_patternstep(mused.selection.start, mused.current_sequencetrack)], sizeof(MusStep), 
					mused.selection.end - mused.selection.start, mused.selection.start);
				
				//a hack -- we copy all columns no matter which are selected, but we keep the information of what columns were selected
				//in paste() and join_paste() we account for what columns were selected
				
				mused.cp.patternx_start = mused.selection.patternx_start;
				mused.cp.patternx_end = mused.selection.patternx_end;
				
				mused.cp.pattern_length = mused.selection.end - mused.selection.start;
			}
		}
		break;
		
		case EDITINSTRUMENT:
		{
			if(mused.cp.inst)
			{
				MusInstrument* inst = mused.cp.inst;
	
				mus_free_inst_programs(inst);
				
				free(inst);
			}
			
			mused.cp.inst = (MusInstrument*)calloc(1, sizeof(MusInstrument));
			
			memcpy(mused.cp.inst, &mused.song.instrument[mused.current_instrument], sizeof(mused.song.instrument[mused.current_instrument]));
			
			for(int i = 0; i < mused.cp.inst->num_macros; ++i)
			{
				mused.cp.inst->program[i] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
				mused.cp.inst->program_unite_bits[i] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
				
				memcpy(mused.cp.inst->program[i], mused.song.instrument[mused.current_instrument].program[i], MUS_PROG_LEN * sizeof(Uint16));
				memcpy(mused.cp.inst->program_unite_bits[i], mused.song.instrument[mused.current_instrument].program_unite_bits[i], (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
			}
			
			for(int op = 0; op < CYD_FM_NUM_OPS; ++op)
			{
				for(int i = 0; i < mused.cp.inst->ops[op].num_macros; ++i)
				{
					mused.cp.inst->ops[op].program[i] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
					mused.cp.inst->ops[op].program_unite_bits[i] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
					
					memcpy(mused.cp.inst->ops[op].program[i], mused.song.instrument[mused.current_instrument].ops[op].program[i], MUS_PROG_LEN * sizeof(Uint16));
					memcpy(mused.cp.inst->ops[op].program_unite_bits[i], mused.song.instrument[mused.current_instrument].ops[op].program_unite_bits[i], (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
				}
			}
			
			cp_copy(&mused.cp, CP_INSTRUMENT, &mused.song.instrument[mused.current_instrument], sizeof(mused.song.instrument[mused.current_instrument]), 0);
		}
		break;
		
		case EDIT4OP:
		case EDITENVELOPE4OP:
		{
			if(mused.cp.op)
			{
				MusFmOp* op = mused.cp.op;
	
				for(int i = 0; i < MUS_MAX_MACROS_OP; ++i)
				{
					if(op->program[i])
					{
						free(op->program[i]);
						op->program[i] = NULL;
					}
					
					if(op->program_unite_bits[i])
					{
						free(op->program_unite_bits[i]);
						op->program_unite_bits[i] = NULL;
					}
				}
				
				free(op);
			}
			
			mused.cp.op = (MusFmOp*)calloc(1, sizeof(MusFmOp));
			
			memcpy(mused.cp.op, &mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1], sizeof(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1]));
			
			for(int i = 0; i < mused.cp.op->num_macros; ++i)
			{
				mused.cp.op->program[i] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
				mused.cp.op->program_unite_bits[i] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
				
				memcpy(mused.cp.op->program[i], mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[i], MUS_PROG_LEN * sizeof(Uint16));
				memcpy(mused.cp.op->program_unite_bits[i], mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[i], (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
			}
			
			cp_copy(&mused.cp, CP_INSTRUMENT, &mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1], sizeof(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1]), 0);
		}
		break;
		
		case EDITPROG:
		case EDITPROG4OP:
		{
			//cp_copy_items(&mused.cp, CP_PROGRAM, &mused.song.instrument[mused.current_instrument].program[mused.selection.start], mused.selection.end - mused.selection.start, 
				//sizeof(mused.song.instrument[mused.current_instrument].program[0]), mused.selection.start);
			
			if(mused.show_four_op_menu)
			{
				cp_copy_items(&mused.cp, CP_PROGRAM, &mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.selection.start], mused.selection.end - mused.selection.start, 
				sizeof(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][0]), mused.selection.start);
			}
				
			else
			{
				cp_copy_items(&mused.cp, CP_PROGRAM, &mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][mused.selection.start], mused.selection.end - mused.selection.start, 
				sizeof(mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][0]), mused.selection.start);
			}
			
			for(int i = mused.selection.start; i <= mused.selection.end; ++i)
			{
				if(mused.show_four_op_menu)
				{
					mused.unite_bits_buffer[i] = (mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8]) & (1 << (i & 7));
				}
				
				else
				{
					mused.unite_bits_buffer[i] = (mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][i / 8]) & (1 << (i & 7));
				}
				
				if(mused.unite_bits_buffer[i] > 0)
				{
					mused.unite_bits_buffer[i] = 1;
				}
			}
			
			mused.unite_bits_to_paste = mused.selection.end - mused.selection.start;
			mused.paste_pointer = mused.selection.start;
			//unite_bits_buffer
			
			//cp_copy_items(&mused.cp, CP_PROGRAM, &mused.song.instrument[mused.current_instrument].program_unite_bits[mused.selection.start / 8], (mused.selection.end - mused.selection.start) / 8 + 1, sizeof(mused.song.instrument[mused.current_instrument].program_unite_bits[0]), mused.selection.start);
		}
		break;
		
		case EDITSEQUENCE:
		{
			int first = -1, last = -1;
			
			for (int i = 0; i < mused.song.num_sequences[mused.current_sequencetrack]; ++i)
			{
				if (first == -1 && mused.song.sequence[mused.current_sequencetrack][i].position >= mused.selection.start && mused.song.sequence[mused.current_sequencetrack][i].position < mused.selection.end)
					first = i;
					
				if (mused.song.sequence[mused.current_sequencetrack][i].position < mused.selection.end)
					last = i;
			}
			
			// Check if no items inside the selection
			
			if (first == -1 || first >= mused.song.num_sequences[mused.current_sequencetrack]) 
				break;
			
			cp_copy_items(&mused.cp, CP_SEQUENCE, &mused.song.sequence[mused.current_sequencetrack][first], last-first+1, sizeof(mused.song.sequence[mused.current_sequencetrack][0]), mused.selection.start);
		}
		break;
		
		case EDITWAVETABLE:
		{
			if(mused.flags & SHOW_WAVEGEN)
			{
				cp_copy(&mused.cp, CP_WAVEGEN, &mused.wgset.chain[mused.selected_wg_osc], sizeof(mused.wgset.chain[mused.selected_wg_osc]), 0);
			}
			
			else
			{
				cp_copy(&mused.cp, CP_WAVETABLE, &mused.mus.cyd->wavetable_entries[mused.selected_wavetable], sizeof(mused.mus.cyd->wavetable_entries[mused.selected_wavetable]), 0);
				
				mused.selection.prev_name_index = mused.selected_wavetable;
				
				mused.selection.is_local_sample = false;
			}
		}
		break;
		
		case EDITLOCALSAMPLE:
		{
			if(mused.show_local_samples_list)
			{
				mused.selection.is_local_sample = true;
				mused.selection.local_sample_instrument = mused.current_instrument;
				
				mused.selection.prev_name_index = mused.selected_local_sample;
				
				cp_copy(&mused.cp, CP_LOCALSAMPLE, mused.song.instrument[mused.current_instrument].local_samples[mused.selected_local_sample], sizeof(mused.song.instrument[mused.current_instrument].local_samples[mused.selected_local_sample][0]), 0);
			}
			
			else
			{
				mused.selection.is_local_sample = false;
				mused.selection.local_sample_instrument = 0;
				
				mused.selection.prev_name_index = mused.selected_local_sample;
				
				cp_copy(&mused.cp, CP_LOCALSAMPLE, &mused.mus.cyd->wavetable_entries[mused.selected_local_sample], sizeof(mused.mus.cyd->wavetable_entries[mused.selected_local_sample]), 0);
			}
		}
		break;
	}
}

void cut()
{
	copy();
	delete();
}


void delete()
{
	switch (mused.focus)
	{
		case EDITPATTERN:
		snapshot(S_T_PATTERN);
		
		if (mused.selection.start == mused.selection.end)
		{
			//debug("if (mused.selection.start == mused.selection.end)");
			clear_pattern(&mused.song.pattern[current_pattern()]);
		}
		
		else
		{
			//debug("else, pattern %d, start step %d, end step %d, selection start %d, selection end %d", get_pattern(mused.selection.start, mused.current_sequencetrack), get_patternstep(mused.selection.start, mused.current_sequencetrack), get_patternstep(mused.selection.end, mused.current_sequencetrack), mused.selection.start, mused.selection.end);
			clear_pattern_range(&mused.song.pattern[get_pattern(mused.selection.start, mused.current_sequencetrack)], get_patternstep(mused.selection.start, mused.current_sequencetrack), get_patternstep(mused.selection.end, mused.current_sequencetrack));
		}
		
		break;
		
		case EDITPROG:
		{
			for(int i = mused.selection.start; i < mused.selection.end; ++i)
			{
				mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][i / 8] &= ~(1 << (i & 7));
				mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][i] = MUS_FX_NOP;
			}
		}
		break;
		
		case EDITPROG4OP:
		{
			for(int i = mused.selection.start; i < mused.selection.end; ++i)
			{
				mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] &= ~(1 << (i & 7));
				mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][i] = MUS_FX_NOP;
			}
		}
		break;
		
		case EDITSEQUENCE:
		snapshot(S_T_SEQUENCE);
		del_sequence(mused.selection.start, mused.selection.end, mused.current_sequencetrack);
		
		break;
	}
	
	mused.selection.start = mused.selection.end = 0;
}


void paste()
{
	switch (mused.focus)
	{
		case EDITSEQUENCE:
		{
			if (mused.cp.type != CP_SEQUENCE) break;
			
			size_t items = cp_get_item_count(&mused.cp, sizeof(mused.song.sequence[0][0]));
			
			if (items < 1) break;
			
			snapshot(S_T_SEQUENCE);
			
			int first = ((MusSeqPattern*)mused.cp.data)[0].position;
			int last = ((MusSeqPattern*)mused.cp.data)[items-1].position;
			
			del_sequence(mused.current_sequencepos, last-first+mused.current_sequencepos, mused.current_sequencetrack);
			
			for (int i = 0; i < items; ++i)
			{
				add_sequence(mused.current_sequencetrack, ((MusSeqPattern*)mused.cp.data)[i].position - mused.cp.position + mused.current_sequencepos, ((MusSeqPattern*)mused.cp.data)[i].pattern, ((MusSeqPattern*)mused.cp.data)[i].note_offset);
			}
		}
		break;
		
		case EDITPATTERN:
		{
			size_t items = cp_get_item_count(&mused.cp, sizeof(mused.song.pattern[current_pattern()].step[0]));
			
			if (items < 1) 
				break;
			
			if (mused.cp.type == CP_PATTERN)
			{
				snapshot(S_T_PATTERN);
				resize_pattern(&mused.song.pattern[current_pattern()], items);
				cp_paste_items(&mused.cp, CP_PATTERN, mused.song.pattern[current_pattern()].step, items, sizeof(mused.song.pattern[current_pattern()].step[0]));
			}
			
			else if (mused.cp.type == CP_PATTERNSEGMENT && (current_pattern() != -1))
			{
				debug("paste to pattern %d", current_pattern());
				snapshot(S_T_PATTERN);
				
				//cp_paste_items(&mused.cp, CP_PATTERNSEGMENT, &mused.song.pattern[current_pattern()].step[current_patternstep()], mused.song.pattern[current_pattern()].num_steps - current_patternstep(), sizeof(mused.song.pattern[current_pattern()].step[0]));
				
				if(mused.cp.data != NULL)
				{
					MusStep* s = &mused.song.pattern[current_pattern()].step[current_patternstep()];
					
					MusStep* cp_step = (MusStep*)mused.cp.data;
					
					for(int i = 0; i < mused.cp.pattern_length && i + current_patternstep() < mused.song.pattern[current_pattern()].num_steps; ++i) //erase
					{
						s[i].note = MUS_NOTE_NONE;
						s[i].instrument = MUS_NOTE_NO_INSTRUMENT;
						s[i].volume = MUS_NOTE_NO_VOLUME;
						s[i].ctrl = 0;
						
						for(int j = 0; j < MUS_MAX_COMMANDS; ++j)
						{
							s[i].command[j] = 0;
						}
					}
					
					for(int i = 0; i < mused.cp.pattern_length && i + current_patternstep() < mused.song.pattern[current_pattern()].num_steps; ++i) //paste
					{
						if(mused.cp.patternx_start == 0 && mused.cp.patternx_end >= 0)
						{
							s[i].note = cp_step[i].note;
						}
						
						if(mused.cp.patternx_start <= 1 && mused.cp.patternx_end >= 1)
						{
							if(s[i].instrument == MUS_NOTE_NO_INSTRUMENT)
							{
								s[i].instrument = 0;
							}
							
							else
							{
								s[i].instrument &= 0x0f;
							}
							
							s[i].instrument |= (cp_step[i].instrument & 0xf0);
						}
						
						if(mused.cp.patternx_start <= 2 && mused.cp.patternx_end >= 2)
						{
							if(s[i].instrument == MUS_NOTE_NO_INSTRUMENT)
							{
								s[i].instrument = 0;
							}
							
							else
							{
								s[i].instrument &= 0xf0;
							}
							
							s[i].instrument |= (cp_step[i].instrument & 0x0f);
						}
						
						if(mused.cp.patternx_start <= 3 && mused.cp.patternx_end >= 3)
						{
							if(s[i].volume == MUS_NOTE_NO_VOLUME)
							{
								s[i].volume = 0;
							}
							
							else
							{
								s[i].volume &= 0x0f;
							}
							
							s[i].volume |= (cp_step[i].volume & 0xf0);
						}
						
						if(mused.cp.patternx_start <= 4 && mused.cp.patternx_end >= 4)
						{
							if(s[i].volume == MUS_NOTE_NO_VOLUME)
							{
								s[i].volume = 0;
							}
							
							else
							{
								s[i].volume &= 0xf0;
							}
							
							s[i].volume |= (cp_step[i].volume & 0x0f);
						}
						
						
						
						if(mused.cp.patternx_start <= 5 && mused.cp.patternx_end >= 5)
						{
							s[i].ctrl |= (cp_step[i].ctrl & 0b0001);
						}
						
						if(mused.cp.patternx_start <= 6 && mused.cp.patternx_end >= 6)
						{
							s[i].ctrl |= (cp_step[i].ctrl & 0b0010);
						}
						
						if(mused.cp.patternx_start <= 7 && mused.cp.patternx_end >= 7)
						{
							s[i].ctrl |= (cp_step[i].ctrl & 0b0100);
						}
						
						if(mused.cp.patternx_start <= 8 && mused.cp.patternx_end >= 8)
						{
							s[i].ctrl |= (cp_step[i].ctrl & 0b1000);
						}
						
						for(int j = 0; j < MUS_MAX_COMMANDS; ++j)
						{
							if(mused.cp.patternx_start <= 9 + j * 4 && mused.cp.patternx_end >= 9 + j * 4)
							{
								s[i].command[j] |= (cp_step[i].command[j] & (Uint16)0xf000);
							}
							
							if(mused.cp.patternx_start <= 10 + j * 4 && mused.cp.patternx_end >= 10 + j * 4)
							{
								s[i].command[j] |= (cp_step[i].command[j] & 0x0f00);
							}
							
							if(mused.cp.patternx_start <= 11 + j * 4 && mused.cp.patternx_end >= 11 + j * 4)
							{
								s[i].command[j] |= (cp_step[i].command[j] & 0x00f0);
							}
							
							if(mused.cp.patternx_start <= 12 + j * 4 && mused.cp.patternx_end >= 12 + j * 4)
							{
								s[i].command[j] |= (cp_step[i].command[j] & 0x000f);
							}
						}
					}
				}
			}
		}
		break;
	
		case EDITINSTRUMENT:
		{
			if (mused.cp.type == CP_INSTRUMENT)
			{
				snapshot(S_T_INSTRUMENT);
				
				MusInstrument* inst = &mused.song.instrument[mused.current_instrument];
				
				mus_free_inst_programs(inst);
			
				cp_paste_items(&mused.cp, CP_INSTRUMENT, &mused.song.instrument[mused.current_instrument], 1, sizeof(mused.song.instrument[mused.current_instrument]));
				
				for(int i = 0; i < mused.cp.inst->num_macros; ++i)
				{
					mused.song.instrument[mused.current_instrument].program[i] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
					mused.song.instrument[mused.current_instrument].program_unite_bits[i] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
					
					memcpy(mused.song.instrument[mused.current_instrument].program[i], mused.cp.inst->program[i], MUS_PROG_LEN * sizeof(Uint16));
					memcpy(mused.song.instrument[mused.current_instrument].program_unite_bits[i], mused.cp.inst->program_unite_bits[i], (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
				}
				
				for(int op = 0; op < CYD_FM_NUM_OPS; ++op)
				{
					for(int i = 0; i < mused.cp.inst->ops[op].num_macros; ++i)
					{
						mused.song.instrument[mused.current_instrument].ops[op].program[i] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
						mused.song.instrument[mused.current_instrument].ops[op].program_unite_bits[i] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
						
						memcpy(mused.song.instrument[mused.current_instrument].ops[op].program[i], mused.cp.inst->ops[op].program[i], MUS_PROG_LEN * sizeof(Uint16));
						memcpy(mused.song.instrument[mused.current_instrument].ops[op].program_unite_bits[i], mused.cp.inst->ops[op].program_unite_bits[i], (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
					}
				}
			}
		}
		break;
		
		case EDIT4OP:
		case EDITENVELOPE4OP:
		{
			if (mused.cp.type == CP_INSTRUMENT)
			{
				snapshot(S_T_INSTRUMENT);
				
				MusFmOp* op = &mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1];
				
				for(int i = 0; i < MUS_MAX_MACROS_OP; ++i)
				{
					if(op->program[i])
					{
						free(op->program[i]);
					}
					
					if(op->program_unite_bits[i])
					{
						free(op->program_unite_bits[i]);
					}
				}
			
				cp_paste_items(&mused.cp, CP_INSTRUMENT, &mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1], 1, sizeof(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1]));
				
				for(int i = 0; i < mused.cp.op->num_macros; ++i)
				{
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[i] = (Uint16*)calloc(1, MUS_PROG_LEN * sizeof(Uint16));
					mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[i] = (Uint8*)calloc(1, (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
					
					memcpy(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[i], mused.cp.op->program[i], MUS_PROG_LEN * sizeof(Uint16));
					memcpy(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[i], mused.cp.op->program_unite_bits[i], (MUS_PROG_LEN / 8 + 1) * sizeof(Uint8));
				}
			}
		}
		break;
		
		case EDITPROG:
		case EDITPROG4OP:
		{
			size_t items = cp_get_item_count(&mused.cp, sizeof(mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][0]));
			
			if (items < 1) 
				break;
		
			if (mused.cp.type == CP_PROGRAM)
			{
				snapshot(S_T_INSTRUMENT);
				
				if(mused.show_four_op_menu)
				{
					cp_paste_items(&mused.cp, CP_PROGRAM, &mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][mused.current_program_step], MUS_PROG_LEN - mused.current_program_step, 
					sizeof(mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program[mused.current_fourop_program[mused.selected_operator - 1]][0]));
				}
				
				else
				{
					cp_paste_items(&mused.cp, CP_PROGRAM, &mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][mused.current_program_step], MUS_PROG_LEN - mused.current_program_step, 
					sizeof(mused.song.instrument[mused.current_instrument].program[mused.current_instrument_program][0]));
				}
				
				int y = 0;
				
				for(int i = mused.current_program_step; i <= my_min(MUS_PROG_LEN - 1, mused.current_program_step + mused.unite_bits_to_paste); ++i, ++y)
				{
					if(mused.unite_bits_buffer[mused.paste_pointer + y] == 0)
					{
						if(mused.show_four_op_menu)
						{
							mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] &= ~(1 << (i & 7));
						}
						
						else
						{
							mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][i / 8] &= ~(1 << (i & 7));
						}
					}
					
					else
					{
						if(mused.show_four_op_menu)
						{
							mused.song.instrument[mused.current_instrument].ops[mused.selected_operator - 1].program_unite_bits[mused.current_fourop_program[mused.selected_operator - 1]][i / 8] |= (1 << (i & 7));
						}
						
						else
						{
							mused.song.instrument[mused.current_instrument].program_unite_bits[mused.current_instrument_program][i / 8] |= (1 << (i & 7));
						}
					}
				}
			}
		}
		break;
		
		case EDITWAVETABLE:
		{
			if(mused.flags & SHOW_WAVEGEN)
			{
				cp_paste_items(&mused.cp, CP_WAVEGEN, &mused.wgset.chain[mused.selected_wg_osc], 1, sizeof(mused.wgset.chain[mused.selected_wg_osc]));
			}
			
			else
			{
				if(mused.cp.type == CP_WAVETABLE)
				{
					if(mused.selection.prev_name_index != mused.selected_wavetable)
					{
						if(mused.mus.cyd->wavetable_entries[mused.selected_wavetable].data)
						{
							free(mused.mus.cyd->wavetable_entries[mused.selected_wavetable].data);
						}
						
						cp_paste_items(&mused.cp, CP_WAVETABLE, &mused.mus.cyd->wavetable_entries[mused.selected_wavetable], 1, sizeof(mused.mus.cyd->wavetable_entries[mused.selected_wavetable]));
						
						memset(mused.song.wavetable_names[mused.selected_wavetable], 0, MUS_WAVETABLE_NAME_LEN + 1);
						
						memcpy(mused.song.wavetable_names[mused.selected_wavetable], mused.song.wavetable_names[mused.selection.prev_name_index], MUS_WAVETABLE_NAME_LEN + 1);
						
						CydWavetableEntry *wave = &mused.mus.cyd->wavetable_entries[mused.selected_wavetable];
						
						Sint16* old_data = wave->data;
						
						if(old_data)
						{
							wave->data = calloc(wave->samples, sizeof(Sint16));
							
							if(wave->data)
							{
								memcpy(wave->data, old_data, wave->samples * sizeof(Sint16));
							}
							
							else
							{
								set_info_message("Out of memory!");
							}
						}
						
						mused.wavetable_preview_idx = 0xff;
					}
				}
			}
		}
		break;
		
		case EDITLOCALSAMPLE:
		{
			if(mused.cp.type == CP_LOCALSAMPLE)
			{
				CydWavetableEntry* source_wave = NULL;
				char* source_wave_name = NULL;
				
				CydWavetableEntry* wave = NULL;
				char* wave_name = NULL;
				
				if(mused.selection.is_local_sample)
				{
					if(mused.song.instrument[mused.selection.local_sample_instrument].num_local_samples >= mused.selection.prev_name_index + 1)
					{
						source_wave = mused.song.instrument[mused.selection.local_sample_instrument].local_samples[mused.selection.prev_name_index];
						source_wave_name = mused.song.instrument[mused.selection.local_sample_instrument].local_sample_names[mused.selection.prev_name_index];
					}
				}
				
				else
				{
					source_wave = &mused.mus.cyd->wavetable_entries[mused.selection.prev_name_index];
					source_wave_name = mused.song.wavetable_names[mused.selection.prev_name_index];
				}
				
				if(mused.show_local_samples_list)
				{
					wave = mused.song.instrument[mused.selection.local_sample_instrument].local_samples[mused.selected_local_sample];
					wave_name = mused.song.instrument[mused.selection.local_sample_instrument].local_sample_names[mused.selected_local_sample];
				}
				
				else
				{
					wave = &mused.mus.cyd->wavetable_entries[mused.selected_local_sample];
					wave_name = mused.song.wavetable_names[mused.selected_local_sample];
				}
				
				if(source_wave == NULL || source_wave_name == NULL || wave == NULL || wave_name == NULL) return;
				
				bool need_to_paste = false;
				
				if(mused.show_local_samples_list)
				{
					if(mused.selection.prev_name_index != mused.selected_local_sample || mused.selection.local_sample_instrument != mused.current_instrument || !(mused.selection.is_local_sample))
					{
						need_to_paste = true;
					}
				}
				
				else
				{
					if(mused.selection.prev_name_index != mused.selected_local_sample || mused.selection.is_local_sample)
					{
						need_to_paste = true;
					}
				}
				
				if(need_to_paste)
				{
					if(wave->data)
					{
						free(wave->data);
						wave->data = NULL;
					}
					
					cp_paste_items(&mused.cp, CP_LOCALSAMPLE, wave, 1, sizeof(*wave));
					
					memset(wave_name, 0, MUS_WAVETABLE_NAME_LEN + 1);
					
					memcpy(wave_name, source_wave_name, MUS_WAVETABLE_NAME_LEN + 1);
					
					//CydWavetableEntry *wave = &mused.mus.cyd->wavetable_entries[mused.selected_wavetable];
					
					//Sint16* old_data = wave->data;
					Sint16* old_data = source_wave->data;
					
					wave->data = calloc(wave->samples, sizeof(Sint16));
					
					if(wave->data)
					{
						memcpy(wave->data, old_data, wave->samples * sizeof(Sint16));
					}
					
					else
					{
						set_info_message("Out of memory!");
					}
					
					mused.wavetable_preview_idx = 0xff;
				}
			}
		}
		break;
	}
}


void join_paste()
{
	switch (mused.focus)
	{
		case EDITSEQUENCE:
		{
			if (mused.cp.type != CP_SEQUENCE) break;
			
			snapshot(S_T_SEQUENCE);
			
			size_t items = cp_get_item_count(&mused.cp, sizeof(mused.song.sequence[0][0]));
			
			if (items < 1) break;
			
			int first = ((MusSeqPattern*)mused.cp.data)[0].position;
			
			for (int i = 0; i < items; ++i)
			{
				add_sequence(mused.current_sequencetrack, ((MusSeqPattern*)mused.cp.data)[i].position-first+mused.current_sequencepos, ((MusSeqPattern*)mused.cp.data)[i].pattern, ((MusSeqPattern*)mused.cp.data)[i].note_offset);
			}
		}
		break;
		
		case EDITPATTERN:
		{
			size_t items = cp_get_item_count(&mused.cp, sizeof(mused.song.pattern[0].step[0]));
			
			if (items < 1) break;
			
			if (mused.cp.type == CP_PATTERN)
			{
				snapshot(S_T_PATTERN);
				
				int ofs;
				
				if (mused.cp.type == CP_PATTERN) 
					ofs = 0;
				else
					ofs = current_patternstep();
				
				for (int i = 0; i < items && i + ofs < mused.song.pattern[current_pattern()].num_steps; ++i)
				{
					const MusStep *s = &((MusStep*)mused.cp.data)[i];
					MusStep *d = &mused.song.pattern[current_pattern()].step[ofs + i];
					if (s->note != MUS_NOTE_NONE)
						d->note = s->note;
						
					if (s->volume != MUS_NOTE_NO_VOLUME)
						d->volume = s->volume;
						
					if (s->instrument != MUS_NOTE_NO_INSTRUMENT)
						d->instrument = s->instrument;
						
					for(int i = 0; i < MUS_MAX_COMMANDS; ++i)
					{
						if (s->command[i] != 0)
							d->command[i] = s->command[i];
					}
						
					if (s->ctrl != 0)
						d->ctrl = s->ctrl;
				}
			}
			
			if(mused.cp.type == CP_PATTERNSEGMENT)
			{
				snapshot(S_T_PATTERN);
				
				if(mused.cp.data != NULL)
				{
					MusStep* s = &mused.song.pattern[current_pattern()].step[current_patternstep()];
					
					MusStep* cp_step = (MusStep*)mused.cp.data;
					
					for(int i = 0; i < mused.cp.pattern_length && i + current_patternstep() < mused.song.pattern[current_pattern()].num_steps; ++i) //paste
					{
						if(mused.cp.patternx_start == 0 && mused.cp.patternx_end >= 0 && cp_step[i].note != MUS_NOTE_NONE)
						{
							s[i].note = cp_step[i].note;
						}
						
						if(mused.cp.patternx_start <= 1 && mused.cp.patternx_end >= 1 && cp_step[i].instrument != MUS_NOTE_NO_INSTRUMENT)
						{
							s[i].instrument |= (cp_step[i].instrument & 0xf0);
						}
						
						if(mused.cp.patternx_start <= 2 && mused.cp.patternx_end >= 2 && cp_step[i].instrument != MUS_NOTE_NO_INSTRUMENT)
						{
							s[i].instrument |= (cp_step[i].instrument & 0x0f);
						}
						
						if(mused.cp.patternx_start <= 3 && mused.cp.patternx_end >= 3 && cp_step[i].volume != MUS_NOTE_NO_VOLUME)
						{
							s[i].volume |= (cp_step[i].volume & 0xf0);
						}
						
						if(mused.cp.patternx_start <= 4 && mused.cp.patternx_end >= 4 && cp_step[i].volume != MUS_NOTE_NO_VOLUME)
						{
							s[i].volume |= (cp_step[i].volume & 0x0f);
						}
						
						if(mused.cp.patternx_start <= 5 && mused.cp.patternx_end >= 9 && cp_step[i].ctrl != 0)
						{
							s[i].ctrl = 0;
						}
						
						if(mused.cp.patternx_start <= 5 && mused.cp.patternx_end >= 5 && cp_step[i].ctrl != 0)
						{
							s[i].ctrl |= (cp_step[i].ctrl & 0b0001);
						}
						
						if(mused.cp.patternx_start <= 6 && mused.cp.patternx_end >= 6 && cp_step[i].ctrl != 0)
						{
							s[i].ctrl |= (cp_step[i].ctrl & 0b0010);
						}
						
						if(mused.cp.patternx_start <= 7 && mused.cp.patternx_end >= 7 && cp_step[i].ctrl != 0)
						{
							s[i].ctrl |= (cp_step[i].ctrl & 0b0100);
						}
						
						if(mused.cp.patternx_start <= 8 && mused.cp.patternx_end >= 8 && cp_step[i].ctrl != 0)
						{
							s[i].ctrl |= (cp_step[i].ctrl & 0b1000);
						}
						
						for(int j = 0; j < MUS_MAX_COMMANDS; ++j)
						{
							if(cp_step[i].command[j] != 0)
							{
								if(mused.cp.patternx_start <= 9 + j * 4 && mused.cp.patternx_end >= 9 + j * 4)
								{
									s[i].command[j] |= (cp_step[i].command[j] & (Uint16)0xf000);
								}
								
								if(mused.cp.patternx_start <= 10 + j * 4 && mused.cp.patternx_end >= 10 + j * 4)
								{
									s[i].command[j] |= (cp_step[i].command[j] & 0x0f00);
								}
								
								if(mused.cp.patternx_start <= 11 + j * 4 && mused.cp.patternx_end >= 11 + j * 4)
								{
									s[i].command[j] |= (cp_step[i].command[j] & 0x00f0);
								}
								
								if(mused.cp.patternx_start <= 12 + j * 4 && mused.cp.patternx_end >= 12 + j * 4)
								{
									s[i].command[j] |= (cp_step[i].command[j] & 0x000f);
								}
							}
						}
					}
				}
			}
		}
		break;
	}
}


void begin_selection(int position)
{
	//mused.selection.start = mused.selection.end;
	mused.selection.keydown = position;
	debug("Selected from %d", position);
}


void select_range(int position)
{
	mused.selection.start = mused.selection.keydown;
	
	if (mused.selection.end == position)
	{
		int extra = 1;
		
		if (mused.focus == EDITSEQUENCE)
			extra = mused.sequenceview_steps;
		
		mused.selection.end = position + extra; // so we can select the last row (can't move the cursor one element after the last)
	}
	
	else
	{
		mused.selection.end = position;
	}
		
	if (mused.selection.end < mused.selection.start)
	{
		swap(mused.selection.start, mused.selection.end);
	}
	
	mused.selection.patternx_start = 0;
	mused.selection.patternx_end = 1 + 2 + 2 + 4 + ((mused.command_columns[mused.current_sequencetrack] + 1) * 4) - 1; //so we do as in mouse drag selection but like if we selected all pattern columns (selecting note, instrument, volume, control bits and all visible commands)
	
	debug("Selected from %d-%d", mused.selection.start, mused.selection.end);
}
