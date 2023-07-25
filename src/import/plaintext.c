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
#include "SDL.h"

#include <stdio.h>

#include "../mused.h"
#include "../view.h"
#include "../../klystron/src/gui/msgbox.h"
#include "../../klystron/src/gui/bevdefs.h"
#include "../../klystron/src/gui/view.h"
#include "../../klystron/src/gui/toolutil.h"
#include "../../klystron/src/gui/bevel.h"
#include "../../klystron/src/gui/dialog.h"
#include "../../klystron/src/gui/mouse.h"

extern Mused mused;
extern GfxDomain* domain;

enum
{
	PATTERN_TEXT_FORMAT_OPENMPT_MOD,
	PATTERN_TEXT_FORMAT_OPENMPT_S3M,
	PATTERN_TEXT_FORMAT_OPENMPT_XM,
	PATTERN_TEXT_FORMAT_OPENMPT_XM_NEW,
	PATTERN_TEXT_FORMAT_OPENMPT_IT,
	PATTERN_TEXT_FORMAT_OPENMPT_IT_NEW,
	PATTERN_TEXT_FORMAT_OPENMPT_MPT,

	PATTERN_TEXT_FORMAT_DEFLEMASK,
	PATTERN_TEXT_FORMAT_FURNACE,

	PATTERN_TEXT_FORMAT_KLYSTRACK,

	PATTERN_TEXT_FORMATS,
};

static const char* text_format_headers[] =
{
	"ModPlug Tracker MOD",
	"ModPlug Tracker S3M",
	"ModPlug Tracker  XM",
	"ModPlug Tracker XM",
	"ModPlug Tracker  IT",
	"ModPlug Tracker IT",
	"ModPlug Tracker MPT",

	" ",
	"org.tildearrow.furnace",

	" ",
};

static char values[1 + 1 + 1 + 8][5]; //note, inst, volume and up to 8 effects
static Uint8** mask;


static const char delimiters_lines[] = "\n\r";
static const char delimiters[] = "|";

static char* plain_text_string_copy;

static Uint8 fur_version;

static Uint16 current_line_index = 0;

static char** lines;
static char** pattern_rows; //size = num_patterns
static Uint16 num_lines;

static MusPattern* patterns;
static Uint8 num_patterns;

static Uint16 passes;
static char* current_line;

static Uint8 format;

static Uint8 furnace_start_column; //0 = note, 1 = inst, 2 = vol, 3 = 2 most significant digits of effect 1, 4 = 2 least significant digits of effect 1, ...

static void process_row_openmpt(MusPattern* pat, Uint16 s, Uint8 format, Uint16 channel)
{
	if(strcmp(values[0], "   ") == 0) //mark which columns are empty in the text (it means we must not paste data from them into destination pattern but rather we keep old pattern data)
	{
		mask[channel][0] = 0;
	}

	if(strcmp(values[1], "  ") == 0)
	{
		mask[channel][1] = 0;
	}

	if(strcmp(values[2], "   ") == 0)
	{
		mask[channel][2] = 0;
	}

	if(strcmp(values[3], "   ") == 0)
	{
		mask[channel][3] = 0;
	}

	MusStep* step = &pat->step[s];

	for(int i = 0; i < FREQ_TAB_SIZE; i++)
	{
		char* ref_note = notename_default(i);

		if(strcmp(ref_note, values[0]) == 0)
		{
			step->note = i - 12; //an octave lower
			goto next;
		}
	}
	
	next:;

	if(strcmp("^^^", values[0]) == 0)
	{
		step->note = MUS_NOTE_CUT;
	}

	if(strcmp("===", values[0]) == 0)
	{
		step->note = MUS_NOTE_RELEASE;
	}

	if(strcmp(values[1], "..") != 0)
	{
		step->instrument = atoi(values[1]);
	}

	if(format < PATTERN_TEXT_FORMAT_DEFLEMASK && format >= PATTERN_TEXT_FORMAT_OPENMPT_MOD)
	{
		char volume_func = values[2][0];
		Uint8 volume_param = 0;

		if(strcmp(&values[2][1], "  ") != 0)
		{
			volume_param = atoi(&values[2][1]);
		}

		switch(volume_func)
		{
			case 'v': //just volume
			{
				step->volume = volume_param * 2;
				break;
			}

			case 'p': //panning
			{
				step->volume = MUS_NOTE_VOLUME_SET_PAN | ((volume_param - 1) >> 2); //0-64 to 0-15
				break;
			}

			case 'l': //slide panning left
			{
				step->volume = MUS_NOTE_VOLUME_PAN_LEFT | volume_param; //0-15
				break;
			}

			case 'r': //slide panning right
			{
				step->volume = MUS_NOTE_VOLUME_PAN_RIGHT | volume_param; //0-15
				break;
			}

			case 'c': //volume up
			{
				step->volume = MUS_NOTE_VOLUME_FADE_UP | ((volume_param * 15) / 9); //0-9 to 0-15
				break;
			}

			case 'd': //volume down
			{
				step->volume = MUS_NOTE_VOLUME_FADE_DN | ((volume_param * 15) / 9); //0-9 to 0-15
				break;
			}

			case 'a': //volume up fine
			{
				if(volume_param > 0) //because klystrack range lol, volume_param = 0 would be 0x80 which is valid volume velue
				{
					step->volume = MUS_NOTE_VOLUME_FADE_UP_FINE | ((volume_param * 15) / 9); //0-9 to 0-15
				}
				break;
			}

			case 'b': //volume down fine
			{
				step->volume = MUS_NOTE_VOLUME_FADE_DN_FINE | ((volume_param * 15) / 9); //0-9 to 0-15
				break;
			}

			//other commands not supported

			default: break;
		}
	}

	if(format < PATTERN_TEXT_FORMAT_DEFLEMASK)
	{
		char command_digit = values[3][0];

		Uint8 command_param = 0;

		if(strcmp(&values[3][1], "..") != 0)
		{
			command_param = strtol(&values[3][1], NULL, 16);

			//debug("command param %d", command_param);
		}

		Uint16 command = 0;

		if(format == PATTERN_TEXT_FORMAT_OPENMPT_XM || format == PATTERN_TEXT_FORMAT_OPENMPT_XM_NEW || format == PATTERN_TEXT_FORMAT_OPENMPT_MOD)
		{
			if((command_digit >= '0' && command_digit <= '9'))
			{
				char aaa[2];
				aaa[0] = command_digit;
				aaa[1] = '\0';
				Uint16 command_num = (atoi(aaa) << 8);

				//debug("%d", command_num);

				command = command_num | command_param;
			}

			else if(command_digit >= 'A' && command_digit <= 'F')
			{
				Uint16 command_num = ((0xF - ('F' - command_digit)) << 8);

				//debug("1 %d", command_num);

				command = command_num | command_param;
			}

			else
			{
				command = ((Uint16)command_digit << 8) | command_param;
			}
		}

		if(format == PATTERN_TEXT_FORMAT_OPENMPT_S3M || format == PATTERN_TEXT_FORMAT_OPENMPT_IT || format == PATTERN_TEXT_FORMAT_OPENMPT_IT_NEW || format == PATTERN_TEXT_FORMAT_OPENMPT_MPT)
		{
			command = ((Uint16)(command_digit - 'A' + 1) << 8) | command_param; //Axx = 0x1XX so +1
		}

		switch(format)
		{
			case PATTERN_TEXT_FORMAT_OPENMPT_MOD:
			{
				step->command[0] = find_command_pt(command, 0xff * 256); //so at the end we have full 0-fff range
				break;
			}

			case PATTERN_TEXT_FORMAT_OPENMPT_XM:
			case PATTERN_TEXT_FORMAT_OPENMPT_XM_NEW:
			{
				find_command_xm(command, step);
				break;
			}

			case PATTERN_TEXT_FORMAT_OPENMPT_S3M:
			{
				find_command_s3m(command, step);
				break;
			}

			case PATTERN_TEXT_FORMAT_OPENMPT_IT:
			case PATTERN_TEXT_FORMAT_OPENMPT_IT_NEW:
			{
				find_command_it(command, step);
				break;
			}

			case PATTERN_TEXT_FORMAT_OPENMPT_MPT:
			{
				find_command_mptm(command, step);
				break;
			}

			default: break;
		}
	}
}

static void process_row_furnace(MusPattern* pat, Uint16 s, Uint8 furnace_version, Uint16 channel)
{
	//debug("%s %s %s %s %s %s %s %s %s %s %s", values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8], values[9], values[10]);

	if(channel == 0)
	{
		for(int i = 0; i < furnace_start_column; i++)
		{
			mask[channel][i] = 1;
		}
	}

	else
	{
		if(strcmp(values[0], "") == 0) //note
		{
			mask[channel][0] = 1;
		}

		if(strcmp(values[1], "") == 0) //inst
		{
			mask[channel][1] = 1;
		}

		if(strcmp(values[2], "") == 0) //volume
		{
			mask[channel][2] = 1;
		}

		for(int i = 3; i < 3 + 8 * 2; i += 2) //effects
		{
			if(strlen(values[3 + i / 2]) == 2) //only first 2 digits
			{
				mask[channel][i] = 1;
			}

			if(strlen(values[3 + i / 2]) == 4) //all 4 digits
			{
				mask[channel][i] = 1;
				mask[channel][i + 1] = 1;
			}
		}
	}

	MusStep* step = &pat->step[s];

	if(strcmp(values[0], "...") != 0 && strcmp(values[0], "") != 0)
	{
		for(int i = 0; i < FREQ_TAB_SIZE; i++)
		{
			char* ref_note = notename_default(i);

			if(strcmp(ref_note, values[0]) == 0)
			{
				step->note = i;
				goto next;
			}
		}
		
		next:;

		if(strcmp("REL", values[0]) == 0)
		{
			step->note = MUS_NOTE_MACRO_RELEASE;
		}

		if(strcmp("OFF", values[0]) == 0)
		{
			step->note = MUS_NOTE_CUT;
		}

		if(strcmp("===", values[0]) == 0)
		{
			step->note = MUS_NOTE_RELEASE;
		}
	}

	if(strcmp(values[1], "..") != 0 && strcmp(values[1], "") != 0)
	{
		step->instrument = strtol(values[1], NULL, 16);
	}

	if(strcmp(values[2], "..") != 0 && strcmp(values[2], "") != 0)
	{
		step->volume = strtol(values[2], NULL, 16);

		if(step->volume == 0x7F)
		{
			step->volume = MAX_VOLUME;
		}
	}

	for(int i = 3; i < 11; i++)
	{
		if(strlen(values[i]) == 2 && strcmp(values[i], "..") != 0) //only 1st two digits of a command
		{
			step->command[i - 3] = (strtol(values[i], NULL, 16) << 8);
		}

		if(strlen(values[i]) == 4 && strcmp(values[i], "....") != 0) //all 4 digits
		{
			if(memcmp(values[i], "..", 2) == 0) //..55 case, so we need 0055 as a result
			{
				step->command[i - 3] = strtol(&values[i][2], NULL, 16); //TODO: add function for effects conversion, think about how to tell it which chip effect belongs to (in Furnace effects change their meaning depending on the chip they are applied to)
			}

			else if(memcmp(&values[i][2], "..", 2) == 0) //55.. case, we need 5500 as a result
			{
				values[i][2] = '\0'; //place null terminator so "55.." becomes "55"; values array will still be overwritten at next step so we can modify it
				step->command[i - 3] = (strtol(values[i], NULL, 16) << 8);
			}

			else //usual case, like 0120
			{
				step->command[i - 3] = strtol(values[i], NULL, 16);
			}
		}
	}
}

static void process_lines_openmpt()
{
	passes = 0;

	current_line = 1;

	char* lines_1_copy = (char*)calloc(1, sizeof(char*) * (strlen(lines[1]) + 1)); //making a copy of the first pattern line and restore its original state for actual parsing (strtok() puts null terminators right inside the string)
	strcpy(lines_1_copy, lines[1]);

	while(current_line != NULL) //process 1st line; count how many channels are in buffer (practically we count "|" characters)
	{
		current_line = strtok(passes > 0 ? NULL : lines[1], delimiters); //lines[0] is header
		passes++;
		
		if(current_line)
		{
			num_patterns++;
			pattern_rows = (char**)realloc(pattern_rows, sizeof(char*) * num_patterns);
			pattern_rows[num_patterns - 1] = current_line;

			//debug("pattern_rows[num_patterns - 1] \"%s\"", pattern_rows[num_patterns - 1]);
		}
	}

	//debug("num_patterns %d, num_lines %d", num_patterns, num_lines - 1);

	patterns = (MusPattern*)calloc(1, sizeof(MusPattern) * num_patterns);
	memset(patterns, 0, sizeof(MusPattern) * num_patterns);

	for(int i = 0; i < num_patterns; i++)
	{
		resize_pattern(&patterns[i], num_lines);
	}

	strcpy(lines[1], lines_1_copy);
	free(lines_1_copy);

	mask = (Uint8**)calloc(1, sizeof(Uint8*) * num_patterns);

	for(int i = 0; i < num_patterns; i++)
	{
		mask[i] = (Uint8*)calloc(1, sizeof(Uint8) * (1 + 1 + 1 + 8 * 2)); //which columns we are to paste; 8 * 2 since Furnace has disctinction between 1st 2 and last 2 digits of effect

		for(int j = 0; j < (1 + 1 + 1 + 8 * 2); j++)
		{
			mask[i][j] = 1; //by default we paste all columns, those we should not paste are marked in process_row_openmpt() and similar functions for other formats
		}
	}

	for(int i = 1; i < num_lines; i++) //this should only be executed if we have more than 1 pattern row of data
	{
		current_line = 1;
		current_line_index = 0;
		passes = 0;

		while(current_line != NULL)
		{
			current_line = strtok(passes > 0 ? NULL : lines[i], delimiters);
			passes++;
			
			if(current_line)
			{
				pattern_rows[current_line_index] = current_line;
				current_line_index++;

				memset(values, 0, (1 + 1 + 1 + 8) * (5) * sizeof(char));
				memcpy(values[0], pattern_rows[current_line_index - 1], 3 * sizeof(char));
				memcpy(values[1], &pattern_rows[current_line_index - 1][3], 2 * sizeof(char));
				memcpy(values[2], &pattern_rows[current_line_index - 1][3 + 2], 3 * sizeof(char));
				memcpy(values[3], &pattern_rows[current_line_index - 1][3 + 2 + 3], 3 * sizeof(char));

				process_row_openmpt(&patterns[current_line_index - 1], i - 1, format, current_line_index - 1);
			}
		}
	}
}

//org.tildearrow.furnace - Pattern Data (144)

static void process_lines_furnace()
{
	char sig[100] = { 0 };
	char first_format_word[100] = { 0 }; //I know sscanf("%[^\n]", shit); syntax exists but I don't want to brain
	char second_format_word[100] = { 0 };
	
	sscanf(lines[0], "%s - %s %s (%d)", sig, first_format_word, second_format_word, &fur_version);

	//debug("%s - %s %s (%d)", sig, first_format_word, second_format_word, fur_version);

	if(strcmp(first_format_word, "Pattern") == 0 && strcmp(second_format_word, "Data") == 0)
	{
		furnace_start_column = strtol(lines[1], NULL, 10); //the column from which we start in the very 1st channel

		Uint16 len = strlen(lines[2]);
		num_patterns = 0;

		for(int i = 0; i < len; i++)
		{
			if(lines[2][i] == '|')
			{
				num_patterns++;
			}
		}

		patterns = (MusPattern*)calloc(1, sizeof(MusPattern) * num_patterns);
		memset(patterns, 0, sizeof(MusPattern) * num_patterns);

		for(int i = 0; i < num_patterns; i++)
		{
			resize_pattern(&patterns[i], num_lines - 2);
		}

		Uint8 current_channel = 0;
		Uint16 string_pos = 0;

		mask = (Uint8**)calloc(1, sizeof(Uint8*) * num_patterns);

		for(int i = 0; i < num_patterns; i++)
		{
			mask[i] = (Uint8*)calloc(1, sizeof(Uint8) * (1 + 1 + 1 + 8 * 2)); //which columns we are to paste; 8 * 2 since Furnace has disctinction between 1st 2 and last 2 digits of effect

			for(int j = 0; j < (1 + 1 + 1 + 8 * 2); j++)
			{
				mask[i][j] = 1; //by default we paste all columns, those we should not paste are marked in process_row_openmpt() and similar functions for other formats
			}
		}

		//debug("num lines %d", num_lines);

		for(int i = 2; i < num_lines; i++)
		{
			string_pos = 0;
			current_channel = 0;

			while(current_channel < num_patterns)
			{
				if(current_channel == 0)
				{
					memset(values, 0, (1 + 1 + 1 + 8) * (5) * sizeof(char));

					if(furnace_start_column == 0)
					{
						//debug("dd0 %s", &lines[i][string_pos]);
						memcpy(values[0], &lines[i][string_pos], 3 * sizeof(char));
						string_pos += 3;

						if(lines[i][string_pos] == '|') goto process1;

						goto inst; //yes I use a chain of goto's here; it's awful, disgusting, bad, ugly, amateur, wrong, insufferable and generally bad practice.
					}

					if(furnace_start_column == 1)
					{
						inst:;
						//debug("dd1 %s", &lines[i][string_pos]);
						memcpy(values[1], &lines[i][string_pos], 2 * sizeof(char));
						string_pos += 2;

						if(lines[i][string_pos] == '|') goto process1;

						goto volume;
					}

					if(furnace_start_column == 2)
					{
						volume:;
						//debug("dd2 %s", &lines[i][string_pos]);
						memcpy(values[2], &lines[i][string_pos], 2 * sizeof(char));
						string_pos += 2;

						if(lines[i][string_pos] == '|') goto process1;

						goto effects;
					}

					if(furnace_start_column > 2)
					{
						effects:;
						Uint8 effect = (furnace_start_column - 3) / 2;

						if(furnace_start_column <= 2)
						{
							effect = 0;
						}

						while(lines[i][string_pos] != '|')
						{
							//debug("dd3 %s", &lines[i][string_pos]);
							memcpy(values[3 + effect], &lines[i][string_pos], 2 * sizeof(char)); //1st half of the effect
							string_pos += 2;

							//debug("dd4 %s", &lines[i][string_pos]);

							if(lines[i][string_pos] != '|') //if selection did not end on first 2 digits of the command
							{
								memcpy(&values[3 + effect][2], &lines[i][string_pos], 2 * sizeof(char)); //2nd half of the effect
								string_pos += 2;
							}

							effect++;
						}

						process1:;

						process_row_furnace(&patterns[current_channel], i - 2, fur_version, current_channel);
						current_channel++;
						string_pos++; //jump over '|' character
					}
				}

				else
				{
					memset(values, 0, (1 + 1 + 1 + 8) * (5) * sizeof(char));

					//debug("dd %s", &lines[i][string_pos]);

					memcpy(values[0], &lines[i][string_pos], 3 * sizeof(char));
					string_pos += 3;

					//debug("dd %s", &lines[i][string_pos]);

					if(lines[i][string_pos] == '|') goto process;

					memcpy(values[1], &lines[i][string_pos], 2 * sizeof(char));
					string_pos += 2;

					//debug("dd %s", &lines[i][string_pos]);

					if(lines[i][string_pos] == '|') goto process;

					memcpy(values[2], &lines[i][string_pos], 2 * sizeof(char));
					string_pos += 2;

					//debug("dd %s", &lines[i][string_pos]);

					if(lines[i][string_pos] == '|') goto process;

					Uint8 effect = 0;

					while(lines[i][string_pos] != '|')
					{
						//debug("dd %s", &lines[i][string_pos]);
						memcpy(values[3 + effect], &lines[i][string_pos], 2 * sizeof(char)); //1st half of the effect
						string_pos += 2;

						if(lines[i][string_pos] != '|') //if selection did not end on first 2 digits of the command
						{
							//debug("dd %s", &lines[i][string_pos]);
							memcpy(&values[3 + effect][2], &lines[i][string_pos], 2 * sizeof(char)); //2nd half of the effect
							string_pos += 2;
						}

						effect++;
					}

					process:;

					process_row_furnace(&patterns[current_channel], i - 2, fur_version, current_channel);
					current_channel++;

					string_pos++; //jump over '|' character
				}
			}
		}

		//debug("finish proc lines");
	}

	else
	{
		return;
	}
}

void paste_from_clipboard()
{
	if(mused.focus != EDITPATTERN)
	{
		return;
	}

	char* plain_text_string = SDL_GetClipboardText();
	plain_text_string_copy = NULL;

	fur_version = 0;

	current_line_index = 0;

	lines = NULL;
	pattern_rows = NULL; //size = num_patterns
	num_lines = 0;
	memset(values, 0, (1 + 1 + 1 + 8) * (5) * sizeof(char));

	patterns = NULL;
	num_patterns = 0;

	mask = NULL;

	if(strcmp(plain_text_string, "") == 0)
	{
		msgbox(domain, mused.slider_bevel, &mused.largefont, "Clipboard is empty!", MB_OK);
		
		goto end;
	}

	format = 0;

	for(int i = 0; i < PATTERN_TEXT_FORMATS; i++)
	{
		if(memcmp(plain_text_string, text_format_headers[i], strlen(text_format_headers[i])) == 0)
		{
			format = i;
			debug("string format %d", format);
			goto process;
		}
	}

	debug("string format not supported");

	msgbox(domain, mused.slider_bevel, &mused.largefont, "Text format is unknown or text is corrupt!", MB_OK);
		
	goto end;

	process:;

	plain_text_string_copy = (char*)calloc(1, strlen(plain_text_string) + 1);
	strcpy(plain_text_string_copy, plain_text_string);

	current_line = 1;

	passes = 0;

	while(current_line != NULL) //divide into lines
	{
		current_line = strtok(passes > 0 ? NULL : plain_text_string_copy, delimiters_lines);
		passes++;
		
		if(current_line)
		{
			debug("line \"%s\"", current_line);
			num_lines++;
			lines = (char**)realloc(lines, sizeof(char*) * num_lines);
			lines[num_lines - 1] = current_line;
		}
	}

	if(format < PATTERN_TEXT_FORMAT_DEFLEMASK)
	{
		process_lines_openmpt();
	}

	if(format == PATTERN_TEXT_FORMAT_FURNACE)
	{
		process_lines_furnace();
	}

	for(int i = 0; i < num_patterns; i++) //paste stuff into patterns; "watch your step!" :)
	{
		bool need_to_paste = false;
		MusPattern* pat;

		if(mused.current_sequencetrack != -1) //check if pattern is there on every channel
		{
			int pattern = current_pattern_for_channel(mused.current_sequencetrack + i);

			if(pattern != -1)
			{
				pat = &mused.song.pattern[pattern];
				need_to_paste = true;
			}
		}
		
		if(need_to_paste)
		{
			Sint32 steps_to_paste = 0;//patterns[i].num_steps - (pat->num_steps - current_patternstep_for_channel(mused.current_sequencetrack + i));

			if(pat->num_steps - current_patternstep_for_channel(mused.current_sequencetrack + i) >= patterns[i].num_steps)
			{
				steps_to_paste = patterns[i].num_steps;
			}

			else
			{
				steps_to_paste = pat->num_steps - current_patternstep_for_channel(mused.current_sequencetrack + i);
			}

			for(int j = 0; j < steps_to_paste; j++)
			{
				MusStep* src_step = &patterns[i].step[j];
				MusStep* dst_step = &pat->step[current_patternstep_for_channel(mused.current_sequencetrack + i) + j];

				for(int k = 0; k < (1 + 1 + 1 + 8 * 2); k++)
				{
					if(mask[i][k])
					{
						switch(k)
						{
							case 0:
							{
								dst_step->note = src_step->note;
								break;
							}

							case 1:
							{
								dst_step->instrument = src_step->instrument;
								break;
							}

							case 2:
							{
								dst_step->volume = src_step->volume;
								break;
							}

							case 3:
							{
								if(format < PATTERN_TEXT_FORMAT_DEFLEMASK)
								{
									dst_step->command[0] = src_step->command[0];
								}

								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[0] &= 0x00ff;
									dst_step->command[0] |= (src_step->command[0] & 0xff00);
								}
								break;
							}

							case 4:
							{
								if(format < PATTERN_TEXT_FORMAT_DEFLEMASK)
								{
									dst_step->command[1] = src_step->command[1];
								}

								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[0] &= 0xff00;
									dst_step->command[0] |= (src_step->command[0] & 0x00ff);
								}
								break;
							}

							case 5:
							{
								if(format < PATTERN_TEXT_FORMAT_DEFLEMASK)
								{
									dst_step->command[2] = src_step->command[2];
								}

								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[1] &= 0x00ff;
									dst_step->command[1] |= (src_step->command[1] & 0xff00);
								}
								break;
							}

							case 6:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[1] &= 0xff00;
									dst_step->command[1] |= (src_step->command[1] & 0x00ff);
								}
								break;
							}

							case 7:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[2] &= 0x00ff;
									dst_step->command[2] |= (src_step->command[2] & 0xff00);
								}
								break;
							}

							case 8:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[2] &= 0xff00;
									dst_step->command[2] |= (src_step->command[2] & 0x00ff);
								}
								break;
							}

							case 9:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[3] &= 0x00ff;
									dst_step->command[3] |= (src_step->command[3] & 0xff00);
								}
								break;
							}

							case 10:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[3] &= 0xff00;
									dst_step->command[3] |= (src_step->command[3] & 0x00ff);
								}
								break;
							}

							case 11:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[4] &= 0x00ff;
									dst_step->command[4] |= (src_step->command[4] & 0xff00);
								}
								break;
							}

							case 12:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[4] &= 0xff00;
									dst_step->command[4] |= (src_step->command[4] & 0x00ff);
								}
								break;
							}

							case 13:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[5] &= 0x00ff;
									dst_step->command[5] |= (src_step->command[5] & 0xff00);
								}
								break;
							}

							case 14:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[5] &= 0xff00;
									dst_step->command[5] |= (src_step->command[5] & 0x00ff);
								}
								break;
							}

							case 15:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[6] &= 0x00ff;
									dst_step->command[6] |= (src_step->command[6] & 0xff00);
								}
								break;
							}

							case 16:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[6] &= 0xff00;
									dst_step->command[6] |= (src_step->command[6] & 0x00ff);
								}
								break;
							}

							case 17:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[7] &= 0x00ff;
									dst_step->command[7] |= (src_step->command[7] & 0xff00);
								}
								break;
							}

							case 18:
							{
								if(format == PATTERN_TEXT_FORMAT_FURNACE)
								{
									dst_step->command[7] &= 0xff00;
									dst_step->command[7] |= (src_step->command[7] & 0x00ff);
								}
								break;
							}

							default: break;
						}
					}
				}
			}
		}
	}

	end:;

	SDL_free(plain_text_string);

	if(plain_text_string_copy)
	{
		free(plain_text_string_copy);
	}

	if(lines)
	{
		free(lines);
	}

	if(patterns)
	{
		for(int i = 0; i < num_patterns; i++)
		{
			if(patterns[i].step)
			{
				free(patterns[i].step);
			}
		}

		free(patterns);
	}

	if(pattern_rows)
	{
		free(pattern_rows);
	}

	if(mask)
	{
		for(int i = 0; i < num_patterns; i++)
		{
			free(mask[i]);
		}

		free(mask);
	}
}