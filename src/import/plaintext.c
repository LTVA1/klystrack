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

#include "../mused.h"
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
	PATTERN_TEXT_FORMAT_OPENMPT_IT,
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
	"ModPlug Tracker  IT",
	"ModPlug Tracker MPT",

	"333",
	"org.tildearrow.furnace",

	"333",
}; 

void paste_from_clipboard()
{
	char* plain_text_string = SDL_GetClipboardText();
	char* plain_text_string_copy = NULL;

	Uint8 fur_version = 0;
	Uint8 fur_startx = 0; //0 = note, 1 = inst, 2 = vol, 3 = 2 most significant digits of effect 1, 4 = 2 least significant digits of effect 1, ...

	Uint16 current_line_index = 0;

	char** lines = NULL;
	char** pattern_rows = NULL; //size = num_patterns
	Uint16 num_lines = 0;
	char values[1 + 1 + 1 + 8][5]; //note, inst, volume and up to 8 effects
	memset(values, 0, (1 + 1 + 1 + 8) * (5) * sizeof(char));

	MusPattern* patterns = NULL;
	Uint8 num_patterns = 0;
	Uint8* pattern_lengths = NULL;

	if(strcmp(plain_text_string, "") == 0)
	{
		msgbox(domain, mused.slider_bevel, &mused.largefont, "Clipboard is empty!", MB_OK);
		
		goto end;
	}

	Uint8 format = 0;

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

	const char delimiters_lines[] = "\n\r";
	const char delimiters[] = "|";
	char* current_line;

	if(format < PATTERN_TEXT_FORMAT_DEFLEMASK)
	{
		Uint16 passes = 0;

		while(current_line != NULL)
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

		patterns = (MusPattern**)calloc(1, sizeof(MusPattern) * num_patterns);
		memset(patterns, 0, sizeof(MusPattern) * num_patterns);

		for(int i = 0; i < num_patterns; i++)
		{
			patterns[i].step = calloc(1, sizeof(MusStep) * num_lines);
		}

		strcpy(lines[1], lines_1_copy);
		free(lines_1_copy);

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
					//debug("\"%s\"", pattern_rows[current_line_index]);
					current_line_index++;

					memset(values, 0, (1 + 1 + 1 + 8) * (5) * sizeof(char));
					//values[0] = { 0 }; //note
					memcpy(values[0], pattern_rows[current_line_index - 1], 3 * sizeof(char));
					//values[1] = { 0 }; //instrument
					memcpy(values[1], &pattern_rows[current_line_index - 1][3], 2 * sizeof(char));
					//values[2] = { 0 }; //volume
					memcpy(values[2], &pattern_rows[current_line_index - 1][3 + 2], 3 * sizeof(char));
					//values[3] = { 0 }; //effect
					memcpy(values[3], &pattern_rows[current_line_index - 1][3 + 2 + 3], 3 * sizeof(char));

					debug("note \"%s\", inst \"%s\", vol \"%s\", eff \"%s\"", values[0], values[1], values[2], values[3]);
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

	if(pattern_lengths)
	{
		free(pattern_lengths);
	}

	if(pattern_rows)
	{
		free(pattern_rows);
	}
}