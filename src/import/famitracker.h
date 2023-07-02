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

// this is based on source code from https://github.com/Dn-Programming-Core-Management/Dn-FamiTracker
// master branch, version from 13 May 2023

#pragma once

#ifndef FAMITRACKER_H
#define FAMITRACKER_H

#include <stdio.h>
#include "../edit.h"
#include "../mused.h"
#include "../event.h"
#include "SDL_endian.h"
#include "../../klystron/src/snd/freqs.h"
#include "../view.h"

#define FAMITRACKER_BLOCK_SIGNATURE_LENGTH 16

#define FT_BLOCK_TYPES 18

#define FT_FILE_VER 0x0450
#define FT_FILE_LOWEST_VER 0x0100

#define FT_MACHINE_NTSC 0
#define FT_MACHINE_PAL 1

#define FT_SNDCHIP_NONE 0		/* just NES that got no expansion chips broke mf lol */
#define FT_SNDCHIP_VRC6 1		/* Konami VRCVI (bruh it's VRC6 and VRC7 respectively, what perv uses Roman numbers there lol) */
#define FT_SNDCHIP_VRC7 2		/* Konami VRCVII */
#define FT_SNDCHIP_FDS  4		/* Famicom Disk Sound */
#define FT_SNDCHIP_MMC5 8		/* Nintendo MMC5 */
#define FT_SNDCHIP_N163 16		/* Namco N-163 (N-106 or however the fuck you freaks call it) */
#define FT_SNDCHIP_S5B  32		/* Sunsoft 5B */

#define FT_SEQ_CNT 5

enum
{
	FT_SEQ_VOLUME,
	FT_SEQ_ARPEGGIO,
	FT_SEQ_PITCH,
	FT_SEQ_HIPITCH,		// TODO: remove this eventually
	FT_SEQ_DUTYCYCLE,
};

#define FT_MAX_FRAMES 256 /* number of patterns in one channel sequence */
#define FT_MAX_CHANNELS 28

#define FT_MAX_DSAMPLES 64
#define FT_MAX_INSTRUMENTS 64

#define FT_MAX_SEQUENCE_ITEMS 252 /* max macro length, 254 would be with GOTO for loop and FB00 for release point so it fits nicely in klystrack 255 length, very pleasant */

#define FT_MAX_SEQUENCES 128

#define FT_FDS_WAVE_SIZE 64
#define FT_FDS_MOD_SIZE 32

#define FT_N163_MAX_WAVE_SIZE 240
#define FT_N163_MAX_WAVE_COUNT 64

#define FT_MAX_EFFECT_COLUMNS 4

#define FT_MAX_SUBSONGS 64

#define FT_HEADER_SIG "FamiTracker Module"
#define DN_FT_HEADER_SIG "Dn-FamiTracker Module"
#define FT_END_SIG "END"

#define CONV_MARKER 0xffff /*marker for later pass conversion, means effect ends with e.g. 1xx and stops with 100*/
#define CONV_MARKER_SWEEP 0xfffe
#define CONV_MARKER_DOWN 0xfffd

enum
{
	INST_NONE = 0,
	INST_2A03 = 1,
	INST_VRC6,
	INST_VRC7,
	INST_FDS,
	INST_N163,
	INST_S5B
};

enum
{
	FT_SETTING_DEFAULT        = 0,

	FT_SETTING_VOL_16_STEPS   = 0,		// // // 050B
	FT_SETTING_VOL_64_STEPS   = 1,		// // // 050B

	FT_SETTING_ARP_ABSOLUTE   = 0,
	FT_SETTING_ARP_FIXED      = 1,
	FT_SETTING_ARP_RELATIVE   = 2,
	FT_SETTING_ARP_SCHEME     = 3,

	FT_SETTING_PITCH_RELATIVE = 0,
	FT_SETTING_PITCH_ABSOLUTE = 1,		// // // 050B

	FT_SETTING_PITCH_SWEEP    = 2,		// // // 050B
};

enum
{
	FT_NOTE_NONE = 0,
	FT_NOTE_RELEASE = 13,
	FT_NOTE_CUT = 14,

	FT_INSTRUMENT_NONE = 255,

	FT_VOLUME_NONE = 16,
};

enum
{
	FT_EF_NONE = 0,
	FT_EF_SPEED,           	// Speed
	FT_EF_JUMP,            	// Jump
	FT_EF_SKIP,            	// Skip
	FT_EF_HALT,            	// Halt
	FT_EF_VOLUME,          	// Volume
	FT_EF_PORTAMENTO,      	// Porta on
	FT_EF_PORTAOFF,        	// Porta off		// unused
	FT_EF_SWEEPUP,         	// Sweep up
	FT_EF_SWEEPDOWN,       	// Sweep down
	FT_EF_ARPEGGIO,        	// Arpeggio
	FT_EF_VIBRATO,         	// Vibrato
	FT_EF_TREMOLO,         	// Tremolo
	FT_EF_PITCH,           	// Pitch
	FT_EF_DELAY,           	// Note delay
	FT_EF_DAC,             	// DAC setting
	FT_EF_PORTA_UP,        	// Portamento up
	FT_EF_PORTA_DOWN,      	// Portamento down
	FT_EF_DUTY_CYCLE,      	// Duty cycle
	FT_EF_SAMPLE_OFFSET,   	// Sample offset
	FT_EF_SLIDE_UP,        	// Slide up
	FT_EF_SLIDE_DOWN,      	// Slide down
	FT_EF_VOLUME_SLIDE,    	// Volume slide
	FT_EF_NOTE_CUT,        	// Note cut
	FT_EF_RETRIGGER,       	// DPCM retrigger
	FT_EF_DELAYED_VOLUME,  	// // // Delayed channel volume
	FT_EF_FDS_MOD_DEPTH,   	// FDS modulation depth
	FT_EF_FDS_MOD_SPEED_HI,	// FDS modulation speed hi
	FT_EF_FDS_MOD_SPEED_LO,	// FDS modulation speed lo
	FT_EF_DPCM_PITCH,      	// DPCM Pitch
	FT_EF_SUNSOFT_ENV_TYPE,	// Sunsoft envelope type
	FT_EF_SUNSOFT_ENV_HI,  	// Sunsoft envelope high
	FT_EF_SUNSOFT_ENV_LO,  	// Sunsoft envelope low
	FT_EF_SUNSOFT_NOISE,   	// // // 050B Sunsoft noise period
	FT_EF_VRC7_PORT,       	// // // 050B VRC7 custom patch port
	FT_EF_VRC7_WRITE,      	// // // 050B VRC7 custom patch write
	FT_EF_NOTE_RELEASE,    	// // // Delayed release
	FT_EF_GROOVE,          	// // // Groove
	FT_EF_TRANSPOSE,       	// // // Delayed transpose
	FT_EF_N163_WAVE_BUFFER,	// // // N163 wave buffer
	FT_EF_FDS_VOLUME,      	// // // FDS volume envelope
	FT_EF_FDS_MOD_BIAS,    	// // // FDS auto-FM bias
	FT_EF_PHASE_RESET,  // Reset waveform phase without retriggering note (VRC6-only so far)
	FT_EF_HARMONIC,  // Multiply the note pitch by an integer
	FT_EF_TARGET_VOLUME_SLIDE,	// // !! Target volume slide

	FT_EF_COUNT
};

enum
{
	INST_TYPE_BLANK = 0,
	
	INST_TYPE_2A03_PULSE, //including MMC5 channels
	INST_TYPE_2A03_TRI,
	INST_TYPE_2A03_NOISE,
	INST_TYPE_2A03_DPCM,

	INST_TYPE_VRC6_PULSE,
	INST_TYPE_VRC6_SAW,

	INST_TYPE_VRC7,

	INST_TYPE_FDS,

	INST_TYPE_N163,

	INST_TYPE_S5B,
};

typedef struct
{
	char name[FAMITRACKER_BLOCK_SIGNATURE_LENGTH + 1];
	Uint32 version;
	Uint32 length;
	
	Uint32 position; // position of actual data so 16+4+4 bytes after block start
} ftm_block;

typedef struct
{
	Uint8 sequence[FT_MAX_SEQUENCE_ITEMS];
	Uint8 sequence_size;
	Sint16 loop; //-1 = none
	Sint16 release; //-1 = none

	Uint8 setting; //BRUH
} ft_inst_macro;

typedef struct
{
	Uint8 sample[8 * 12];
	Uint8 pitch[8 * 12];
} ft_inst_dpcm_sample_map;

typedef struct
{
	ft_inst_macro macros[FT_SEQ_CNT];
	Uint8 fds_wave[FT_FDS_WAVE_SIZE];
	Uint8 fds_mod[FT_FDS_MOD_SIZE];

	Uint32 fds_mod_speed;
	Uint32 fds_mod_depth;
	Uint32 fds_mod_delay;

	Uint8 vrc7_custom_patch[8];

	Uint8 n163_samples[FT_N163_MAX_WAVE_COUNT][FT_N163_MAX_WAVE_SIZE];
	Uint8 n163_samples_len;
	Uint8 num_n163_samples;

	Uint8 type;

	Uint8 seq_indices[FT_SEQ_CNT];
	bool seq_enable[FT_SEQ_CNT];

	Uint8 klystrack_instrument;

	ft_inst_dpcm_sample_map dpcm_sample_map;
} ft_inst;

int import_famitracker(FILE *f, int type);

#endif