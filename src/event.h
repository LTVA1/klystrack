#pragma once

#ifndef EVENT_H
#define EVENT_H

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

#include "SDL.h"
#include "gui/slider.h"
#include "optimize.h"

#include "../klystron/src/gui/toolutil.h"

#include "gui/msgbox.h"

#include "import/mod.h"
#include "import/ahx.h"
#include "import/xm.h"
#include "import/org.h"
#include "import/hubbard.h"
#include "import/fzt.h"
#include "import/famitracker.h"
#include "import/fur.h"

#include <dirent.h>
#include <errno.h>

#include "gui/mouse.h"
#include "gui/bevel.h"
#include "gui/bevdefs.h"
#include "gui/menu.h"
#include "gui/msgbox.h"

#include <time.h>

void edit_instrument_event(SDL_Event *e);
void sequence_event(SDL_Event *e);
void pattern_event(SDL_Event *e);
void edit_program_event(SDL_Event *e);
void edit_text(SDL_Event *e);
int generic_edit_text(SDL_Event *e, char *edit_buffer, size_t edit_buffer_size, int *editpos);
void fx_event(SDL_Event *e);
void fx_add_param(int d);
void wave_event(SDL_Event *e);
void local_sample_event(SDL_Event *e);
void wave_add_param(int d);
void local_sample_add_param(int d);
void songinfo_event(SDL_Event *e);
void songinfo_add_param(int d);
void instrument_add_param(int a);
void del_sequence(int first, int last, int track);
void add_sequence(int channel, int position, int pattern, int offset);
void set_room_size(int fx, int size, int vol, int dec);
void update_position_sliders();
void update_horiz_sliders();
void note_event(SDL_Event *e);

void dropfile_event(SDL_Event *e);

int seqsort(const void *_a, const void *_b); //wasn't there

void edit_fourop_event(SDL_Event *e);
void four_op_add_param(int a);
void env_editor_add_param(int a);

void edit_env_editor_event(SDL_Event *e);
void fourop_env_editor_add_param(int a);
void edit_4op_env_editor_event(SDL_Event *e);

void do_autosave(Uint32* timeout);

enum
{
	PED_NOTE,
	PED_INSTRUMENT1,
	PED_INSTRUMENT2,
	PED_VOLUME1,
	PED_VOLUME2,
	PED_LEGATO,
	PED_SLIDE,
	PED_VIB,
	PED_TREM, //wasn't there
	PED_COMMAND1,
	PED_COMMAND2,
	PED_COMMAND3,
	PED_COMMAND4,
	
	PED_COMMAND21,
	PED_COMMAND22,
	PED_COMMAND23,
	PED_COMMAND24,
	PED_COMMAND31,
	PED_COMMAND32,
	PED_COMMAND33,
	PED_COMMAND34,
	PED_COMMAND41,
	PED_COMMAND42,
	PED_COMMAND43,
	PED_COMMAND44,
	PED_COMMAND51,
	PED_COMMAND52,
	PED_COMMAND53,
	PED_COMMAND54,
	PED_COMMAND61,
	PED_COMMAND62,
	PED_COMMAND63,
	PED_COMMAND64,
	PED_COMMAND71,
	PED_COMMAND72,
	PED_COMMAND73,
	PED_COMMAND74,
	PED_COMMAND81,
	PED_COMMAND82,
	PED_COMMAND83,
	PED_COMMAND84,
	
	PED_PARAMS,
};

#define PED_CTRL PED_LEGATO

enum
{
	ENV_ENABLE_VOLUME_ENVELOPE,
	ENV_VOLUME_ENVELOPE_FADEOUT,
	ENV_VOLUME_ENVELOPE_SCALE,
	ENV_ENABLE_VOLUME_ENVELOPE_SUSTAIN,
	ENV_VOLUME_ENVELOPE_SUSTAIN_POINT,
	ENV_ENABLE_VOLUME_ENVELOPE_LOOP,
	ENV_VOLUME_ENVELOPE_LOOP_BEGIN,
	ENV_VOLUME_ENVELOPE_LOOP_END,
	
	ENV_ENABLE_PANNING_ENVELOPE,
	ENV_PANNING_ENVELOPE_FADEOUT,
	ENV_PANNING_ENVELOPE_SCALE,
	ENV_ENABLE_PANNING_ENVELOPE_SUSTAIN,
	ENV_PANNING_ENVELOPE_SUSTAIN_POINT,
	ENV_ENABLE_PANNING_ENVELOPE_LOOP,
	ENV_PANNING_ENVELOPE_LOOP_BEGIN,
	ENV_PANNING_ENVELOPE_LOOP_END,
	/*----------*/
	ENV_PARAMS
};

enum
{
	FOUROP_ENV_ENABLE_VOLUME_ENVELOPE,
	FOUROP_ENV_VOLUME_ENVELOPE_FADEOUT,
	FOUROP_ENV_VOLUME_ENVELOPE_SCALE,
	FOUROP_ENV_ENABLE_VOLUME_ENVELOPE_SUSTAIN,
	FOUROP_ENV_VOLUME_ENVELOPE_SUSTAIN_POINT,
	FOUROP_ENV_ENABLE_VOLUME_ENVELOPE_LOOP,
	FOUROP_ENV_VOLUME_ENVELOPE_LOOP_BEGIN,
	FOUROP_ENV_VOLUME_ENVELOPE_LOOP_END,
	/*----------*/
	FOUROP_ENV_PARAMS
};

enum
{
	P_INSTRUMENT,
	P_NAME,
	P_BASENOTE,
	P_FINETUNE,
	P_LOCKNOTE,
	P_DRUM,
	P_KEYSYNC,
	P_INVVIB,
	
	P_INVTREM,
	
	P_SETPW,
	P_SETCUTOFF,
	P_SLIDESPEED,
	P_PULSE,
	P_PW,
	P_SAW,
	P_TRIANGLE,
	P_NOISE,
	P_METAL,
	P_LFSR,
	P_LFSRTYPE,
	P_1_4TH,
	P_SINE,
	P_SINE_PHASE_SHIFT,
	
	P_FIX_NOISE_PITCH,
	P_FIXED_NOISE_BASE_NOTE,
	P_1_BIT_NOISE,
	
	P_WAVE,
	P_WAVE_ENTRY,
	P_WAVE_OVERRIDE_ENV,
	P_WAVE_LOCK_NOTE,
	
	P_OSCMIXMODE, //wasn't there
	
	P_VOLUME,
	P_RELVOL,
	P_ATTACK,
	P_DECAY,
	P_SUSTAIN,
	P_RELEASE,
	
	P_EXP_VOL, //wasn't there
	P_EXP_ATTACK, //wasn't there
	P_EXP_DECAY, //wasn't there
	P_EXP_RELEASE, //wasn't there
	
	P_VOL_KSL,
	P_VOL_KSL_LEVEL,
	P_ENV_KSL,
	P_ENV_KSL_LEVEL,
	
	P_BUZZ,
	P_BUZZ_SEMI,
	P_BUZZ_SHAPE,
	P_BUZZ_ENABLE_AY8930, //wasn't there
	P_BUZZ_FINE,
	
	P_SYNC,
	P_SYNCSRC,
	P_RINGMOD,
	P_RINGMODSRC,
	P_FILTER,
	P_FLTTYPE,
	P_CUTOFF,
	P_RESONANCE,
	P_SLOPE, //wasn't there
	P_NUM_OF_MACROS,
	P_PROGPERIOD,
	P_FX,
	P_FXBUS,
	P_VIBSPEED,
	P_VIBDEPTH,
	P_VIBSHAPE,
	P_VIBDELAY,
	P_PWMSPEED,
	P_PWMDEPTH,
	P_PWMSHAPE,
	P_PWMDELAY, //wasn't there
	
	P_TREMSPEED,
	P_TREMDEPTH,
	P_TREMSHAPE,
	P_TREMDELAY,
	
	P_NORESTART,
	P_MULTIOSC,
	
	P_SAVE_LFO_SETTINGS,
	
	P_FM_ENABLE,
	P_FM_MODULATION,
	
	P_FM_VOL_KSL_ENABLE,
	P_FM_VOL_KSL_LEVEL,
	P_FM_ENV_KSL_ENABLE,
	P_FM_ENV_KSL_LEVEL,
	
	P_FM_FEEDBACK,
	P_FM_HARMONIC_CARRIER,
	P_FM_HARMONIC_MODULATOR,
	
	P_FM_BASENOTE, //weren't there
	P_FM_FINETUNE,
	
	P_FM_FREQ_LUT, //wasn't there
	
	P_FM_ATTACK,
	P_FM_DECAY,
	P_FM_SUSTAIN,
	P_FM_RELEASE,
	
	P_FM_EXP_VOL, //wasn't there
	P_FM_EXP_ATTACK, //wasn't there
	P_FM_EXP_DECAY, //wasn't there
	P_FM_EXP_RELEASE, //wasn't there
	
	P_FM_ENV_START,
	P_FM_WAVE,
	P_FM_WAVE_ENTRY,
	
	P_FM_VIBSPEED,
	P_FM_VIBDEPTH,
	P_FM_VIBSHAPE,
	P_FM_VIBDELAY,
	P_FM_TREMSPEED,
	P_FM_TREMDEPTH,
	P_FM_TREMSHAPE,
	P_FM_TREMDELAY,
	
	P_FM_ADDITIVE, //wasn't there
	P_FM_SAVE_LFO_SETTINGS,
	P_FM_ENABLE_4OP,
	/*----------*/
	P_PARAMS
};

enum
{
	FOUROP_3CH_EXP_MODE,
	FOUROP_ALG,
	
	FOUROP_MASTER_VOL,
	FOUROP_USE_MAIN_INST_PROG,
	FOUROP_BYPASS_MAIN_INST_FILTER,
	
	FOUROP_BASENOTE,
	FOUROP_FINETUNE,
	FOUROP_LOCKNOTE,
	
	FOUROP_HARMONIC_CARRIER,
	FOUROP_HARMONIC_MODULATOR,
	
	FOUROP_DETUNE,
	FOUROP_COARSE_DETUNE,
	
	FOUROP_FEEDBACK,
	FOUROP_ENABLE_SSG_EG,
	FOUROP_SSG_EG_TYPE,
	
	FOUROP_DRUM,
	FOUROP_KEYSYNC,
	FOUROP_INVVIB,
	
	FOUROP_INVTREM,
	
	FOUROP_SETPW,
	FOUROP_SETCUTOFF,
	FOUROP_SLIDESPEED,
	FOUROP_PULSE,
	FOUROP_PW,
	FOUROP_SAW,
	FOUROP_TRIANGLE,
	FOUROP_NOISE,
	FOUROP_METAL,
	FOUROP_1_4TH,
	FOUROP_SINE,
	FOUROP_SINE_PHASE_SHIFT,
	
	FOUROP_FIX_NOISE_PITCH,
	FOUROP_FIXED_NOISE_BASE_NOTE,
	FOUROP_1_BIT_NOISE,
	
	FOUROP_WAVE,
	FOUROP_WAVE_ENTRY,
	FOUROP_WAVE_OVERRIDE_ENV,
	FOUROP_WAVE_LOCK_NOTE,
	
	FOUROP_OSCMIXMODE, //wasn't there
	
	FOUROP_VOLUME,
	FOUROP_RELVOL,
	FOUROP_ATTACK,
	FOUROP_DECAY,
	FOUROP_SUSTAIN,
	
	FOUROP_SUSTAIN_RATE,
	
	FOUROP_RELEASE,
	
	FOUROP_EXP_VOL, //wasn't there
	FOUROP_EXP_ATTACK, //wasn't there
	FOUROP_EXP_DECAY, //wasn't there
	FOUROP_EXP_RELEASE, //wasn't there
	
	FOUROP_VOL_KSL,
	FOUROP_VOL_KSL_LEVEL,
	FOUROP_ENV_KSL,
	FOUROP_ENV_KSL_LEVEL,

	FOUROP_SYNC,
	FOUROP_SYNCSRC,
	FOUROP_RINGMOD,
	FOUROP_RINGMODSRC,
	FOUROP_FILTER,
	FOUROP_FLTTYPE,
	FOUROP_CUTOFF,
	FOUROP_RESONANCE,
	FOUROP_SLOPE, //wasn't there
	FOUROP_NUM_OF_MACROS,
	
	FOUROP_PROGPERIOD,
	
	FOUROP_VIBSPEED,
	FOUROP_VIBDEPTH,
	FOUROP_VIBSHAPE,
	FOUROP_VIBDELAY,
	FOUROP_PWMSPEED,
	FOUROP_PWMDEPTH,
	FOUROP_PWMSHAPE,
	FOUROP_PWMDELAY, //wasn't there
	
	FOUROP_TREMSPEED,
	FOUROP_TREMDEPTH,
	FOUROP_TREMSHAPE,
	FOUROP_TREMDELAY,
	
	FOUROP_NORESTART,
	
	FOUROP_SAVE_LFO_SETTINGS,
	
	FOUROP_TRIG_DELAY,
	FOUROP_ENV_OFFSET,
	
	FOUROP_ENABLE_CSM_TIMER,
	FOUROP_CSM_TIMER_NOTE,
	FOUROP_CSM_TIMER_FINETUNE,
	FOUROP_LINK_CSM_TIMER_NOTE,
	/*----------*/
	FOUROP_PARAMS
};

enum
{
	W_WAVE,
	W_NAME,
	W_RATE,
	W_BASE,
	W_BASEFINE,
	W_INTERPOLATE,
	W_INTERPOLATION_TYPE, //wasn't there
	W_LOOP,
	W_LOOPBEGIN,
	W_LOOPPINGPONG,
	W_LOOPEND,
	W_NUMOSCS,
	W_OSCTYPE,
	W_OSCMUL,
	W_OSCSHIFT,
	W_OSCEXP,
	W_OSCVOL, //wasn't there
	W_OSCABS,
	W_OSCNEG,
	W_WAVELENGTH,
	W_RNDGEN,
	W_GENERATE,
	W_RND,
	W_TOOLBOX,
	/* ----- */
	W_N_PARAMS
};

enum
{
	LS_ENABLE,
	LS_LOCAL_SAMPLE,
	
	LS_WAVE,
	LS_NAME,
	LS_RATE,
	LS_BASE,
	LS_BASEFINE,
	LS_INTERPOLATE,
	LS_INTERPOLATION_TYPE, //wasn't there
	LS_LOOP,
	LS_LOOPBEGIN,
	LS_LOOPPINGPONG,
	LS_LOOPEND,
	LS_BINDTONOTES,
	/* ----- */
	LS_N_PARAMS
};

enum
{
	R_MULTIPLEX,
	R_MULTIPLEX_PERIOD,
	R_PITCH_INACCURACY,
	R_FX_BUS,
	R_FX_BUS_NAME,
	R_CRUSH,
	R_CRUSHBITS,
	R_CRUSHDOWNSAMPLE,
	R_CRUSHDITHER,
	R_CRUSHGAIN,
	R_CHORUS,
	R_MINDELAY,
	R_MAXDELAY,
	R_SEPARATION,
	R_RATE,
	R_ENABLE,
	R_ROOMSIZE,
	R_ROOMVOL,
	R_ROOMDECAY,
	R_SNAPTICKS,
	R_TAPENABLE,
	R_TAP,
	R_DELAY,
	R_GAIN,
	
	R_NUM_TAPS, //wasn't there
	
	R_PANNING,
	/* ---- */
	R_N_PARAMS
};

enum
{
	SI_LENGTH,
	SI_LOOP,
	SI_STEP,
	SI_SPEED1,
	SI_SPEED2,
	SI_ENABLE_GROOVE,
	SI_RATE,
	SI_TIME1,
	SI_TIME2,
	SI_OCTAVE,
	SI_CHANNELS,
	SI_MASTERVOL,
	/*--------*/
	SI_N_PARAMS
};

#endif