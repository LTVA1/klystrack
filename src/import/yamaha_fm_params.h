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

#pragma once

#include "SDL.h"
#include <math.h>

#define OPN_AR_VALUES 32
#define OPN_DR_VALUES 32
#define OPN_SR_VALUES 32
#define OPN_RR_VALUES 16

enum
{
	YAMAHA_PARAM_LFRQ, //LFO freq (OPM only)
	YAMAHA_PARAM_AMD, //amplitude modulation depth (tremolo depth?) (OPM only)
	YAMAHA_PARAM_PMD, //phase modulation depth (vibrato depth?) (OPM only)
	YAMAHA_PARAM_WF, //LFO waveform (OPM only)
	YAMAHA_PARAM_NFRQ, //noise frequency (OPM only)
	
	YAMAHA_PARAM_PAN, //panning
	YAMAHA_PARAM_FL, //feedback level
	YAMAHA_PARAM_CON, //algorithm
	YAMAHA_PARAM_AMS, //amplitude modulation sensitivity (tremolo speed?) (OPM only)
	YAMAHA_PARAM_PMS, //phase modulation sensitivity (vibrato speed?) (OPM only)
	YAMAHA_PARAM_SLOT_MASK, //like a key on thing bit flags, for VOPM (OPM) file format might be bit-shifted to the right for 3 bits
	YAMAHA_NE, //noise enable (OPM only)
	
	YAMAHA_AR, //attack rate
	YAMAHA_DR, //decay rate
	YAMAHA_SR, //sustain rate
	YAMAHA_SL, //sustain level
	YAMAHA_RR, //release rate
	YAMAHA_TL, //total level
	YAMAHA_KS, //key scale (envelope key scaling level?) (OPM only)
	YAMAHA_MUL, //freq mult
	YAMAHA_DT1, //detune
	YAMAHA_DT2, //coarse detune (OPM only)
	YAMAHA_AMS_EN, //enable tremolo (and vibrato?)
};

const static float OPN_attack_rate_lengths[OPN_AR_VALUES] = 
{
	INFINITY,
	7088.0,
	4706.0,
	3599.0,
	2410.0,
	1800.0,
	1204.0,
	900.0,
	602.0,
	450.0,
	225.0,
	151.0,
	113.0,
	76.0,
	57.0,
	38.0,
	28.0,
	19.0,
	14.0,
	10.0,
	8.0,
	5.0,
	4.0,
	3.0,
	2.0,
	1.0,
	1.0,
	0.9,
	0.5,
	0.0,
	0.0,
	
	/* here and later data in ms */
	/* measured from wav exported from BambooTracker */
};

Uint8 reinterpret_yamaha_params(Uint8 param, Uint8 type);