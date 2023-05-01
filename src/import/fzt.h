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

#ifndef FZT_H
#define FZT_H

#include <stdio.h>
#include "../edit.h"
#include "../mused.h"
#include "../event.h"
#include "SDL_endian.h"
#include "../../klystron/src/snd/freqs.h"
#include "../view.h"

#define FZT_SONG_SIG "FZT!SONG"
#define FZT_SONG_MAX_CHANNELS 4
#define FZT_MAX_SEQUENCE_LENGTH 256
#define FZT_SONG_NAME_LEN 16
#define FZT_MUS_INST_NAME_LEN 13
#define FZT_WAVE_NAME_LEN (32 - 8 + 1)
#define FZT_INST_PROG_LEN 16
#define FZT_VERSION 2
#define FZT_MIDDLE_C (12 * 4)

#define FZT_MUS_NOTE_NONE 127
#define FZT_MUS_NOTE_RELEASE 126
#define FZT_MUS_NOTE_CUT 125

#define FZT_MUS_NOTE_INSTRUMENT_NONE 31
#define FZT_MUS_NOTE_VOLUME_NONE 31

#define FZT_MAX_ADSR (0xff << 17)
#define FZT_BASE_FREQ 22050
#define FZT_SAMPLE_RATE 44100
#define fzt_envspd(slope) ((slope) != 0 ? (((Uint64)FZT_MAX_ADSR / ((slope) * (slope)*256 / 8)) * FZT_BASE_FREQ / FZT_SAMPLE_RATE) : ((uint64_t)FZT_MAX_ADSR * FZT_BASE_FREQ / FZT_SAMPLE_RATE))

#define fzt_envelope_length(slope) ((((double)FZT_MAX_ADSR / (double)fzt_envspd(slope)) / (double)FZT_SAMPLE_RATE) * 1000.0) /* so we have time in milliseconds */

enum
{
	FZT_FIL_OUTPUT_LOWPASS = 1,
	FZT_FIL_OUTPUT_HIGHPASS = 2,
	FZT_FIL_OUTPUT_BANDPASS = 3,
	FZT_FIL_OUTPUT_LOW_HIGH = 4,
	FZT_FIL_OUTPUT_HIGH_BAND = 5,
	FZT_FIL_OUTPUT_LOW_BAND = 6,
	FZT_FIL_OUTPUT_LOW_HIGH_BAND = 7,
	/* ============ */
	FZT_FIL_MODES = 8,
};

enum
{
	FZT_SE_WAVEFORM_NONE = 0,
	FZT_SE_WAVEFORM_NOISE = 1,
	FZT_SE_WAVEFORM_PULSE = 2,
	FZT_SE_WAVEFORM_TRIANGLE = 4,
	FZT_SE_WAVEFORM_SAW = 8,
	FZT_SE_WAVEFORM_NOISE_METAL = 16,
	FZT_SE_WAVEFORM_SINE = 32,
};

enum
{
	FZT_SE_ENABLE_FILTER = 1,
	FZT_SE_ENABLE_GATE = 2,
	FZT_SE_ENABLE_RING_MOD = 4,
	FZT_SE_ENABLE_HARD_SYNC = 8,
	FZT_SE_ENABLE_KEYDOWN_SYNC = 16, // sync oscillators on keydown
	FZT_SE_ENABLE_SAMPLE = 32,
	FZT_SE_SAMPLE_OVERRIDE_ENVELOPE = 64,
};

enum
{
	FZT_TE_ENABLE_VIBRATO = 1,
	FZT_TE_ENABLE_PWM = 2,
	FZT_TE_PROG_NO_RESTART = 4,
	FZT_TE_SET_CUTOFF = 8,
	FZT_TE_SET_PW = 16,
	FZT_TE_RETRIGGER_ON_SLIDE = 32, // call trigger instrument function even if slide command is there
	FZT_TE_SAMPLE_LOCK_TO_BASE_NOTE = 64,
};

enum
{
	FZT_SE_SAMPLE_LOOP = 1,
};

enum
{
	FZT_TE_EFFECT_ARPEGGIO = 0x0000,
	FZT_TE_EFFECT_PORTAMENTO_UP = 0x0100,
	FZT_TE_EFFECT_PORTAMENTO_DOWN = 0x0200,
	FZT_TE_EFFECT_SLIDE = 0x0300,
	FZT_TE_EFFECT_VIBRATO = 0x0400,
	FZT_TE_EFFECT_PWM = 0x0500,
	FZT_TE_EFFECT_SET_PW = 0x0600,
	FZT_TE_EFFECT_PW_DOWN = 0x0700,
	FZT_TE_EFFECT_PW_UP = 0x0800,
	FZT_TE_EFFECT_SET_CUTOFF = 0x0900,
	FZT_TE_EFFECT_VOLUME_FADE = 0x0a00,
	FZT_TE_EFFECT_SET_WAVEFORM = 0x0b00,
	FZT_TE_EFFECT_SET_VOLUME = 0x0c00,
	FZT_TE_EFFECT_SKIP_PATTERN = 0x0d00,

	FZT_TE_EFFECT_EXT = 0x0e00,
	FZT_TE_EFFECT_EXT_TOGGLE_FILTER = 0x0e00,
	FZT_TE_EFFECT_EXT_PORTA_UP = 0x0e10,
	FZT_TE_EFFECT_EXT_PORTA_DN = 0x0e20,
	FZT_TE_EFFECT_EXT_FILTER_MODE = 0x0e30,
	FZT_TE_EFFECT_EXT_PATTERN_LOOP = 0x0e60, // e60 = start, e61-e6f = end and indication how many loops you want
	FZT_TE_EFFECT_EXT_RETRIGGER = 0x0e90,
	FZT_TE_EFFECT_EXT_FINE_VOLUME_DOWN = 0x0ea0,
	FZT_TE_EFFECT_EXT_FINE_VOLUME_UP = 0x0eb0,
	FZT_TE_EFFECT_EXT_NOTE_CUT = 0x0ec0,
	FZT_TE_EFFECT_EXT_NOTE_DELAY = 0x0ed0,
	FZT_TE_EFFECT_EXT_PHASE_RESET = 0x0ef0,

	FZT_TE_EFFECT_SET_SPEED_PROG_PERIOD = 0x0f00,
	FZT_TE_EFFECT_CUTOFF_UP = 0x1000, // Gxx
	FZT_TE_EFFECT_CUTOFF_DOWN = 0x1100, // Hxx
	FZT_TE_EFFECT_SET_RESONANCE = 0x1200, // Ixx
	FZT_TE_EFFECT_RESONANCE_UP = 0x1300, // Jxx
	FZT_TE_EFFECT_RESONANCE_DOWN = 0x1400, // Kxx

	FZT_TE_EFFECT_SET_ATTACK = 0x1500, // Lxx
	FZT_TE_EFFECT_SET_DECAY = 0x1600, // Mxx
	FZT_TE_EFFECT_SET_SUSTAIN = 0x1700, // Nxx
	FZT_TE_EFFECT_SET_RELEASE = 0x1800, // Oxx
	FZT_TE_EFFECT_PROGRAM_RESTART = 0x1900, // Pxx
	
	FZT_TE_EFFECT_SET_DPCM_SAMPLE = 0x1a00, //Qxx

	FZT_TE_EFFECT_SET_RING_MOD_SRC = 0x1b00, // Rxx
	FZT_TE_EFFECT_SET_HARD_SYNC_SRC = 0x1c00, // Sxx

	FZT_TE_EFFECT_PORTA_UP_SEMITONE = 0x1d00, // Txx
	FZT_TE_EFFECT_PORTA_DOWN_SEMITONE = 0x1e00, // Uxx
	/*
	FZT_TE_EFFECT_ = 0x1f00, //Vxx
	FZT_TE_EFFECT_ = 0x2000, //Wxx
	*/

	FZT_TE_EFFECT_LEGATO = 0x2100, // Xxx
	FZT_TE_EFFECT_ARPEGGIO_ABS = 0x2200, // Yxx
	FZT_TE_EFFECT_TRIGGER_RELEASE = 0x2300, // Zxx

	/* These effects work only in instrument program */
	FZT_TE_PROGRAM_LOOP_BEGIN = 0x7d00,
	FZT_TE_PROGRAM_LOOP_END = 0x7e00,
	FZT_TE_PROGRAM_JUMP = 0x7f00,
	FZT_TE_PROGRAM_NOP = 0x7ffe,
	FZT_TE_PROGRAM_END = 0x7fff,
};

typedef struct 
{
	char sig[sizeof(FZT_SONG_SIG) + 1];
	Uint8 version;
	char song_name[FZT_SONG_NAME_LEN + 1];
	Uint8 loop_start;
	Uint8 loop_end;
	Uint16 pattern_length;
	Uint8 speed;
	Uint8 rate;
	Uint16 num_sequence_steps;
	Uint8 num_patterns;
	Uint8 num_instruments;
} fzt_header;

typedef struct
{
	Uint8 pattern_indices[FZT_SONG_MAX_CHANNELS];
} fzt_sequence_step;

typedef struct
{
	fzt_sequence_step sequence_step[FZT_MAX_SEQUENCE_LENGTH];
} fzt_sequence;

typedef struct 
{
	Uint8 note; // MSB is used for instrument number MSB
	Uint8 inst_vol; // high nibble + MSB from note = instrument, low nibble = 4 volume LSBs
	Uint16 command; // MSB used as volume MSB
} fzt_pattern_step;

typedef struct 
{
	fzt_pattern_step* step;
} fzt_pattern;

typedef struct
{
	Uint8 a, d, s, r, volume;
} fzt_instrument_adsr;

typedef struct {
	Uint32 length, loop_start, loop_end, position;
	Uint8 delta_counter; //0-63
	Uint8 initial_delta_counter_position;
	Uint8 delta_counter_position_on_loop_start;
	char name[FZT_WAVE_NAME_LEN];
	Uint8* data;

	Uint8 flags;
	bool playing;
} fzt_dpcm_sample;

typedef struct 
{
	char name[FZT_MUS_INST_NAME_LEN + 1];

	Uint8 waveform;
	Uint16 flags;
	Uint16 sound_engine_flags;

	Uint8 slide_speed;

	fzt_instrument_adsr adsr;

	Uint8 ring_mod, hard_sync; // 0xff = self

	Uint8 pw; // store only one byte since we don't have the luxury of virtually unlimited memory!

	Uint16 program[FZT_INST_PROG_LEN]; // MSB is unite bit (indicates this and next command must be executed at once)
	Uint8 program_period;

	Uint8 vibrato_speed, vibrato_depth, vibrato_delay;
	Uint8 pwm_speed, pwm_depth, pwm_delay;

	Uint8 filter_cutoff, filter_resonance, filter_type;

	Uint8 base_note;
	int8_t finetune;
	
	Uint8 sample;
} fzt_instrument;

int import_fzt(FILE *f);

#endif