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

#include "wavetable_string.h"
#include "stdlib.h"

extern Mused mused;
extern GfxDomain *domain;

void import_wavetable_string(MusInstrument* inst)
{
	if (confirm(domain, mused.slider_bevel, &mused.largefont, "  Import wavetable string to current instrument?\nIt will overwrite current instrument local samples!"))
	{
		char* wave_string = SDL_GetClipboardText();
		
		debug("got wavetable string: \"%s\"", wave_string);
		
		if(strcmp(wave_string, "") == 0)
		{
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Clipboard is empty!", MB_OK);
			SDL_free(wave_string);
			
			goto end;
		}
		
		char* wave_string_copy = (char*)calloc(1, strlen(wave_string) + 1);
		strcpy(wave_string_copy, wave_string);
		
		const char delimiters_lines[] = "\n\r;";
		
		Uint16 passes = 0;
		Uint16 wavetables = 0;
		
		char* current_line;
		
		while(current_line != NULL)
		{
			current_line = strtok(passes > 0 ? NULL : wave_string, delimiters_lines);
			passes++;
			
			if(current_line != NULL)
			{
				if(strcmp(current_line, "") != 0)
				{
					wavetables++;
				}
			}
		}
		
		debug("wavetables in string: %d", wavetables);
		
		SDL_free(wave_string);
	}
	
	end:;
}