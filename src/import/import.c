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

#include "mused.h"
#include "event.h"
#include "import.h"
#include "mod.h"
#include "ahx.h"
#include "xm.h"
#include "org.h"
#include "fzt.h"
#include "famitracker.h"
#include "hubbard.h"
#include "gui/toolutil.h"
#include "gui/msgbox.h"
#include "diskop.h"
#include "SDL_endian.h"
#include "action.h"
#include "optimize.h"


extern Mused mused;
extern GfxDomain *domain;

void import_module(void *type, void* unused1, void* unused2)
{
	int r;
	if (mused.modified) r = confirm_ync(domain, mused.slider_bevel, &mused.largefont, "Save song?");
	else r = -1;
				
	if (r == 0) return;
	if (r == 1) 
	{ 
		int r;
				
		open_data(MAKEPTR(OD_T_SONG), MAKEPTR(OD_A_SAVE), &r);
				
		if (!r) return;
	}
	
	static const char *mod_name[] = {"a Protracker", "an OctaMED", "a Scream Tracker III", "an AHX", "a FastTracker II", "an Impulse Tracker", "an OpenMPT", "a Cave Story", "a Rob Hubbard", "a FamiTracker", "a 0CC-FamiTracker (j0CC-FamiTracker)", "a Dn-FamiTracker", "an E-FamiTracker", "a DefleMask", "a Furnace", "an Adlib Tracker II", "a Raster Music Tracker", "a Flizzer Tracker"};
	static const char *mod_ext[] = {"mod", "med", "s3m", "ahx", "xm", "it", "mptm", "org", "sid", "ftm", "0cc", "dnm", "eft", "dmf", "fur", "a2m", "rmt", "fzt"};
	
	char buffer[100];
	snprintf(buffer, sizeof(buffer), "Import %s song", mod_name[CASTPTR(int, type)]);

	FILE * f = open_dialog("rb", buffer, mod_ext[CASTPTR(int, type)], domain, mused.slider_bevel, &mused.largefont, &mused.smallfont, NULL);
	
	if (!f) return;
	
	stop(NULL, NULL, NULL);
	new_song();
	
	switch (CASTPTR(int, type))
	{
		case IMPORT_MOD: r = import_mod(f); break;
		case IMPORT_AHX: r = import_ahx(f); break;
		case IMPORT_XM: r = import_xm(f); break;
		case IMPORT_ORG: r = import_org(f); break;
		case IMPORT_HUBBARD: r = import_hubbard(f); break;
		case IMPORT_FZT: r = import_fzt(f); break;

		case IMPORT_FTM: r = import_famitracker(f, IMPORT_FTM); break;
		case IMPORT_0CC: r = import_famitracker(f, IMPORT_0CC); break;
		case IMPORT_DNM: r = import_famitracker(f, IMPORT_DNM); break;
		case IMPORT_EFT: r = import_famitracker(f, IMPORT_EFT); break;
		//others will be there I promise
	}
	
	if (CASTPTR(int, type) != IMPORT_HUBBARD)
	{
		if (!r) 
		{
			snprintf(buffer, sizeof(buffer), "Not %s song", mod_name[CASTPTR(int, type)]);
			msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		}
		
		else
		{
			//kill_empty_patterns(&mused.song, NULL); //wasn't there

			if(CASTPTR(int, type) != IMPORT_FTM && CASTPTR(int, type) != IMPORT_0CC && CASTPTR(int, type) != IMPORT_DNM && CASTPTR(int, type) != IMPORT_EFT)
			{
				optimize_duplicate_patterns(&mused.song, true);
			}
		}
	}
	
	else
	{
		optimize_song(&mused.song);
	}
	
	mused.song.num_patterns = NUM_PATTERNS;
	
	mused.time_signature = mused.song.time_signature;
	
	fclose(f);
	
	set_channels(mused.song.num_channels);
}

