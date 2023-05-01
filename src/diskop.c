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

#include "diskop.h"
#include "action.h"
#include "gui/toolutil.h"
#include "mused.h"
#include "macros.h"
#include "gui/msgbox.h"
#include <stdbool.h>
#include "wave.h"
#include "snd/freqs.h"
#include "snd/pack.h"
#include "view/wavetableview.h"
#include <string.h>
#include "memwriter.h"
#include <time.h>
#include <unistd.h>
#include "wavewriter.h"
#include "gui/menu.h"
#include "action.h"
#include <libgen.h>

#include "optimize.h"

extern Mused mused;
extern GfxDomain *domain;

#define MAX_RECENT 10

extern const Menu filemenu[];
Menu recentmenu[MAX_RECENT + 1];


void update_recent_files_list(const char *path)
{
	debug("Adding %s to recent files list", path);

	// Check if path already exists and move all items forward
	// until the found item - or if not found, move all items
	// forward until the last item

	int found;

	for (found = 0; found < MAX_RECENT - 1; ++found)
	{
		if (recentmenu[found].p2 != NULL ||
			(recentmenu[found].p1 != NULL && strcmp((char*)recentmenu[found].p1, path) == 0))
		{
			break;
		}
	}

	if (recentmenu[found].text != NULL)
	{
		free((void*)recentmenu[found].text);
	}

	if (recentmenu[found].p1 != NULL)
	{
		free((void*)recentmenu[found].p1);
	}

	if (found > 0)
	{
		memmove(recentmenu + 1, recentmenu, sizeof(recentmenu[0]) * found);
	}

	// Add new item on top

	char *str = strdup(path);

	Menu *menu = &recentmenu[0];
	menu->parent = filemenu;
	menu->text = strdup(basename(str));
	menu->p1 = strdup(path);
	menu->p2 = NULL;
	menu->action = open_recent_file;

	free(str);
}


void init_recent_files_list()
{
	memset(recentmenu, 0, sizeof(recentmenu));
	// Set a dummy item in case the list is empty to
	// circumvent a bug in the menu routine
	recentmenu[0].parent = filemenu;
	recentmenu[0].text = strdup("No files");
	recentmenu[0].p2 = (void*)1; // Magic value to catch this item

	int list_count = 0;

	debug("Loading recent files list");
	char *e = expand_tilde("~/.klystrackrecent");

	if (e)
	{
		FILE *f = fopen(e, "r");

		if (f)
		{
			for (int i = 0; i < MAX_RECENT; ++i)
			{
				char path[500], cleaned[500];

				if (!fgets(path, sizeof(path), f))
					break;

				sscanf(path, "%500[^\r\n]", cleaned);

				Menu *menu = &recentmenu[i];
				menu->parent = filemenu;
				menu->action = open_recent_file;

				// Order is important: basename might modify the input
				menu->p1 = strdup(cleaned);
				menu->text = strdup(basename(cleaned));
				menu->p2 = NULL;

				list_count++;
			}

			fclose(f);
		}

		free(e);
	}
}


void deinit_recent_files_list()
{
	debug("Saving recent files list");
	char *e = expand_tilde("~/.klystrackrecent"); //char *e = expand_tilde(".klystrackrecent");

	if (e)
	{
		FILE *f = fopen(e, "w");

		if (f)
		{
			for (int i = 0; i < MAX_RECENT; ++i)
			{
				if (recentmenu[i].p1 != NULL)
					fprintf(f,"%s\n", (char*)recentmenu[i].p1);
			}

			fclose(f);
		}
		
		else
		{
			warning("Could not save recent files list");
		}

		free(e);
	}

	for (int i = 0; i < MAX_RECENT; ++i)
	{
		if (recentmenu[i].text != NULL)
		{
			free((void*)recentmenu[i].text);
			free((void*)recentmenu[i].p1);
		}
	}
}


int create_backup(char *filename)
{
	char new_filename[5000] = {0};
	time_t now_time;
	time(&now_time);
	struct tm *now_tm = localtime(&now_time);

	if (!now_tm)
		return 0;

	snprintf(new_filename, sizeof(new_filename) - 1, "%s.%04d%02d%02d-%02d%02d%02d.backup", filename,
		now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);

	return rename(filename, new_filename) == 0;
}


static void write_wavetable_entry(SDL_RWops *f, const CydWavetableEntry *write_wave, bool write_wave_data)
{
	Uint32 flags = write_wave->flags & ~(CYD_WAVE_COMPRESSED_DELTA|CYD_WAVE_COMPRESSED_GRAY); // need to unmask bits because they're set by bitpack
	Uint32 sample_rate = write_wave->sample_rate;
	Uint32 samples = write_wave->samples, loop_begin = write_wave->loop_begin, loop_end = write_wave->loop_end;
	Uint16 base_note = write_wave->base_note;

	if (!write_wave_data)
	{
		// if the wave is not used and the data is not written, set these to zero too
		loop_begin = 0;
		loop_end = 0;
		samples = 0;
	}

	Uint8 *packed = NULL;
	Uint32 packed_size = 0;

	if (write_wave->samples > 0 && write_wave_data)
	{
		int pack_flags;
		packed = bitpack_best(write_wave->data, write_wave->samples, &packed_size, &pack_flags);

		flags |= (Uint32)pack_flags << 3;
	}

	FIX_ENDIAN(flags);
	FIX_ENDIAN(sample_rate);
	FIX_ENDIAN(samples);
	FIX_ENDIAN(loop_begin);
	FIX_ENDIAN(loop_end);
	FIX_ENDIAN(base_note);

	SDL_RWwrite(f, &flags, sizeof(flags), 1);
	SDL_RWwrite(f, &sample_rate, sizeof(sample_rate), 1);
	SDL_RWwrite(f, &samples, sizeof(samples), 1);
	SDL_RWwrite(f, &loop_begin, sizeof(loop_begin), 1);
	SDL_RWwrite(f, &loop_end, sizeof(loop_end), 1);
	SDL_RWwrite(f, &base_note, sizeof(base_note), 1);


	if (packed)
	{
		FIX_ENDIAN(packed_size);
		SDL_RWwrite(f, &packed_size, sizeof(packed_size), 1);

		SDL_RWwrite(f, packed, sizeof(packed[0]), (packed_size + 7) / 8);

		free(packed);
	}
}


/*  Write max 255 character string
 */

static void write_string8(SDL_RWops *f, const char * string)
{
	Uint8 len = strlen(string);
	SDL_RWwrite(f, &len, sizeof(len), 1);
	if (len)
		SDL_RWwrite(f, string, sizeof(string[0]), len);
}


static void save_instrument_inner(SDL_RWops *f, MusInstrument *inst, const CydWavetableEntry *write_wave, const CydWavetableEntry *write_wave_fm)
{
	if(inst->num_macros == 1)
	{
		inst->flags &= ~(MUS_INST_SEVERAL_MACROS);
	}
	
	Uint32 temp32_f = inst->flags;
	
	if(inst->num_macros > 1)
	{
		temp32_f |= MUS_INST_SEVERAL_MACROS;
	}
	
	FIX_ENDIAN(temp32_f);
	SDL_RWwrite(f, &temp32_f, sizeof(temp32_f), 1);
	
	Uint32 temp32 = inst->cydflags;
	FIX_ENDIAN(temp32);
	SDL_RWwrite(f, &temp32, sizeof(temp32), 1);
	//SDL_RWwrite(f, &inst->adsr, sizeof(inst->adsr), 1);
	
	if(inst->cydflags & CYD_CHN_ENABLE_FILTER)
	{
		inst->adsr.a |= ((inst->flttype & 0b110) << 5);
		inst->adsr.d |= ((inst->flttype & 0b1) << 6);
	}
	
	SDL_RWwrite(f, &inst->adsr.a, sizeof(inst->adsr.a), 1);
	SDL_RWwrite(f, &inst->adsr.d, sizeof(inst->adsr.d), 1);
	
	Uint8 temp8 = inst->adsr.s;
	temp8 |= (inst->slope << 5);
	SDL_RWwrite(f, &temp8, sizeof(temp8), 1);
	
	SDL_RWwrite(f, &inst->adsr.r, sizeof(inst->adsr.r), 1);
	
	inst->adsr.a &= 0b00111111;
	inst->adsr.d &= 0b00111111;
	inst->adsr.s &= 0b00011111;
	inst->adsr.r &= 0b00111111;
	
	if(inst->flags & MUS_INST_USE_VOLUME_ENVELOPE)
	{
		SDL_RWwrite(f, &inst->vol_env_flags, sizeof(inst->vol_env_flags), 1);
		SDL_RWwrite(f, &inst->num_vol_points, sizeof(inst->num_vol_points), 1);
		
		Uint16 temp16 = inst->vol_env_fadeout;
		FIX_ENDIAN(temp16);
		SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
		
		if(inst->vol_env_flags & MUS_ENV_SUSTAIN)
		{
			SDL_RWwrite(f, &inst->vol_env_sustain, sizeof(inst->vol_env_sustain), 1);
		}
	
		if(inst->vol_env_flags & MUS_ENV_LOOP)
		{
			SDL_RWwrite(f, &inst->vol_env_loop_start, sizeof(inst->vol_env_loop_start), 1);
			SDL_RWwrite(f, &inst->vol_env_loop_end, sizeof(inst->vol_env_loop_end), 1);
		}
		
		for(int i = 0; i < inst->num_vol_points; ++i)
		{
			temp16 = inst->volume_envelope[i].x;
			FIX_ENDIAN(temp16);
			SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
			
			SDL_RWwrite(f, &inst->volume_envelope[i].y, sizeof(inst->volume_envelope[i].y), 1);
		}
	}
	
	if(inst->flags & MUS_INST_USE_PANNING_ENVELOPE)
	{
		SDL_RWwrite(f, &inst->pan_env_flags, sizeof(inst->pan_env_flags), 1);
		SDL_RWwrite(f, &inst->num_pan_points, sizeof(inst->num_pan_points), 1);
		
		Uint16 temp16 = inst->pan_env_fadeout;
		FIX_ENDIAN(temp16);
		SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
		
		if(inst->pan_env_flags & MUS_ENV_SUSTAIN)
		{
			SDL_RWwrite(f, &inst->pan_env_sustain, sizeof(inst->pan_env_sustain), 1);
		}
	
		if(inst->pan_env_flags & MUS_ENV_LOOP)
		{
			SDL_RWwrite(f, &inst->pan_env_loop_start, sizeof(inst->pan_env_loop_start), 1);
			SDL_RWwrite(f, &inst->pan_env_loop_end, sizeof(inst->pan_env_loop_end), 1);
		}
		
		for(int i = 0; i < inst->num_pan_points; ++i)
		{
			temp16 = inst->panning_envelope[i].x;
			FIX_ENDIAN(temp16);
			SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
			
			SDL_RWwrite(f, &inst->panning_envelope[i].y, sizeof(inst->panning_envelope[i].y), 1);
		}
	}
	
	if(inst->cydflags & CYD_CHN_ENABLE_FIXED_NOISE_PITCH)
	{
		SDL_RWwrite(f, &inst->noise_note, sizeof(inst->noise_note), 1);
	}
	
	if(inst->cydflags & CYD_CHN_ENABLE_VOLUME_KEY_SCALING)
	{
		SDL_RWwrite(f, &inst->vol_ksl_level, sizeof(inst->vol_ksl_level), 1);
	}
	
	if(inst->cydflags & CYD_CHN_ENABLE_ENVELOPE_KEY_SCALING)
	{
		SDL_RWwrite(f, &inst->env_ksl_level, sizeof(inst->env_ksl_level), 1);
	}
	
	if(inst->cydflags & CYD_CHN_ENABLE_SYNC)
	{
		SDL_RWwrite(f, &inst->sync_source, sizeof(inst->sync_source), 1);
	}
	
	if(inst->cydflags & CYD_CHN_ENABLE_RING_MODULATION)
	{
		SDL_RWwrite(f, &inst->ring_mod, sizeof(inst->ring_mod), 1);
	}
	
	Uint16 temp16 = 0;
	
	temp16 = inst->pw;
	
	temp16 |= (inst->mixmode << 12);
	
	FIX_ENDIAN(temp16);
	SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
	SDL_RWwrite(f, &inst->volume, sizeof(inst->volume), 1);
	
	Uint8 temp_macros = 1;
	
	if(inst->num_macros > 1)
	{
		temp_macros = inst->num_macros;
		
		if(is_empty_program(inst->program[inst->num_macros - 1])) //the last one may be empty
		{
			temp_macros--;
		}
		
		SDL_RWwrite(f, &temp_macros, sizeof(temp_macros), 1);
	}
	
	for(int pr = 0; pr < temp_macros; ++pr)
	{
		write_string8(f, inst->program_names[pr]);
		
		Uint8 progsteps = 0;
		
		for (int i = 0; i < MUS_PROG_LEN; ++i)
		{
			if (inst->program[pr][i] != MUS_FX_NOP) 
			{
				progsteps = i + 1;
			}
		}
		
		SDL_RWwrite(f, &progsteps, sizeof(progsteps), 1);
		
		if(progsteps != 0)
		{
			for (int i = 0; i < progsteps / 8 + 1; ++i)
			{
				SDL_RWwrite(f, &inst->program_unite_bits[pr][i], sizeof(Uint8), 1);
			}
		}
		
		for (int i = 0; i < progsteps; ++i)
		{
			temp16 = inst->program[pr][i];
			FIX_ENDIAN(temp16);
			SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
		}

		SDL_RWwrite(f, &inst->prog_period[pr], sizeof(inst->prog_period[pr]), 1);
	}
	
	if(inst->flags & MUS_INST_SAVE_LFO_SETTINGS)
	{
		SDL_RWwrite(f, &inst->vibrato_speed, sizeof(inst->vibrato_speed), 1);
		SDL_RWwrite(f, &inst->vibrato_depth, sizeof(inst->vibrato_depth), 1);
		SDL_RWwrite(f, &inst->pwm_speed, sizeof(inst->pwm_speed), 1);
		SDL_RWwrite(f, &inst->pwm_depth, sizeof(inst->pwm_depth), 1);
		
		SDL_RWwrite(f, &inst->pwm_delay, sizeof(inst->pwm_delay), 1);
		
		SDL_RWwrite(f, &inst->tremolo_speed, sizeof(inst->tremolo_speed), 1);
		SDL_RWwrite(f, &inst->tremolo_depth, sizeof(inst->tremolo_depth), 1);
		SDL_RWwrite(f, &inst->tremolo_shape, sizeof(inst->tremolo_shape), 1);
		SDL_RWwrite(f, &inst->tremolo_delay, sizeof(inst->tremolo_delay), 1);
	}
	
	Uint16 temp_sl_sp = inst->slide_speed | ((inst->sine_acc_shift) << 12);
	FIX_ENDIAN(temp_sl_sp);
	SDL_RWwrite(f, &temp_sl_sp, sizeof(temp_sl_sp), 1);
	
	SDL_RWwrite(f, &inst->base_note, sizeof(inst->base_note), 1);
	SDL_RWwrite(f, &inst->finetune, sizeof(inst->finetune), 1);

	write_string8(f, inst->name);
	
	if(inst->cydflags & CYD_CHN_ENABLE_FILTER)
	{
		temp16 = inst->cutoff | (inst->resonance << 12);
		FIX_ENDIAN(temp16);
		SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
		//SDL_RWwrite(f, &inst->resonance, sizeof(inst->resonance), 1);
		//SDL_RWwrite(f, &inst->flttype, sizeof(inst->flttype), 1);
		//SDL_RWwrite(f, &inst->slope, sizeof(inst->slope), 1);
	}
	
	if(inst->flags & MUS_INST_YM_BUZZ)
	{
		SDL_RWwrite(f, &inst->ym_env_shape, sizeof(inst->ym_env_shape), 1);
		temp16 = inst->buzz_offset;
		FIX_ENDIAN(temp16);
		SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
	}
	
	if(inst->cydflags & CYD_CHN_ENABLE_FX)
	{
		SDL_RWwrite(f, &inst->fx_bus, sizeof(inst->fx_bus), 1);
	}
	
	if(inst->flags & MUS_INST_SAVE_LFO_SETTINGS)
	{
		SDL_RWwrite(f, &inst->vibrato_shape, sizeof(inst->vibrato_shape), 1);
		SDL_RWwrite(f, &inst->vibrato_delay, sizeof(inst->vibrato_delay), 1);
		SDL_RWwrite(f, &inst->pwm_shape, sizeof(inst->pwm_shape), 1);
	}
	
	if(inst->cydflags & CYD_CHN_ENABLE_LFSR)
	{
		SDL_RWwrite(f, &inst->lfsr_type, sizeof(inst->lfsr_type), 1);
	}
	
	//SDL_RWwrite(f, &inst->mixmode, sizeof(inst->mixmode), 1); //wasn't there
	
	if (write_wave)
	{
		Uint8 temp = 0xff;
		SDL_RWwrite(f, &temp, sizeof(temp), 1);
	}
		
	else
	{
		SDL_RWwrite(f, &inst->wavetable_entry, sizeof(inst->wavetable_entry), 1);
	}
	
	if(inst->cydflags & CYD_CHN_ENABLE_FM)
	{
		SDL_RWwrite(f, &inst->fm_flags, sizeof(inst->fm_flags), 1);
		SDL_RWwrite(f, &inst->fm_modulation, sizeof(inst->fm_modulation), 1); //fm volume
		
		if(inst->fm_flags & CYD_FM_ENABLE_VOLUME_KEY_SCALING)
		{
			SDL_RWwrite(f, &inst->fm_vol_ksl_level, sizeof(inst->fm_vol_ksl_level), 1);
		}
		
		if(inst->fm_flags & CYD_FM_ENABLE_ENVELOPE_KEY_SCALING)
		{
			SDL_RWwrite(f, &inst->fm_env_ksl_level, sizeof(inst->fm_env_ksl_level), 1);
		}
		
		//SDL_RWwrite(f, &inst->fm_feedback, sizeof(inst->fm_feedback), 1);
		SDL_RWwrite(f, &inst->fm_harmonic, sizeof(inst->fm_harmonic), 1);
		
		//SDL_RWwrite(f, &inst->fm_adsr, sizeof(inst->fm_adsr), 1);
		
		inst->fm_adsr.a |= (inst->fm_freq_LUT << 6);
		SDL_RWwrite(f, &inst->fm_adsr.a, sizeof(inst->fm_adsr.a), 1);
		
		SDL_RWwrite(f, &inst->fm_adsr.d, sizeof(inst->fm_adsr.d), 1);
		
		inst->fm_adsr.s |= (inst->fm_feedback << 5);
		SDL_RWwrite(f, &inst->fm_adsr.s, sizeof(inst->fm_adsr.d), 1);
		
		SDL_RWwrite(f, &inst->fm_adsr.r, sizeof(inst->fm_adsr.r), 1);
		
		inst->fm_adsr.a &= 0b00111111;
		inst->fm_adsr.d &= 0b00111111;
		inst->fm_adsr.s &= 0b00011111;
		inst->fm_adsr.r &= 0b00111111;
		
		SDL_RWwrite(f, &inst->fm_attack_start, sizeof(inst->fm_attack_start), 1);
		
		SDL_RWwrite(f, &inst->fm_base_note, sizeof(inst->fm_base_note), 1); //weren't there
		SDL_RWwrite(f, &inst->fm_finetune, sizeof(inst->fm_finetune), 1);
		
		//SDL_RWwrite(f, &inst->fm_freq_LUT, sizeof(inst->fm_freq_LUT), 1);
		
		if(inst->fm_flags & CYD_FM_SAVE_LFO_SETTINGS)
		{
			SDL_RWwrite(f, &inst->fm_vibrato_speed, sizeof(inst->fm_vibrato_speed), 1);
			SDL_RWwrite(f, &inst->fm_vibrato_depth, sizeof(inst->fm_vibrato_depth), 1);
			SDL_RWwrite(f, &inst->fm_vibrato_shape, sizeof(inst->fm_vibrato_shape), 1);
			SDL_RWwrite(f, &inst->fm_vibrato_delay, sizeof(inst->fm_vibrato_delay), 1);
			
			SDL_RWwrite(f, &inst->fm_tremolo_speed, sizeof(inst->fm_tremolo_speed), 1);
			SDL_RWwrite(f, &inst->fm_tremolo_depth, sizeof(inst->fm_tremolo_depth), 1);
			SDL_RWwrite(f, &inst->fm_tremolo_shape, sizeof(inst->fm_tremolo_shape), 1);
			SDL_RWwrite(f, &inst->fm_tremolo_delay, sizeof(inst->fm_tremolo_delay), 1);
		}
		
		if(inst->fm_flags & CYD_FM_ENABLE_4OP)
		{
			SDL_RWwrite(f, &inst->alg, sizeof(inst->alg), 1);
			SDL_RWwrite(f, &inst->fm_4op_vol, sizeof(inst->fm_4op_vol), 1);
			
			for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
			{
				if(inst->ops[i].num_macros == 1)
				{
					inst->ops[i].flags &= ~(MUS_FM_OP_SEVERAL_MACROS);
				}
				
				Uint16 temp16_f = inst->ops[i].flags;
				
				if(inst->ops[i].num_macros > 1)
				{
					temp16_f |= MUS_FM_OP_SEVERAL_MACROS;
				}
				
				FIX_ENDIAN(temp16_f);
				SDL_RWwrite(f, &temp16_f, sizeof(temp16_f), 1);
				Uint32 temp32 = inst->ops[i].cydflags;
				FIX_ENDIAN(temp32);
				SDL_RWwrite(f, &temp32, sizeof(temp32), 1);
				
				Uint16 temp_sl_sp_op = inst->ops[i].slide_speed | ((inst->ops[i].sine_acc_shift) << 12);
				FIX_ENDIAN(temp_sl_sp_op);
				SDL_RWwrite(f, &temp_sl_sp_op, sizeof(temp_sl_sp_op), 1);
				
				if(inst->fm_flags & CYD_FM_ENABLE_3CH_EXP_MODE)
				{
					SDL_RWwrite(f, &inst->ops[i].base_note, sizeof(inst->ops[i].base_note), 1);
					SDL_RWwrite(f, &inst->ops[i].finetune, sizeof(inst->ops[i].finetune), 1);
				}
				
				else
				{
					SDL_RWwrite(f, &inst->ops[i].harmonic, sizeof(inst->ops[i].harmonic), 1);
					
					Uint8 unsigned_detune = (Uint8)(inst->ops[i].detune + 7);
					
					Uint8 temp_detune = ((unsigned_detune << 4) | ((inst->ops[i].coarse_detune) << 2));
					
					SDL_RWwrite(f, &temp_detune, sizeof(temp_detune), 1);
				}
				
				Uint8 temp_feedback_ssgeg = inst->ops[i].feedback;
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_SSG_EG)
				{
					temp_feedback_ssgeg |= (inst->ops[i].ssg_eg_type << 4);
				}
				
				SDL_RWwrite(f, &temp_feedback_ssgeg, sizeof(temp_feedback_ssgeg), 1);
				
				//if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_FILTER)
				//{
					//inst->ops[i].adsr.a |= ((inst->ops[i].flttype & 0b110) << 5);
					//inst->ops[i].adsr.d |= ((inst->ops[i].flttype & 0b1) << 6);
				//}
				
				SDL_RWwrite(f, &inst->ops[i].adsr.a, sizeof(inst->ops[i].adsr.a), 1);
				SDL_RWwrite(f, &inst->ops[i].adsr.d, sizeof(inst->ops[i].adsr.d), 1);
				
				Uint8 temp8 = inst->ops[i].adsr.s;
				temp8 |= (inst->ops[i].slope << 5);
				SDL_RWwrite(f, &temp8, sizeof(temp8), 1);
				
				SDL_RWwrite(f, &inst->ops[i].adsr.r, sizeof(inst->ops[i].adsr.r), 1);
				
				SDL_RWwrite(f, &inst->ops[i].adsr.sr, sizeof(inst->ops[i].adsr.sr), 1); //wasn't there
				
				//inst->ops[i].adsr.a &= 0b00111111;
				//inst->ops[i].adsr.d &= 0b00111111;
				inst->ops[i].adsr.s &= 0b00011111;
				//inst->ops[i].adsr.r &= 0b00111111;
				
				if(inst->ops[i].flags & MUS_FM_OP_USE_VOLUME_ENVELOPE)
				{
					SDL_RWwrite(f, &inst->ops[i].vol_env_flags, sizeof(inst->ops[i].vol_env_flags), 1);
					SDL_RWwrite(f, &inst->ops[i].num_vol_points, sizeof(inst->ops[i].num_vol_points), 1);
					
					Uint16 temp16 = inst->ops[i].vol_env_fadeout;
					FIX_ENDIAN(temp16);
					SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
					
					if(inst->ops[i].vol_env_flags & MUS_ENV_SUSTAIN)
					{
						SDL_RWwrite(f, &inst->ops[i].vol_env_sustain, sizeof(inst->ops[i].vol_env_sustain), 1);
					}
				
					if(inst->ops[i].vol_env_flags & MUS_ENV_LOOP)
					{
						SDL_RWwrite(f, &inst->ops[i].vol_env_loop_start, sizeof(inst->ops[i].vol_env_loop_start), 1);
						SDL_RWwrite(f, &inst->ops[i].vol_env_loop_end, sizeof(inst->ops[i].vol_env_loop_end), 1);
					}
					
					for(int j = 0; j < inst->ops[i].num_vol_points; ++j)
					{
						temp16 = inst->ops[i].volume_envelope[j].x;
						FIX_ENDIAN(temp16);
						SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
						
						SDL_RWwrite(f, &inst->ops[i].volume_envelope[j].y, sizeof(inst->ops[i].volume_envelope[j].y), 1);
					}
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_FIXED_NOISE_PITCH)
				{
					SDL_RWwrite(f, &inst->ops[i].noise_note, sizeof(inst->ops[i].noise_note), 1);
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_VOLUME_KEY_SCALING)
				{
					SDL_RWwrite(f, &inst->ops[i].vol_ksl_level, sizeof(inst->ops[i].vol_ksl_level), 1);
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_ENVELOPE_KEY_SCALING)
				{
					SDL_RWwrite(f, &inst->ops[i].env_ksl_level, sizeof(inst->ops[i].env_ksl_level), 1);
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_SYNC)
				{
					SDL_RWwrite(f, &inst->ops[i].sync_source, sizeof(inst->ops[i].sync_source), 1);
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_RING_MODULATION)
				{
					SDL_RWwrite(f, &inst->ops[i].ring_mod, sizeof(inst->ops[i].ring_mod), 1);
				}
				
				Uint16 temp16 = 0;
				
				temp16 = inst->ops[i].pw;
				
				temp16 |= (inst->ops[i].mixmode << 12);
				
				FIX_ENDIAN(temp16);
				SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
				SDL_RWwrite(f, &inst->ops[i].volume, sizeof(inst->ops[i].volume), 1);
				
				Uint8 temp_macros_op = 1;
	
				if(inst->ops[i].num_macros > 1)
				{
					temp_macros_op = inst->ops[i].num_macros;
					
					if(is_empty_program(inst->ops[i].program[inst->num_macros - 1])) //the last one may be empty
					{
						temp_macros_op--;
					}
					
					SDL_RWwrite(f, &temp_macros_op, sizeof(temp_macros_op), 1);
					
					debug("macros %d", temp_macros_op);
				}
				
				for(int pr = 0; pr < temp_macros_op; ++pr)
				{
					write_string8(f, inst->ops[i].program_names[pr]);
					
					Uint8 progsteps = 0;
					
					for (int j = 0; j < MUS_PROG_LEN; ++j)
					{
						if (inst->ops[i].program[pr][j] != MUS_FX_NOP) 
						{
							progsteps = j + 1;
						}
					}
					
					SDL_RWwrite(f, &progsteps, sizeof(progsteps), 1);
					
					if(progsteps != 0)
					{
						for (int i1 = 0; i1 < progsteps / 8 + 1; ++i1)
						{
							SDL_RWwrite(f, &inst->ops[i].program_unite_bits[pr][i1], sizeof(Uint8), 1);
						}
					}
					
					for (int i1 = 0; i1 < progsteps; ++i1)
					{
						temp16 = inst->ops[i].program[pr][i1];
						FIX_ENDIAN(temp16);
						SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
					}

					SDL_RWwrite(f, &inst->ops[i].prog_period[pr], sizeof(inst->ops[i].prog_period[pr]), 1);
				}
				
				if(inst->ops[i].flags & MUS_FM_OP_SAVE_LFO_SETTINGS)
				{
					SDL_RWwrite(f, &inst->ops[i].vibrato_speed, sizeof(inst->ops[i].vibrato_speed), 1);
					SDL_RWwrite(f, &inst->ops[i].vibrato_depth, sizeof(inst->ops[i].vibrato_depth), 1);
					SDL_RWwrite(f, &inst->ops[i].vibrato_shape, sizeof(inst->ops[i].tremolo_shape), 1);
					SDL_RWwrite(f, &inst->ops[i].vibrato_delay, sizeof(inst->ops[i].tremolo_delay), 1);
					
					SDL_RWwrite(f, &inst->ops[i].pwm_speed, sizeof(inst->ops[i].pwm_speed), 1);
					SDL_RWwrite(f, &inst->ops[i].pwm_depth, sizeof(inst->ops[i].pwm_depth), 1);
					SDL_RWwrite(f, &inst->ops[i].pwm_shape, sizeof(inst->ops[i].pwm_shape), 1);
					SDL_RWwrite(f, &inst->ops[i].pwm_delay, sizeof(inst->ops[i].pwm_delay), 1);
					
					SDL_RWwrite(f, &inst->ops[i].tremolo_speed, sizeof(inst->ops[i].tremolo_speed), 1);
					SDL_RWwrite(f, &inst->ops[i].tremolo_depth, sizeof(inst->ops[i].tremolo_depth), 1);
					SDL_RWwrite(f, &inst->ops[i].tremolo_shape, sizeof(inst->ops[i].tremolo_shape), 1);
					SDL_RWwrite(f, &inst->ops[i].tremolo_delay, sizeof(inst->ops[i].tremolo_delay), 1);
				}
				
				SDL_RWwrite(f, &inst->ops[i].trigger_delay, sizeof(inst->ops[i].trigger_delay), 1);
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_CSM_TIMER)
				{
					SDL_RWwrite(f, &inst->ops[i].CSM_timer_note, sizeof(inst->ops[i].CSM_timer_note), 1);
					SDL_RWwrite(f, &inst->ops[i].CSM_timer_finetune, sizeof(inst->ops[i].CSM_timer_finetune), 1);
				}
				
				if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_FILTER)
				{
					temp16 = inst->ops[i].cutoff | (inst->ops[i].resonance << 12);
					FIX_ENDIAN(temp16);
					SDL_RWwrite(f, &temp16, sizeof(temp16), 1);
					SDL_RWwrite(f, &inst->ops[i].flttype, sizeof(inst->ops[i].flttype), 1);
				}
				
				if (inst->ops[i].cydflags & CYD_FM_OP_ENABLE_WAVE)
				{
					SDL_RWwrite(f, &inst->ops[i].wavetable_entry, sizeof(inst->ops[i].wavetable_entry), 1);
				}
			}
		}
	}

	if (write_wave_fm)
	{
		Uint8 temp = (inst->wavetable_entry == inst->fm_wave) ? 0xfe : 0xff;
		SDL_RWwrite(f, &temp, sizeof(temp), 1);
	}
		
	else
	{
		SDL_RWwrite(f, &inst->fm_wave, sizeof(inst->fm_wave), 1);
	}

	if (write_wave)
	{
		if((inst->cydflags & CYD_CHN_ENABLE_WAVE) || ((inst->fm_flags & CYD_FM_ENABLE_WAVE) && (inst->wavetable_entry == inst->fm_wave)))
		write_wavetable_entry(f, write_wave, true);
	}
	
	if (write_wave_fm)
	{
		if (inst->wavetable_entry != inst->fm_wave && (inst->fm_flags & CYD_FM_ENABLE_WAVE))
		{
			write_wavetable_entry(f, write_wave_fm, true);
		}
	}
	
	if((inst->fm_flags & CYD_FM_ENABLE_4OP) && write_wave)
	{
		Uint8 wave_entries[CYD_FM_NUM_OPS] = { 0xFF };
		
		for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
		{
			if(inst->ops[i].cydflags & CYD_FM_OP_ENABLE_WAVE)
			{
				wave_entries[i] = inst->ops[i].wavetable_entry;
			}
		}
		
		for(int i = 0; i < CYD_FM_NUM_OPS; ++i)
		{
			if (inst->ops[i].cydflags & CYD_FM_OP_ENABLE_WAVE)
			{
				bool need_write_wave = true;
				
				for(int j = 0; j < i; ++j)
				{
					if(wave_entries[j] == wave_entries[i] && wave_entries[i] != 0xff && wave_entries[j] != 0xff && (inst->ops[j].cydflags & CYD_FM_OP_ENABLE_WAVE))
					{
						need_write_wave = false;
						goto end;
					}
				}
				
				end:;
				
				if(need_write_wave)
				{
					write_wavetable_entry(f, &mused.mus.cyd->wavetable_entries[inst->ops[i].wavetable_entry], true);
				}
			}
		}
	}
}


static void save_fx_inner(SDL_RWops *f, CydFxSerialized *fx)
{
	CydFxSerialized temp;
	memcpy(&temp, fx, sizeof(temp));

	FIX_ENDIAN(temp.flags);
	for (int i = 0; i < CYDRVB_TAPS; ++i)
	{
		FIX_ENDIAN(temp.rvb.tap[i].gain);
		FIX_ENDIAN(temp.rvb.tap[i].delay);
	}

	write_string8(f, temp.name);

	SDL_RWwrite(f, &temp.flags, sizeof(temp.flags), 1);
	
	if(temp.flags & CYDFX_ENABLE_CRUSH)
	{
		SDL_RWwrite(f, &temp.crush.bit_drop, sizeof(temp.crush.bit_drop), 1);
	}
	
	if(temp.flags & CYDFX_ENABLE_CHORUS)
	{
		SDL_RWwrite(f, &temp.chr.rate, sizeof(temp.chr.rate), 1);
		SDL_RWwrite(f, &temp.chr.min_delay, sizeof(temp.chr.min_delay), 1);
		SDL_RWwrite(f, &temp.chr.max_delay, sizeof(temp.chr.max_delay), 1);
		SDL_RWwrite(f, &temp.chr.sep, sizeof(temp.chr.sep), 1);
	}

	if(temp.flags & CYDFX_ENABLE_REVERB)
	{
		SDL_RWwrite(f, &temp.rvb.taps_quant, sizeof(temp.rvb.taps_quant), 1);
		
		for (int i = 0; i < temp.rvb.taps_quant; ++i)
		{
			SDL_RWwrite(f, &temp.rvb.tap[i].delay, sizeof(temp.rvb.tap[i].delay), 1);
			SDL_RWwrite(f, &temp.rvb.tap[i].gain, sizeof(temp.rvb.tap[i].gain), 1);
			SDL_RWwrite(f, &temp.rvb.tap[i].panning, sizeof(temp.rvb.tap[i].panning), 1);
			SDL_RWwrite(f, &temp.rvb.tap[i].flags, sizeof(temp.rvb.tap[i].flags), 1);
		}
	}

	if(temp.flags & CYDFX_ENABLE_CRUSH)
	{
		SDL_RWwrite(f, &temp.crushex.downsample, sizeof(temp.crushex.downsample), 1);
		SDL_RWwrite(f, &temp.crushex.gain, sizeof(temp.crushex.gain), 1);
	}
}


static void write_packed_pattern(SDL_RWops *f, const MusPattern *pattern, bool skip)
{
	/*

	Format:
		00 		1 * byte			number of incoming steps
		01...	n_steps * nibble	incoming data
		...		n_steps * variable	data described by bits in nibble

		If ctrl bit 7 is set, there's also a volume column incoming
	*/

	Uint16 steps = pattern->num_steps;

	if (skip) steps = 0;

	FIX_ENDIAN(steps);
	SDL_RWwrite(f, &steps, 1, sizeof(steps));
	SDL_RWwrite(f, &pattern->color, 1, sizeof(pattern->color));

	Uint8 buffer = 0;
	
	for (int i = 0; i < steps; ++i)
	{
		if (pattern->step[i].note != MUS_NOTE_NONE)
			buffer |= MUS_PAK_BIT_NOTE;

		if (pattern->step[i].instrument != MUS_NOTE_NO_INSTRUMENT)
			buffer |= MUS_PAK_BIT_INST;

		if (pattern->step[i].ctrl != 0 || pattern->step[i].volume != MUS_NOTE_NO_VOLUME
		|| pattern->step[i].command[1] != 0 || pattern->step[i].command[2] != 0
		|| pattern->step[i].command[3] != 0 || pattern->step[i].command[4] != 0
		|| pattern->step[i].command[5] != 0 || pattern->step[i].command[6] != 0
		|| pattern->step[i].command[7] != 0)
			buffer |= MUS_PAK_BIT_CTRL; //MUS_PAK_BIT_NEW_CMDS_1 to 3 go to ctrl. So ctrl if little-endian should be like this: V C C C T Vib S L, where V is volume, C bits for new commands (coding how many commands there would be besides command 1, so 1-7 additional commands), T tremolo, Vib vibrato, S slide and L legato

		if (pattern->step[i].command[0] != 0)
			buffer |= MUS_PAK_BIT_CMD;

		if (i & 1 || i + 1 >= steps)
			SDL_RWwrite(f, &buffer, 1, sizeof(buffer));

		buffer <<= 4;
	}

	for (int i = 0; i < steps; ++i)
	{
		if (pattern->step[i].note != MUS_NOTE_NONE)
			SDL_RWwrite(f, &pattern->step[i].note, 1, sizeof(pattern->step[i].note));

		if (pattern->step[i].instrument != MUS_NOTE_NO_INSTRUMENT)
			SDL_RWwrite(f, &pattern->step[i].instrument, 1, sizeof(pattern->step[i].instrument));
		
		
		Uint8 flag = 1;
		Uint8 coding_bits = 0; // C C C bits from above
		
		for(int j = MUS_MAX_COMMANDS - 1; j > 0 && flag == 1; --j)
		{
			if(pattern->step[i].command[j] != 0)
			{
				flag = 0;
				coding_bits = j;
			}
		}
		
		Uint8 ctrl = pattern->step[i].ctrl;
		ctrl |= (coding_bits & 7) << 4;

		if (pattern->step[i].ctrl != 0 || pattern->step[i].volume != MUS_NOTE_NO_VOLUME || ctrl != 0)
		{
			if (pattern->step[i].volume != MUS_NOTE_NO_VOLUME)
			{
				ctrl |= MUS_PAK_BIT_VOLUME;
			}
			
			SDL_RWwrite(f, &ctrl, 1, sizeof(pattern->step[i].ctrl));
		}
		
		if (pattern->step[i].command[0] != 0)
		{
			Uint16 c = pattern->step[i].command[0];
			FIX_ENDIAN(c);
			SDL_RWwrite(f, &c, 1, sizeof(pattern->step[i].command[0]));
		}
		
		
		for(int j = 0; j < coding_bits; j++)
		{
			Uint16 c = pattern->step[i].command[j + 1];
			FIX_ENDIAN(c);
			SDL_RWwrite(f, &c, 1, sizeof(pattern->step[i].command[j + 1]));
		}
		

		if (pattern->step[i].volume != MUS_NOTE_NO_VOLUME)
		{
			SDL_RWwrite(f, &pattern->step[i].volume, 1, sizeof(pattern->step[i].volume));
		}
	}
}


int open_song(FILE *f)
{
	new_song();

	if (!mus_load_song_file(f, &mused.song, mused.mus.cyd->wavetable_entries)) return 0;

	mused.song.num_patterns = NUM_PATTERNS;
	mused.song.num_instruments = NUM_INSTRUMENTS;

	// Use kewlkool heuristics to determine sequence spacing

	mused.sequenceview_steps = mused.song.sequence_step;

	if (mused.sequenceview_steps == 0)
	{
		mused.sequenceview_steps = 1000;

		for (int c = 0; c < MUS_MAX_CHANNELS; ++c)
			for (int s = 1; s < mused.song.num_sequences[c] && mused.song.sequence[c][s-1].position < mused.song.song_length; ++s)
				if (mused.sequenceview_steps > mused.song.sequence[c][s].position - mused.song.sequence[c][s - 1].position)
				{
					mused.sequenceview_steps = mused.song.sequence[c][s].position - mused.song.sequence[c][s - 1].position;
				}

		for (int c = 0; c < MUS_MAX_CHANNELS; ++c)
			if (mused.song.num_sequences[c] > 0 && mused.song.sequence[c][mused.song.num_sequences[c] - 1].position < mused.song.song_length)
			{
				if (mused.sequenceview_steps > mused.song.song_length - mused.song.sequence[c][mused.song.num_sequences[c] - 1].position)
				{
					mused.sequenceview_steps = mused.song.song_length - mused.song.sequence[c][mused.song.num_sequences[c] - 1].position;
				}
			}

		if (mused.sequenceview_steps < 1) mused.sequenceview_steps = 1;
		if (mused.sequenceview_steps == 1000) mused.sequenceview_steps = 16;
	}

	mus_set_fx(&mused.mus, &mused.song);
	enable_callback(true);
	mirror_flags();

	if (!mused.song.time_signature) mused.song.time_signature = 0x404;

	mused.time_signature = mused.song.time_signature;

	mused.flags &= ~EDIT_MODE;

	unmute_all_action(NULL, NULL, NULL);

	for (int i = 0; i < mused.song.num_patterns; ++i)
		if(mused.song.pattern[i].num_steps == 0)
			resize_pattern(&mused.song.pattern[i], mused.default_pattern_length);

	mused.song.wavetable_names = realloc(mused.song.wavetable_names, sizeof(mused.song.wavetable_names[0]) * CYD_WAVE_MAX_ENTRIES);

	for (int i = mused.song.num_wavetables; i < CYD_WAVE_MAX_ENTRIES; ++i)
	{
		mused.song.wavetable_names[i] = malloc(MUS_WAVETABLE_NAME_LEN + 1);
		memset(mused.song.wavetable_names[i], 0, MUS_WAVETABLE_NAME_LEN + 1);
	}

	set_channels(mused.song.num_channels);
	
	if(mused.song.use_old_filter)
	{
		mused.cyd.flags |= CYD_USE_OLD_FILTER;
		debug("Using old filter!");
	}
	
	else
	{
		mused.cyd.flags &= ~CYD_USE_OLD_FILTER;
		debug("Using new filter!");
	}

	return 1;
}

int save_instrument(SDL_RWops *f)
{
	const Uint8 version = MUS_VERSION;

	SDL_RWwrite(f, MUS_INST_SIG, strlen(MUS_INST_SIG), sizeof(MUS_INST_SIG[0]));

	SDL_RWwrite(f, &version, 1, sizeof(version));

	save_instrument_inner(f, &mused.song.instrument[mused.current_instrument], &mused.mus.cyd->wavetable_entries[mused.song.instrument[mused.current_instrument].wavetable_entry], &mused.mus.cyd->wavetable_entries[mused.song.instrument[mused.current_instrument].fm_wave]);

	return 1;
}


int save_fx(SDL_RWops *f)
{
	const Uint8 version = MUS_VERSION;

	SDL_RWwrite(f, MUS_FX_SIG, strlen(MUS_FX_SIG), sizeof(MUS_FX_SIG[0]));

	SDL_RWwrite(f, &version, 1, sizeof(version));

	save_fx_inner(f, &mused.song.fx[mused.fx_bus]);

	return 1;
}

void save_wavepatch_inner(SDL_RWops *f, WgSettings *settings) //wasn't there
{
	SDL_RWwrite(f, &settings->num_oscs, sizeof(settings->num_oscs), 1);
	SDL_RWwrite(f, &settings->length, sizeof(settings->length), 1);
	
	int num_osc_value = *(&settings->num_oscs);
	
	for(int i = 0; i < num_osc_value; i++)
	{
		SDL_RWwrite(f, &settings->chain[i].osc, sizeof(settings->chain[i].osc), 1);
		SDL_RWwrite(f, &settings->chain[i].op, sizeof(settings->chain[i].op), 1);
		SDL_RWwrite(f, &settings->chain[i].mult, sizeof(settings->chain[i].mult), 1);
		SDL_RWwrite(f, &settings->chain[i].shift, sizeof(settings->chain[i].shift), 1);
		SDL_RWwrite(f, &settings->chain[i].exp, sizeof(settings->chain[i].exp), 1);
		SDL_RWwrite(f, &settings->chain[i].vol, sizeof(settings->chain[i].vol), 1);
		SDL_RWwrite(f, &settings->chain[i].exp_c, sizeof(settings->chain[i].exp_c), 1);
		SDL_RWwrite(f, &settings->chain[i].flags, sizeof(settings->chain[i].flags), 1);
	}
}

int save_wavepatch(SDL_RWops *f) //wasn't there
{
	const Uint8 version = MUS_VERSION;
	
	SDL_RWwrite(f, MUS_WAVEGEN_PATCH_SIG, strlen(MUS_WAVEGEN_PATCH_SIG), sizeof(MUS_WAVEGEN_PATCH_SIG[0]));
	
	SDL_RWwrite(f, &version, 1, sizeof(version));
	
	save_wavepatch_inner(f, &mused.wgset);
	
	return 1;
}

int open_wavepatch(FILE *f) //wasn't there
{
	if (!mus_load_wavepatch(f, &mused.wgset)) return 0;
	
	return 1;
}

int save_song_inner(SDL_RWops *f, SongStats *stats, bool confirm_save /* if no confirm save all, even unused */)
{
	bool kill_unused_things = false;

	Uint8 n_inst = mused.song.num_instruments;
	
	if(confirm_save)
	{
		if (!confirm(domain, mused.slider_bevel, &mused.largefont, "Save unused song elements?"))
		{
			//optimize_song(&mused.song); //wasn't there
			
			int maxpat = -1;
			
			for (int c = 0; c < mused.song.num_channels; ++c)
			{
				for (int i = 0; i < mused.song.num_sequences[c]; ++i)
					 if (maxpat < mused.song.sequence[c][i].pattern)
						maxpat = mused.song.sequence[c][i].pattern;
			}

			n_inst = 0;

			for (int i = 0; i <= maxpat; ++i)
				for (int s = 0; s < mused.song.pattern[i].num_steps; ++s)
					if (mused.song.pattern[i].step[s].instrument != MUS_NOTE_NO_INSTRUMENT)
						n_inst = my_max(n_inst, mused.song.pattern[i].step[s].instrument + 1);

			mused.song.num_patterns = maxpat + 1;

			kill_unused_things = true;
		}
	}
	
	SDL_RWwrite(f, MUS_SONG_SIG, strlen(MUS_SONG_SIG), sizeof(MUS_SONG_SIG[0]));

	const Uint8 version = MUS_VERSION;

	mused.song.time_signature = mused.time_signature;
	mused.song.sequence_step = mused.sequenceview_steps;

	SDL_RWwrite(f, &version, 1, sizeof(version));

	SDL_RWwrite(f, &mused.song.num_channels, 1, sizeof(mused.song.num_channels));
	Uint16 temp16 = mused.song.time_signature;
	FIX_ENDIAN(temp16);
	SDL_RWwrite(f, &temp16, 1, sizeof(mused.song.time_signature));
	temp16 = mused.song.sequence_step;
	FIX_ENDIAN(temp16);
	SDL_RWwrite(f, &temp16, 1, sizeof(mused.song.sequence_step));
	SDL_RWwrite(f, &n_inst, 1, sizeof(mused.song.num_instruments));
	temp16 = mused.song.num_patterns;
	FIX_ENDIAN(temp16);
	SDL_RWwrite(f, &temp16, 1, sizeof(mused.song.num_patterns));
	
	for (int i = 0; i < mused.song.num_channels; ++i)
	{
		temp16 = mused.song.num_sequences[i];
		FIX_ENDIAN(temp16);
		SDL_RWwrite(f, &temp16, 1, sizeof(mused.song.num_sequences[i]));
	}
	
	temp16 = mused.song.song_length;
	FIX_ENDIAN(temp16);
	SDL_RWwrite(f, &temp16, 1, sizeof(mused.song.song_length));
	temp16 = mused.song.loop_point;
	FIX_ENDIAN(temp16);
	SDL_RWwrite(f, &temp16, 1, sizeof(mused.song.loop_point));
	SDL_RWwrite(f, &mused.song.master_volume, 1, 1);
	SDL_RWwrite(f, &mused.song.song_speed, 1, sizeof(mused.song.song_speed));
	SDL_RWwrite(f, &mused.song.song_speed2, 1, sizeof(mused.song.song_speed2));
	
	Uint8 temp_rate = mused.song.song_rate & 0x00FF;
	SDL_RWwrite(f, &temp_rate, 1, sizeof(temp_rate));
	
	Uint32 temp32 = mused.song.flags;
	
	if(mused.song.num_patterns <= 0xff)
	{
		temp32 |= MUS_8_BIT_PATTERN_INDEX;
	}
	
	else
	{
		temp32 &= ~MUS_8_BIT_PATTERN_INDEX;
	}
	
	temp32 |= MUS_PATTERNS_NO_COMPRESSION | MUS_SEQ_NO_COMPRESSION;
	temp32 &= ~MUS_NO_REPEAT; //just in case
	
	if(mused.song.song_rate > 255)
	{
		temp32 |= MUS_16_BIT_RATE;
	}
	
	FIX_ENDIAN(temp32);
	SDL_RWwrite(f, &temp32, 1, sizeof(mused.song.flags));
	
	if(mused.song.song_rate > 255)
	{
		temp16 = mused.song.song_rate;
		FIX_ENDIAN(temp16);
		SDL_RWwrite(f, &temp16, 1, sizeof(mused.song.song_rate));
	}
	
	SDL_RWwrite(f, &mused.song.multiplex_period, 1, sizeof(mused.song.multiplex_period));
	SDL_RWwrite(f, &mused.song.pitch_inaccuracy, 1, sizeof(mused.song.pitch_inaccuracy));

	write_string8(f, mused.song.title);

	if (stats)
		stats->size[STATS_HEADER] = SDL_RWtell(f);

	Uint8 n_fx = kill_unused_things ? 0 : CYD_MAX_FX_CHANNELS;

	if (kill_unused_things)
	{
		for (int i = 0; i < CYD_MAX_FX_CHANNELS; ++i)
		{
			if (mused.song.fx[i].flags & (CYDFX_ENABLE_REVERB |	CYDFX_ENABLE_CRUSH | CYDFX_ENABLE_CHORUS))
				n_fx = my_max(n_fx, i + 1);
		}
	}

	SDL_RWwrite(f, &n_fx, 1, sizeof(n_fx));

	debug("Saving %d fx", n_fx);
	for (int fx = 0; fx < n_fx; ++fx)
	{
		save_fx_inner(f, &mused.song.fx[fx]);
	}

	if (stats)
		stats->size[STATS_FX] = SDL_RWtell(f);

	SDL_RWwrite(f, &mused.song.default_volume[0], sizeof(mused.song.default_volume[0]), mused.song.num_channels);
	SDL_RWwrite(f, &mused.song.default_panning[0], sizeof(mused.song.default_panning[0]), mused.song.num_channels);

	if (stats)
		stats->size[STATS_DEFVOLPAN] = SDL_RWtell(f);

	debug("Saving %d instruments", n_inst);
	for (int i = 0; i < n_inst; ++i)
	{
		save_instrument_inner(f, &mused.song.instrument[i], NULL, NULL);
	}

	if (stats)
		stats->size[STATS_INSTRUMENTS] = SDL_RWtell(f);

	bool *used_pattern = calloc(sizeof(bool), mused.song.num_patterns);

	for (int i = 0; i < mused.song.num_channels; ++i)
	{
		for (int s = 0; s < mused.song.num_sequences[i]; ++s)
		{
			temp16 = mused.song.sequence[i][s].position;
			FIX_ENDIAN(temp16);
			SDL_RWwrite(f, &temp16, 1, sizeof(temp16));

			used_pattern[mused.song.sequence[i][s].pattern] = true;

			//temp16 = mused.song.sequence[i][s].pattern;
			//FIX_ENDIAN(temp16);
			//SDL_RWwrite(f, &temp16, 1, sizeof(temp16));
			
			if(temp32 & MUS_8_BIT_PATTERN_INDEX)
			{
				Uint8 temp = (Uint8)(mused.song.sequence[i][s].pattern & 0xFF);
				SDL_RWwrite(f, &temp, 1, sizeof(temp));
			}
			
			else
			{
				temp16 = mused.song.sequence[i][s].pattern;
				FIX_ENDIAN(temp16);
				SDL_RWwrite(f, &temp16, 1, sizeof(temp16));
			}
			
			SDL_RWwrite(f, &mused.song.sequence[i][s].note_offset, 1, sizeof(mused.song.sequence[i][s].note_offset));
		}
	}

	if (stats)
		stats->size[STATS_SEQUENCE] = SDL_RWtell(f);

	for (int i = 0; i < mused.song.num_patterns; ++i)
	{
		write_packed_pattern(f, &mused.song.pattern[i], !used_pattern[i]);
	}

	if (stats)
		stats->size[STATS_PATTERNS] = SDL_RWtell(f);

	free(used_pattern);

	int max_wt = kill_unused_things ? 0 : CYD_WAVE_MAX_ENTRIES;

	for (int i = 0; i < CYD_WAVE_MAX_ENTRIES; ++i)
	{
		if (mused.mus.cyd->wavetable_entries[i].samples)
			max_wt = my_max(max_wt, i + 1);
	}

	FIX_ENDIAN(max_wt);

	SDL_RWwrite(f, &max_wt, 1, sizeof(Uint8));

	debug("Saving %d wavetable items", max_wt);

	for (int i = 0; i < max_wt; ++i)
	{
		write_wavetable_entry(f, &mused.mus.cyd->wavetable_entries[i], true);
	}

	if (stats)
		stats->size[STATS_WAVETABLE] = SDL_RWtell(f);

	for (int i = 0; i < max_wt; ++i)
	{
		write_string8(f, mused.song.wavetable_names[i]);
	}

	if (stats)
		stats->size[STATS_WAVETABLE_NAMES] = SDL_RWtell(f);

	mused.song.num_patterns = NUM_PATTERNS;
	mused.song.num_instruments = NUM_INSTRUMENTS;

	if (stats)
	{
		for (int i = N_STATS - 1; i > 0; --i)
		{
			stats->size[i] -= stats->size[i - 1];
		}

		stats->total_size = 0;

		for (int i = 0; i < N_STATS; ++i)
		{
			stats->total_size += stats->size[i];
		}
	}

	return 1;
}


int open_wavetable(FILE *f)
{
	Wave *w = wave_load(f);

	if (w)
	{
    // http://soundfile.sapp.org/doc/WaveFormat/ says: "8-bit samples are stored as unsigned bytes, ranging from 0 to 255.
    // 16-bit samples are stored as 2's-complement signed integers, ranging from -32768 to 32767."

		cyd_wave_entry_init(&mused.mus.cyd->wavetable_entries[mused.selected_wavetable], w->data, w->length, w->bits_per_sample == 16 ? CYD_WAVE_TYPE_SINT16 : CYD_WAVE_TYPE_UINT8, w->channels, 1, 1);

		mused.mus.cyd->wavetable_entries[mused.selected_wavetable].flags = 0;
		mused.mus.cyd->wavetable_entries[mused.selected_wavetable].sample_rate = w->sample_rate;
		mused.mus.cyd->wavetable_entries[mused.selected_wavetable].base_note = MIDDLE_C << 8;

		wave_destroy(w);

		invalidate_wavetable_view();

		return 1;
	}

	return 0;
}


static int open_wavetable_raw_inner(FILE *f, int t)
{
	size_t pos = ftell(f);
	fseek(f, 0, SEEK_END);
	size_t s = ftell(f) - pos;
	fseek(f, pos, SEEK_SET);

	if (s > 0)
	{
		Sint8 *w = calloc(s, sizeof(Sint8));

		if (w)
		{
			fread(w, 1, s, f);

			cyd_wave_entry_init(&mused.mus.cyd->wavetable_entries[mused.selected_wavetable], w, s, t, 1, 1, 1);

			mused.mus.cyd->wavetable_entries[mused.selected_wavetable].flags = 0;
			mused.mus.cyd->wavetable_entries[mused.selected_wavetable].sample_rate = 8000;
			mused.mus.cyd->wavetable_entries[mused.selected_wavetable].base_note = MIDDLE_C << 8;

			free(w);

			invalidate_wavetable_view();

			return 1;
		}
	}

	return 0;
}


int open_wavetable_raw(FILE *f)
{
	return open_wavetable_raw_inner(f, CYD_WAVE_TYPE_SINT8);
}


int open_wavetable_raw_u(FILE *f)
{
	return open_wavetable_raw_inner(f, CYD_WAVE_TYPE_UINT8);
}


int open_instrument(FILE *f)
{
	if (!mus_load_instrument_file2(f, &mused.song.instrument[mused.current_instrument], mused.mus.cyd->wavetable_entries)) return 0;

	mused.modified = true;

	invalidate_wavetable_view();

	return 1;
}


int open_fx(FILE *f)
{
	if (!mus_load_fx_file(f, &mused.song.fx[mused.fx_bus])) return 0;

	mused.modified = true;

	mus_set_fx(&mused.mus, &mused.song);

	return 1;
}


int save_song(SDL_RWops *ops, bool confirm_save /* if no confirm save all, even unused */)
{
	int r = save_song_inner(ops, NULL, confirm_save);

	mused.modified = false;

	return r;
}


int save_wavetable(FILE *ops)
{
	WaveWriter *ww = ww_create(ops, mused.mus.cyd->wavetable_entries[mused.selected_wavetable].sample_rate, 1);

	ww_write(ww, mused.mus.cyd->wavetable_entries[mused.selected_wavetable].data, mused.mus.cyd->wavetable_entries[mused.selected_wavetable].samples);

	ww_finish(ww);
	return 1;
}


void open_data(void *type, void *action, void *_ret)
{
	int t = CASTPTR(int, type);
	int a = CASTPTR(int, action);
	int *ret = _ret;

	if (a == OD_A_OPEN && t == OD_T_SONG)
	{
		int r = -1;
		if (mused.modified) r = confirm_ync(domain, mused.slider_bevel, &mused.largefont, "Save song?");
		int ret_val;

		if (r == 0)
		{
			if (ret) *ret = 0;
			return;
		}

		if (r == 1)
		{
			stop(0, 0, 0); // so loop positions set by pattern loop mode will be restored
			open_data(MAKEPTR(OD_T_SONG), MAKEPTR(OD_A_SAVE), &ret_val);
			if (!ret_val)
			{
				if (ret) *ret = 0;
				return;
			}
		}
	}

	const struct
	{
		const char *name, *ext;
		int (*open)(FILE *);
		int (*save)(SDL_RWops *);
	} open_stuff[] = {
		{ "song", "kt", open_song, /* save_song */ NULL },
		{ "instrument", "ki", open_instrument, save_instrument },
		{ "wave", "wav", open_wavetable, NULL },
		{ "raw signed", "", open_wavetable_raw, NULL },
		{ "raw unsigned", "", open_wavetable_raw_u, NULL },
		{ "FX bus", "kx", open_fx, save_fx },
		{ "wavegen patch", "kw", open_wavepatch, save_wavepatch }, //wasn't there
	};

	const char *mode[] = { "rb", "wb" };
	const char *modename[] = { "Open", "Save" };
	char str[1000];
	snprintf(str, sizeof(str), "%s %s", modename[a], open_stuff[t].name);

	stop(0, 0, 0);

	char _def[1024] = "";
	char *def = NULL;

	if (a == OD_A_SAVE)
	{
		switch (t)
		{
			case OD_T_INSTRUMENT:
			{
				snprintf(_def, sizeof(_def), "%s.ki", mused.song.instrument[mused.current_instrument].name);
			}
			break;

			case OD_T_FX:
			{
				snprintf(_def, sizeof(_def), "%s.kx", mused.song.fx[mused.fx_bus].name);
			}
			break;

			case OD_T_SONG:
			{
				if (strlen(mused.previous_song_filename) == 0)
				{
					snprintf(_def, sizeof(_def), "%s.kt", mused.song.title);
				}
				else
				{
					strncpy(_def, mused.previous_song_filename, sizeof(_def));
				}
			}
			break;
			
			case OD_T_WAVEGEN_PATCH: //wasn't there
			{
				snprintf(_def, sizeof(_def), "%s.kw", "Wavegen patch");
			}
			break;
		}

		def = _def;
	}

	char filename[5000], previous_cwd[5000];
	FILE *f = NULL;
	SDL_RWops *rw = NULL;

	// Save current cwd
	getcwd(previous_cwd, sizeof(previous_cwd));

	if (mused.previous_filebox_path[t][0])
		chdir(mused.previous_filebox_path[t]);

	if (open_dialog_fn(mode[a], str, open_stuff[t].ext, domain, mused.slider_bevel, &mused.largefont, &mused.smallfont, def, filename, sizeof(filename)))
	{
		getcwd(mused.previous_filebox_path[t], sizeof(mused.previous_filebox_path[t]));

		if (!(mused.flags & DISABLE_BACKUPS) && a == OD_A_SAVE && !create_backup(filename))
			warning("Could not create backup for %s", filename);

		if (a == OD_A_SAVE && t == OD_T_SONG)
			strncpy(mused.previous_song_filename, filename, sizeof(mused.previous_song_filename) - 1);

		f = fopen(filename, mode[a]);
		
		if (!f)
		{
			msgbox(domain, mused.slider_bevel, &mused.largefont, "Could not open file", MB_OK);
		}
		
		else if (t == OD_T_SONG)
		{
			// Update recent files list if we are opening/saving a song

			char fullpath[6001] = {0};
			snprintf(fullpath, sizeof(fullpath) - 1, "%s/%s", mused.previous_filebox_path[t], filename);

			update_recent_files_list(fullpath);
		}
	}

	if (f)
	{
		int return_val = 1;
		void * tmp;

		if (a == 0)
			tmp = open_stuff[t].open;
		else
		{
			if(t != OD_T_SONG)
			{
				tmp = open_stuff[t].save;
			}
			
			else
			{
				tmp = &save_song;
			}
		}
		
		if (tmp || ((t == OD_T_WAVETABLE) && (a == 1)))
		{
			cyd_lock(&mused.cyd, 1);
			int r;
			
			if (a == 0)
				r = open_stuff[t].open(f);
			else
			{
				if (t != OD_T_WAVETABLE)
				{
					rw = create_memwriter(f);
					
					if(t != OD_T_SONG)
					{
						r = open_stuff[t].save(rw);
					}
					
					else
					{
						r = save_song(rw, true);
					}
				}
				
				else
				{
					debug("save wave");
					
					r = save_wavetable(f);
				}
			}

			cyd_lock(&mused.cyd, 0);

			if (!r)
			{
				if(t == OD_T_SONG)
				{
					snprintf(str, sizeof(str), "              Could not open song!\nMaybe it is too new for this version of klystrack.", open_stuff[t].name);
				}
				
				else
				{
					snprintf(str, sizeof(str), "Could not open %s!", open_stuff[t].name);
				}
				
				msgbox(domain, mused.slider_bevel, &mused.largefont, str, MB_OK);

				return_val = 0;
			}
			else if (a == OD_A_OPEN && t != OD_T_SONG)
				mused.modified = true;
		}

		if (rw)
			SDL_RWclose(rw);

		fclose(f);

		if (ret) *ret = return_val;
	}
	else
		if (ret) *ret = 0;

	// Restore previous cwd
	chdir(previous_cwd);

	change_mode(mused.mode);
}