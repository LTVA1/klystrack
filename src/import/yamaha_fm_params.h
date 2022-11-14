

/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2022 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

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

#include "SDL.h"
#include <math.h>

#include "mml_string.h"

#include "../../klystron/src/snd/cyd.h"

#define OPN_AR_VALUES 32
#define OPN_DR_VALUES 32
#define OPN_SR_VALUES 32
#define OPN_RR_VALUES 16

enum
{
	YAMAHA_CHIP_OPL,
	YAMAHA_CHIP_OPM,
	YAMAHA_CHIP_OPN,
};

enum
{
	YAMAHA_PARAM_LFRQ, //LFO freq (OPM only)
	YAMAHA_PARAM_AMD, //amplitude modulation depth (tremolo depth fine?) (OPM only)
	YAMAHA_PARAM_PMD, //phase modulation depth (vibrato depth fine?) (OPM only)
	YAMAHA_PARAM_WF, //LFO waveform (OPM only)
	YAMAHA_PARAM_NFRQ, //noise frequency (OPM only)
	
	YAMAHA_PARAM_PAN, //panning
	YAMAHA_PARAM_FL, //feedback level
	YAMAHA_PARAM_CON, //algorithm
	YAMAHA_PARAM_AMS, //amplitude modulation sensitivity (tremolo depth coarse?)
	YAMAHA_PARAM_PMS, //phase modulation sensitivity (vibrato depth coarse?)
	YAMAHA_PARAM_SLOT_MASK, //like a key on thing bit flags, for VOPM (OPM) file format might be bit-shifted to the right for 3 bits
	YAMAHA_PARAM_NE, //noise enable (OPM only)
	
	YAMAHA_PARAM_AR, //attack rate
	YAMAHA_PARAM_DR, //decay rate
	YAMAHA_PARAM_SR, //sustain rate
	YAMAHA_PARAM_SL, //sustain level
	YAMAHA_PARAM_RR, //release rate
	YAMAHA_PARAM_TL, //total level
	YAMAHA_PARAM_KS, //key scale (envelope key scaling level?) (OPM/OPN only)
	YAMAHA_PARAM_MUL, //freq mult
	YAMAHA_PARAM_DT1, //detune
	YAMAHA_PARAM_DT2, //coarse detune (OPM only)
	YAMAHA_PARAM_AMS_EN, //enable tremolo (doesn't affect vibrato)
	YAMAHA_PARAM_SSG_EG, //SSG-EG (OPN2/OPNA/OPNB/etc. only)
	
	YAMAHA_EGT, //OPL only, discards SL setting (it is set to 0) (but does it override previous register value?)
};

Sint8 reinterpret_yamaha_params(Sint8 param, Uint8 type, Uint16 song_rate, /* <- for LFO speed */ CydEngine* cyd, /* <- for envelope length */ Uint8 chip_type, /* OPNx, OPLx or OPM */ Uint8 dialect);