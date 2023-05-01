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

#pragma once

#include "snd/music.h"
#include "../mused.h"
#include "../klystron/src/gui/msgbox.h"
#include "../klystron/src/gui/bevdefs.h"
#include "../klystron/src/gui/view.h"
#include "../klystron/src/gui/toolutil.h"
#include "../klystron/src/gui/bevel.h"
#include "../klystron/src/gui/dialog.h"
#include "../klystron/src/gui/mouse.h"

#include <string.h>

#include "yamaha_fm_params.h"

enum
{
	BTN_PMD = 1,
	BTN_FMP = 2,
	BTN_FMP7 = 4,
	BTN_VOPM = 8,
	BTN_NRTDRV = 16,
	BTN_MXDRV = 32,
	BTN_MMLDRV = 64,
	BTN_MUCOM88 = 128,
	
	MML_BUTTON_TYPES = 8,
};

void import_mml_fm_instrument_string(MusInstrument* inst);