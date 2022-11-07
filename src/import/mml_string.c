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

#include "mml_string.h"

extern Mused mused;
extern GfxDomain *domain;

void import_mml_fm_instrument_string(MusInstrument* inst)
{
	if (confirm(domain, mused.slider_bevel, &mused.largefont, "Import MML string to current instrument?\n  It will overwrite current instrument!"))
	{
		char* mml_string = SDL_GetClipboardText();
		debug("got mml string: \"%s\"", mml_string);
		
		if(strcmp(mml_string, "") == 0)
		{
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Clipboard is empty!", MB_OK);
			goto end;
		}
		
		//do stuff
		
		snapshot(S_T_INSTRUMENT);
		
		SDL_free(mml_string);
	}
	
	end:;
	
	mused.import_mml_string = false;
}