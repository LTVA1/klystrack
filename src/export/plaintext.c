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

#include "plaintext.h"
#include "../mused.h"
#include "../view.h"

extern Mused mused;

static char** lines;
static char* final_line;
static Uint32 num_lines;

#define PATTERN_LINE_LENGTH (3 + 2 + 2 + 4 + 4 * 8 + 1) //note, inst, vol, control bits, 8 effects and "|"

static void create_pattern_line(MusStep* s, char* line)
{
	Uint16 line_pos = 0;
	Uint8 sel_start = 0;
	Uint8 sel_end = 0;

	if(mused.selection.start == mused.selection.end) //copy entire visible pattern
	{
		sel_end = 1 + 2 + 2 + 4 + ((mused.command_columns[mused.current_sequencetrack] + 1) * 4) - 1;
	}

	else
	{
		sel_start = mused.selection.patternx_start;
		sel_end = mused.selection.patternx_end;
	}

	if(sel_start == 0 && sel_end >= 0)
	{
		if(s->note < FREQ_TAB_SIZE)
		{
			memcpy(&line[line_pos], notename_default(s->note), 3);
		}

		else if(s->note == MUS_NOTE_CUT)
		{
			memcpy(&line[line_pos], KLYSTRACK_PLAINTEXT_NOTE_CUT, 3);
		}

		else if(s->note == MUS_NOTE_RELEASE)
		{
			memcpy(&line[line_pos], KLYSTRACK_PLAINTEXT_NOTE_RELEASE, 3);
		}

		else if(s->note == MUS_NOTE_MACRO_RELEASE)
		{
			memcpy(&line[line_pos], KLYSTRACK_PLAINTEXT_NOTE_MACRO_RELEASE, 3);
		}

		else if(s->note == MUS_NOTE_RELEASE_WITHOUT_MACRO)
		{
			memcpy(&line[line_pos], KLYSTRACK_PLAINTEXT_NOTE_NOTE_RELEASE, 3);
		}

		else
		{
			memcpy(&line[line_pos], "...", 3);
		}

		line_pos += 3;
	}

	if((sel_start <= 1 && sel_end >= 1) || (sel_start <= 2 && sel_end >= 2))
	{
		char ins[4] = { 0 };

		if(s->instrument != MUS_NOTE_NO_INSTRUMENT)
		{
			snprintf(ins, 3, "%02X", s->instrument);
			memcpy(&line[line_pos], ins, 2);
		}

		else
		{
			memcpy(&line[line_pos], "..", 2);
		}

		line_pos += 2;
	}

	if((sel_start <= 3 && sel_end >= 3) || (sel_start <= 4 && sel_end >= 4))
	{
		char vol[4] = { 0 };

		if(s->volume <= MAX_VOLUME)
		{
			snprintf(vol, 3, "%02X", s->volume);
			memcpy(&line[line_pos], vol, 2);
		}

		else if (s->volume > MAX_VOLUME && s->volume != MUS_NOTE_NO_VOLUME)
		{
			char c;
			
			switch (s->volume & 0xf0)
			{
				default:
					c = '-';
					break;
					
				case MUS_NOTE_VOLUME_SET_PAN:
					c = 'P';
					break;
					
				case MUS_NOTE_VOLUME_PAN_LEFT:
					c = 'L';
					break;
					
				case MUS_NOTE_VOLUME_PAN_RIGHT:
					c = 'R';
					break;
					
				case MUS_NOTE_VOLUME_FADE_UP:
					c = 'U';
					break;
					
				case MUS_NOTE_VOLUME_FADE_DN:
					c = 'D';
					break;
					
				case MUS_NOTE_VOLUME_FADE_UP_FINE:
					c = 'a';
					break;
					
				case MUS_NOTE_VOLUME_FADE_DN_FINE:
					c = 'b';
					break;
			}

			snprintf(vol, 3, "%c%01X", c, (s->volume & 0xf));
			memcpy(&line[line_pos], vol, 2);
		}

		else
		{
			memcpy(&line[line_pos], "..", 2);
		}

		line_pos += 2;
	}

	if((sel_start <= 5 && sel_end >= 5) || (sel_start <= 6 && sel_end >= 6) || (sel_start <= 7 && sel_end >= 7) || (sel_start <= 8 && sel_end >= 8))
	{
		char ctrl[6] = { 0 };
		snprintf(ctrl, 5, "%c%c%c%c", ((s->ctrl & MUS_CTRL_LEGATO) ? 'L' : '.'),
			((s->ctrl & MUS_CTRL_SLIDE) ? 'S' : '.'),
			((s->ctrl & MUS_CTRL_VIB) ? 'V' : '.'),
			((s->ctrl & MUS_CTRL_TREM) ? 'T' : '.'));
		
		memcpy(&line[line_pos], ctrl, 4);

		line_pos += 4;
	}

	for(int j = 0; j < MUS_MAX_COMMANDS; ++j)
	{
		if((sel_start <= 9 + j * 4 && sel_end >= 9 + j * 4) ||
		(sel_start <= 10 + j * 4 && sel_end >= 10 + j * 4) ||
		(sel_start <= 11 + j * 4 && sel_end >= 11 + j * 4) ||
		(sel_start <= 12 + j * 4 && sel_end >= 12 + j * 4))
		{
			char command[6] = { 0 };

			if(s->command[j] != 0)
			{
				snprintf(command, 5, "%04X", s->command[j]);
				memcpy(&line[line_pos], command, 4);
			}

			else
			{
				memcpy(&line[line_pos], "....", 4);
			}

			line_pos += 4;
		}
	}

	line[line_pos] = '|';
}

static void add_pattern_lines(MusPattern* pattern, Uint16 first_step, Uint16 steps_to_copy)
{
	for(int i = 0; i < steps_to_copy; i++)
	{
		num_lines++;
		lines = (char**)realloc(lines, sizeof(char*) * num_lines);
		lines[num_lines - 1] = (char*)calloc(1, sizeof(char) * (PATTERN_LINE_LENGTH + 1));
		memset(lines[num_lines - 1], 0, sizeof(char) * (PATTERN_LINE_LENGTH + 1));

		MusStep* s = &pattern->step[i + first_step];

		create_pattern_line(s, lines[num_lines - 1]);
	}
}

void create_plain_text() //create a specially formatted string to allow plain-text sharing of pattern segments and sequence segments; also allows for klystrack-to-klystrack instances copypastes
{
	debug("put pattern plain text in clipboard");

	lines = NULL;
	final_line = NULL;
	num_lines = 2;

	//mused.cp.patternx_start mused.cp.patternx_end

	char version[5] = { 0 };
	snprintf(version, sizeof(version), "%d", MUS_VERSION);

	lines = (char**)calloc(1, sizeof(char*) * 2);
	//                                              "KLYSTRACK_PLAINTEXT_SIG - KLYSTRACK_PATTERN_SEGMENT_SIG (MUS_VERSION)\0"
	lines[0] = (char*)calloc(1, sizeof(char) * (strlen(KLYSTRACK_PLAINTEXT_SIG) + 1 + 1 + 1 + strlen(KLYSTRACK_PATTERN_SEGMENT_SIG) + 1 + 1 + strlen(version) + 1 + 1));
	snprintf(lines[0], 200, "%s - %s (%s)", KLYSTRACK_PLAINTEXT_SIG, KLYSTRACK_PATTERN_SEGMENT_SIG, version);

	lines[1] = (char*)calloc(1, sizeof(char) * 15);

	if (mused.selection.start == mused.selection.end && get_current_pattern())
	{
		snprintf(lines[1], 5, "%d %d", 0, 1 + 2 + 2 + 4 + ((mused.command_columns[mused.current_sequencetrack] + 1) * 4) - 1);
	}

	else
	{
		snprintf(lines[1], 5, "%d %d", mused.selection.patternx_start, mused.selection.patternx_end);
	}

	MusPattern* pattern = NULL;
	Uint16 steps_to_copy = 0;
	Uint16 first_step = 0;

	if (mused.selection.start == mused.selection.end && get_current_pattern()) //copy whole pattern
	{
		pattern = get_current_pattern();
		steps_to_copy = get_current_pattern()->num_steps;
		first_step = 0;
	}
	
	else if(get_pattern(mused.selection.start, mused.current_sequencetrack) != -1) //copy part of the pattern; here mused.selection.start is in absolute song coordinates so we need to figure out offset relative to pattern start
	{
		pattern = &mused.song.pattern[get_pattern(mused.selection.start, mused.current_sequencetrack)];

		steps_to_copy = mused.selection.end - mused.selection.start;

		int pattern_offset = 0;

		for(int i = 0; i < NUM_SEQUENCES; ++i)
		{
			const MusSeqPattern *sp = &mused.song.sequence[mused.current_sequencetrack][i];
			
			if(sp->position + mused.song.pattern[sp->pattern].num_steps >= mused.current_patternpos && sp->position <= mused.current_patternpos)
			{
				pattern_offset = sp->position;
				break;
			}
		}

		first_step = mused.selection.start - pattern_offset;
	}

	if(pattern && steps_to_copy != 0) //we have smth to copy
	{
		add_pattern_lines(pattern, first_step, steps_to_copy);
	}

	else
	{
		goto end;
	}

	Uint32 final_line_length = 0;

	for(int i = 0; i < num_lines; i++)
	{
		final_line_length += strlen(lines[i]) + 1; //+ 1 for \n
	}

	final_line_length += 1; //for \0 at line end

	final_line = (char*)calloc(1, sizeof(char) * final_line_length);

	Uint32 curr_pos = 0;

	for(int i = 0; i < num_lines; i++)
	{
		strcpy(&final_line[curr_pos], lines[i]);
		curr_pos += strlen(lines[i]);
		final_line[curr_pos] = '\n';
		curr_pos++;
	}

	curr_pos--;
	final_line[curr_pos] = '\0';

	SDL_SetClipboardText(final_line);

	end:;

	if(lines)
	{
		for(int i = 0; i < num_lines; i++)
		{
			free(lines[i]);
		}

		free(lines);
	}

	if(final_line)
	{
		free(final_line);
	}
}