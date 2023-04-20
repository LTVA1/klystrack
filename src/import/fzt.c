#include "fzt.h"

Uint8 fzt_get_note(fzt_pattern_step* step)
{
    return (step->note & 0x7f);
}

Uint8 fzt_get_instrument(fzt_pattern_step* step)
{
    return ((step->note & 0x80) >> 3) | ((step->inst_vol & 0xf0) >> 4);
}

Uint8 fzt_get_volume(fzt_pattern_step* step)
{
    return (step->inst_vol & 0xf) | ((step->command & 0x8000) >> 11);
}

Uint16 fzt_get_command(fzt_pattern_step* step)
{
    return (step->command & 0x7fff);
}

Uint8 convert_fzt_envelope_params(Uint8 fzt_env_param)
{
	double min_delta = 1000000.0;
	Uint8 min_index = 0;
	
	for(Uint8 i = 0; i < 0x3f; i++)
	{
		double delta = fabs(envelope_length(i) - fzt_envelope_length(fzt_env_param));
		
		if(delta < min_delta)
		{
			min_delta = delta;
			min_index = i;
		}
	}
	
	return min_index;
}

void set_default_fzt_instrument(fzt_instrument* inst)
{
	memset(inst, 0, sizeof(fzt_instrument));

    inst->flags = FZT_TE_SET_CUTOFF | FZT_TE_SET_PW | FZT_TE_ENABLE_VIBRATO;
    inst->sound_engine_flags = FZT_SE_ENABLE_KEYDOWN_SYNC;

    inst->base_note = FZT_MIDDLE_C;

    inst->waveform = FZT_SE_WAVEFORM_PULSE;
    inst->pw = 0x80;

    inst->adsr.a = 0x4;
    inst->adsr.d = 0x28;
    inst->adsr.volume = 0x80;

    inst->filter_type = FZT_FIL_OUTPUT_LOWPASS;
    inst->filter_cutoff = 0xff;

    inst->program_period = 1;

    for(int i = 0; i < FZT_INST_PROG_LEN; i++) 
	{
        inst->program[i] = FZT_TE_PROGRAM_NOP;
    }

    inst->vibrato_speed = 0x60;
    inst->vibrato_depth = 0x20;
    inst->vibrato_delay = 0x20;
}

void load_fzt_instrument(FILE* f, fzt_instrument* inst, Uint8 version)
{
	fread(inst->name, 1, sizeof(inst->name), f);
	fread(&inst->waveform, 1, sizeof(inst->waveform), f);
	fread(&inst->flags, 1, sizeof(inst->flags), f);
	fread(&inst->sound_engine_flags, 1, sizeof(inst->sound_engine_flags), f);
	fread(&inst->base_note, 1, sizeof(inst->base_note), f);
	fread(&inst->finetune, 1, sizeof(inst->finetune), f);
	fread(&inst->slide_speed, 1, sizeof(inst->slide_speed), f);
	fread(&inst->adsr, 1, sizeof(inst->adsr), f);
	fread(&inst->pw, 1, sizeof(inst->pw), f);

    if(inst->sound_engine_flags & FZT_SE_ENABLE_RING_MOD)
	{
		fread(&inst->ring_mod, 1, sizeof(inst->ring_mod), f);
    }

    if(inst->sound_engine_flags & FZT_SE_ENABLE_HARD_SYNC)
	{
		fread(&inst->hard_sync, 1, sizeof(inst->hard_sync), f);
    }

    Uint8 progsteps = 0;
	
	fread(&progsteps, 1, sizeof(progsteps), f);

    if(progsteps > 0)
	{
		fread(&inst->program[0], 1, (int)progsteps * sizeof(inst->program[0]), f);
    }
	
	fread(&inst->program_period, 1, sizeof(inst->program_period), f);

    if(inst->flags & FZT_TE_ENABLE_VIBRATO)
	{
		fread(&inst->vibrato_speed, 1, sizeof(inst->vibrato_speed), f);
		fread(&inst->vibrato_depth, 1, sizeof(inst->vibrato_depth), f);
		fread(&inst->vibrato_delay, 1, sizeof(inst->vibrato_delay), f);
    }

    if(inst->flags & FZT_TE_ENABLE_PWM)
	{
		fread(&inst->pwm_speed, 1, sizeof(inst->pwm_speed), f);
		fread(&inst->pwm_depth, 1, sizeof(inst->pwm_depth), f);
		fread(&inst->pwm_delay, 1, sizeof(inst->pwm_delay), f);
    }

    if(inst->sound_engine_flags & FZT_SE_ENABLE_FILTER)
	{
		fread(&inst->filter_cutoff, 1, sizeof(inst->filter_cutoff), f);
		fread(&inst->filter_resonance, 1, sizeof(inst->filter_resonance), f);
		fread(&inst->filter_type, 1, sizeof(inst->filter_type), f);
    }
}

Uint16 convert_command(Uint16 fzt_command, bool in_program)
{
	switch(fzt_command & 0x7f00) 
	{
		case FZT_TE_EFFECT_ARPEGGIO: 
		{
			return MUS_FX_ARPEGGIO | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_PORTAMENTO_UP:
		{
			return MUS_FX_PORTA_UP | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_PORTAMENTO_DOWN: 
		{
			return MUS_FX_PORTA_DN | (fzt_command & 0xff);
			break;
		}
		
		case FZT_TE_EFFECT_SLIDE:
		{
			return (((fzt_command & 0xff) >= 0x3f) ? (MUS_FX_FAST_SLIDE | ((fzt_command & 0xff) >> 2)) : (MUS_FX_SLIDE | ((fzt_command & 0xff) << 2)));
			break;
		}

		case FZT_TE_EFFECT_VIBRATO: 
		{
			return MUS_FX_VIBRATO | ((((fzt_command & 0xf0) >> 4) / 2) << 4) | (fzt_command & 0xf);
			//return MUS_FX_VIBRATO | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_PWM:
		{
			return MUS_FX_PWM | ((((fzt_command & 0xf0) >> 4) / 2) << 4) | (fzt_command & 0xf);
			break;
		}

		case FZT_TE_EFFECT_SET_PW:
		{
			return MUS_FX_PW_SET | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_PW_UP:
		{
			return MUS_FX_PW_UP | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_PW_DOWN:
		{
			return MUS_FX_PW_DN | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_SET_CUTOFF:
		{
			return MUS_FX_CUTOFF_SET | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_VOLUME_FADE:
		{
			return MUS_FX_FADE_VOLUME | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_SET_WAVEFORM:
		{
			return MUS_FX_SET_WAVEFORM | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_SET_VOLUME: 
		{
			//return MUS_FX_SET_VOLUME | (fzt_command & 0xff);
			return MUS_FX_SET_ABSOLUTE_VOLUME | (fzt_command & 0xff);
			break;
		}
		
		case FZT_TE_EFFECT_SKIP_PATTERN: 
		{
			return MUS_FX_SKIP_PATTERN | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_EXT:
		{
			switch(fzt_command & 0x7ff0) 
			{
				case FZT_TE_EFFECT_EXT_TOGGLE_FILTER: 
				{
					//return (in_program ? MUS_FX_NOP : 0); //not supported in klystrack yet
					return MUS_FX_EXT_TOGGLE_FILTER | (fzt_command & 0xf);
					break;
				}

				case FZT_TE_EFFECT_EXT_PORTA_DN:
				{
					return MUS_FX_EXT_FINE_PORTA_DN | (fzt_command & 0xf);
					break;
				}

				case FZT_TE_EFFECT_EXT_PORTA_UP:
				{
					return MUS_FX_EXT_FINE_PORTA_UP | (fzt_command & 0xf);
					break;
				}

				case FZT_TE_EFFECT_EXT_FILTER_MODE:
				{
					return MUS_FX_FILTER_TYPE | (fzt_command & 0xf);
					break;
				}

				case FZT_TE_EFFECT_EXT_RETRIGGER:
				{
					return MUS_FX_EXT_RETRIGGER | (fzt_command & 0xf);
					break;
				}

				case FZT_TE_EFFECT_EXT_FINE_VOLUME_DOWN:
				{
					return MUS_FX_EXT_FADE_VOLUME_DN | (fzt_command & 0xf);
					break;
				}

				case FZT_TE_EFFECT_EXT_FINE_VOLUME_UP:
				{
					return MUS_FX_EXT_FADE_VOLUME_UP | (fzt_command & 0xf);
					break;
				}

				case FZT_TE_EFFECT_EXT_NOTE_CUT:
				{
					return MUS_FX_EXT_NOTE_CUT | (fzt_command & 0xf);
					break;
				}

				case FZT_TE_EFFECT_EXT_PHASE_RESET:
				{
					return MUS_FX_PHASE_RESET | (fzt_command & 0xf);
					break;
				}
				
				case FZT_TE_EFFECT_EXT_PATTERN_LOOP:
				{
					return MUS_FX_FT2_PATTERN_LOOP | (fzt_command & 0xf);
					break;
				}
			}

			break;
		}

		case FZT_TE_EFFECT_SET_SPEED_PROG_PERIOD:
		{
			return MUS_FX_SET_SPEED | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_CUTOFF_UP:
		{
			return MUS_FX_CUTOFF_UP | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_CUTOFF_DOWN:
		{
			return MUS_FX_CUTOFF_UP | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_SET_RESONANCE:
		{
			return MUS_FX_RESONANCE_SET | ((fzt_command & 0xff) >> 4);
			break;
		}

		case FZT_TE_EFFECT_RESONANCE_UP:
		{
			return (in_program ? MUS_FX_NOP : 0); //not supported in klystrack yet
			break;
		}

		case FZT_TE_EFFECT_RESONANCE_DOWN:
		{
			return (in_program ? MUS_FX_NOP : 0); //not supported in klystrack yet
			break;
		}

		case FZT_TE_EFFECT_SET_RING_MOD_SRC:
		{
			return MUS_FX_SET_RINGSRC | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_SET_HARD_SYNC_SRC:
		{
			return MUS_FX_SET_SYNCSRC | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_SET_ATTACK:
		{
			return MUS_FX_SET_ATTACK_RATE | convert_fzt_envelope_params(fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_SET_DECAY:
		{
			return MUS_FX_SET_DECAY_RATE | convert_fzt_envelope_params(fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_SET_SUSTAIN:
		{
			return MUS_FX_SET_SUSTAIN_LEVEL | ((fzt_command & 0xff) / 8);
			break;
		}

		case FZT_TE_EFFECT_SET_RELEASE:
		{
			return MUS_FX_SET_RELEASE_RATE | convert_fzt_envelope_params(fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_PROGRAM_RESTART:
		{
			return MUS_FX_RESTART_PROGRAM | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_PORTA_UP_SEMITONE:
		{
			return MUS_FX_PORTA_UP_SEMI | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_PORTA_DOWN_SEMITONE:
		{
			return MUS_FX_PORTA_DN_SEMI | (fzt_command & 0xff);
			break;
		}

		case FZT_TE_EFFECT_ARPEGGIO_ABS:
		{
			return MUS_FX_ARPEGGIO_ABS | (fzt_command & 0xff);
			break;
		}
		
		case FZT_TE_EFFECT_LEGATO:
		{
			return (in_program ? MUS_FX_NOP : 0); //in klystrack you use legato control bit
			break;
		}

		case FZT_TE_EFFECT_TRIGGER_RELEASE:
		{
			return MUS_FX_TRIGGER_RELEASE | (fzt_command & 0xff);
			break;
		}

		default: return (in_program ? MUS_FX_NOP : 0); break;
		
		// THESE ARE USED ONLY IN INSTRUMENT PROGRAM
		
		case FZT_TE_PROGRAM_LOOP_BEGIN:
		{
			return MUS_FX_LABEL | (fzt_command & 0xff);
			break;
		}
		
		case FZT_TE_PROGRAM_LOOP_END:
		{
			return MUS_FX_LOOP | (fzt_command & 0xff);
			break;
		}
		
		case FZT_TE_PROGRAM_JUMP:
		{
			return MUS_FX_JUMP | (fzt_command & 0xff);
			break;
		}
	}
	
	switch(fzt_command & 0x7fff)
	{
		case FZT_TE_PROGRAM_NOP:
		{
			return MUS_FX_NOP;
			break;
		}
		
		case FZT_TE_PROGRAM_END:
		{
			return MUS_FX_END;
			break;
		}
		
		default: return (in_program ? MUS_FX_NOP : 0); break;
	}
}


void convert_fzt_pattern(MusPattern* pattern, fzt_pattern* fzt_pat, Uint16 pattern_length)
{
	for(int i = 0; i < pattern_length; i++)
	{
		MusStep* klystrack_step = &pattern->step[i];
		fzt_pattern_step* fzt_step = &fzt_pat->step[i];
		
		Uint8 note = fzt_get_note(fzt_step);
		
		if(note == FZT_MUS_NOTE_NONE)
		{
			klystrack_step->note = MUS_NOTE_NONE;
		}
		
		else if(note == FZT_MUS_NOTE_CUT)
		{
			klystrack_step->note = MUS_NOTE_CUT;
		}
		
		else if(note == FZT_MUS_NOTE_RELEASE)
		{
			klystrack_step->note = MUS_NOTE_RELEASE;
		}
		
		else
		{
			klystrack_step->note = note + C_ZERO;
		}
		
		klystrack_step->instrument = ((fzt_get_instrument(fzt_step) == FZT_MUS_NOTE_INSTRUMENT_NONE) ? MUS_NOTE_NO_INSTRUMENT : fzt_get_instrument(fzt_step));
		klystrack_step->volume = ((fzt_get_volume(fzt_step) == FZT_MUS_NOTE_VOLUME_NONE) ? MUS_NOTE_NO_VOLUME : (fzt_get_volume(fzt_step) * 4));
		//klystrack_step->command[0] = fzt_get_command(fzt_step);
		
		Uint16 command = fzt_get_command(fzt_step);
		
		if((command & 0x7f00) == FZT_TE_EFFECT_VIBRATO)
		{
			klystrack_step->ctrl |= MUS_CTRL_VIB;
		}
		
		else if((command & 0x7f00) == FZT_TE_EFFECT_LEGATO)
		{
			klystrack_step->ctrl |= MUS_CTRL_LEGATO;
		}
		
		klystrack_step->command[0] = convert_command(fzt_get_command(fzt_step), false);
	}
}


void convert_fzt_instrument(MusInstrument* inst, fzt_instrument* fzt_inst)
{
	inst->flags &= ~(MUS_INST_DRUM);
	inst->flags |= MUS_INST_RELATIVE_VOLUME | MUS_INST_KEEP_VOLUME_ON_SLIDE_AND_LEGATO;
	inst->cydflags &= ~(CYD_CHN_ENABLE_TRIANGLE);
	
	if(fzt_inst->waveform & FZT_SE_WAVEFORM_NOISE)
	{
		inst->cydflags |= CYD_CHN_ENABLE_NOISE;
	}
	
	if(fzt_inst->waveform & FZT_SE_WAVEFORM_PULSE)
	{
		inst->cydflags |= CYD_CHN_ENABLE_PULSE;
	}
	
	if(fzt_inst->waveform & FZT_SE_WAVEFORM_TRIANGLE)
	{
		inst->cydflags |= CYD_CHN_ENABLE_TRIANGLE;
	}
	
	if(fzt_inst->waveform & FZT_SE_WAVEFORM_SAW)
	{
		inst->cydflags |= CYD_CHN_ENABLE_SAW;
	}
	
	if(fzt_inst->waveform & FZT_SE_WAVEFORM_NOISE_METAL)
	{
		inst->cydflags |= CYD_CHN_ENABLE_METAL;
	}
	
	if(fzt_inst->waveform & FZT_SE_WAVEFORM_SINE)
	{
		inst->cydflags |= CYD_CHN_ENABLE_SINE;
	}
	
	inst->vibrato_speed = fzt_inst->vibrato_speed / 8;
	inst->vibrato_depth = fzt_inst->vibrato_depth;
	inst->vibrato_delay = fzt_inst->vibrato_delay;
	
	inst->vibrato_shape = MUS_SHAPE_TRI_UP;
	
	inst->pwm_speed = fzt_inst->pwm_speed / 8;
	inst->pwm_depth = fzt_inst->pwm_depth;
	inst->pwm_delay = fzt_inst->pwm_delay;
	
	inst->pwm_shape = MUS_SHAPE_TRI_UP;
	
	if(fzt_inst->flags & FZT_TE_PROG_NO_RESTART)
	{
		inst->flags |= MUS_INST_NO_PROG_RESTART;
	}
	
	if(fzt_inst->flags & FZT_TE_SET_CUTOFF)
	{
		inst->flags |= MUS_INST_SET_CUTOFF;
	}
	
	else
	{
		inst->flags &= ~(MUS_INST_SET_CUTOFF);
	}
	
	if(fzt_inst->flags & FZT_TE_SET_PW)
	{
		inst->flags |= MUS_INST_SET_PW;
	}
	
	else
	{
		inst->flags &= ~(FZT_TE_SET_PW);
	}
	
	if(fzt_inst->sound_engine_flags & FZT_SE_ENABLE_FILTER)
	{
		inst->cydflags |= CYD_CHN_ENABLE_FILTER;
	}
	
	inst->cutoff = (Uint16)fzt_inst->filter_cutoff << 4;
	inst->resonance = fzt_inst->filter_resonance >> 4;
	
	switch(fzt_inst->filter_type)
	{
		case FZT_FIL_OUTPUT_LOWPASS: inst->flttype = FLT_LP; break;
		case FZT_FIL_OUTPUT_HIGHPASS: inst->flttype = FLT_HP; break;
		case FZT_FIL_OUTPUT_BANDPASS: inst->flttype = FLT_BP; break;
		case FZT_FIL_OUTPUT_LOW_HIGH: inst->flttype = FLT_LH; break;
		case FZT_FIL_OUTPUT_HIGH_BAND: inst->flttype = FLT_LB; break;
		case FZT_FIL_OUTPUT_LOW_BAND: inst->flttype = FLT_LB; break;
		case FZT_FIL_OUTPUT_LOW_HIGH_BAND: inst->flttype = FLT_ALL; break;
		
		default: break;
	}
	
	if(fzt_inst->sound_engine_flags & FZT_SE_ENABLE_RING_MOD)
	{
		inst->cydflags |= CYD_CHN_ENABLE_RING_MODULATION;
	}
	
	inst->ring_mod = fzt_inst->ring_mod;
	
	if(fzt_inst->sound_engine_flags & FZT_SE_ENABLE_HARD_SYNC)
	{
		inst->cydflags |= CYD_CHN_ENABLE_SYNC;
	}
	
	inst->sync_source = fzt_inst->hard_sync;
	
	if(fzt_inst->sound_engine_flags & FZT_SE_ENABLE_KEYDOWN_SYNC)
	{
		inst->cydflags |= CYD_CHN_ENABLE_KEY_SYNC;
	}
	
	else
	{
		inst->cydflags &= ~(CYD_CHN_ENABLE_KEY_SYNC);
	}
	
	inst->base_note = fzt_inst->base_note + C_ZERO;
	inst->finetune = fzt_inst->finetune;
	inst->slide_speed = (Uint16)fzt_inst->slide_speed << 2;
	
	inst->volume = fzt_inst->adsr.volume;
	inst->adsr.a = convert_fzt_envelope_params(fzt_inst->adsr.a);
	inst->adsr.d = convert_fzt_envelope_params(fzt_inst->adsr.d);
	inst->adsr.s = fzt_inst->adsr.s / 8;
	inst->adsr.r = convert_fzt_envelope_params(fzt_inst->adsr.r);
	
	inst->pw = (Uint16)fzt_inst->pw << 4;
	
	inst->prog_period[0] = fzt_inst->program_period;
	
	for(int i = 0; i < FZT_INST_PROG_LEN; i++)
	{
		inst->program[0][i] = convert_command(fzt_inst->program[i], true);
		
		if(fzt_inst->program[i] & 0x8000)
		{
			inst->program_unite_bits[0][i / 8] |= (1 << (i & 7)); 
		}
	}
}

int import_fzt(FILE *f)
{
	bool abort = false;
	
	struct 
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
	} header;
	
	fzt_sequence* sequence = NULL;
	fzt_pattern* pattern = NULL;
	
	fread(&header.sig[0], 1, sizeof(FZT_SONG_SIG) - 1, f);
	fread(&header.version, 1, sizeof(header.version), f);
	fread(&header.song_name[0], 1, FZT_SONG_NAME_LEN + 1, f);
	fread(&header.loop_start, 1, sizeof(header.loop_start), f);
	fread(&header.loop_end, 1, sizeof(header.loop_end), f);
	fread(&header.pattern_length, 1, sizeof(header.pattern_length), f);
	fread(&header.speed, 1, sizeof(header.speed), f);
	fread(&header.rate, 1, sizeof(header.rate), f);
	fread(&header.num_sequence_steps, 1, sizeof(header.num_sequence_steps), f);
	
	header.sig[sizeof(FZT_SONG_SIG) - 1] = '\0';
	header.song_name[FZT_SONG_NAME_LEN] = '\0';
	
	if(strcmp(header.sig, FZT_SONG_SIG) != 0)
	{
		debug("Wrong file signature: \"%s\"", header.sig);
		abort = true;
		goto abort;
	}
	
	if(header.version > FZT_VERSION)
	{
		debug("Module version is higher than the latest version klystrack can import.");
		abort = true;
		goto abort;
	}
	
	sequence = (fzt_sequence*)calloc(1, sizeof(fzt_sequence));
	
	pattern = (fzt_pattern*)calloc(1, sizeof(fzt_pattern));
	
	if(sequence == NULL || pattern == NULL)
	{
		debug("Failed to allocate memory.");
		abort = true;
		goto abort;
	}
	
	pattern->step = (fzt_pattern_step*)calloc(header.pattern_length, sizeof(fzt_pattern_step));
		
	if(pattern->step == NULL)
	{
		debug("Failed to allocate memory.");
		abort = true;
		goto abort;
	}
	
	int sequence_pos = 0;
	
	for(int i = 0; i < header.num_sequence_steps; i++) 
	{
        fread(&sequence->sequence_step[i], 1, sizeof(sequence->sequence_step[0]), f);
		
		for(int j = 0; j < FZT_SONG_MAX_CHANNELS; j++)
		{
			add_sequence(j, sequence_pos, sequence->sequence_step[i].pattern_indices[j], 0);
		}
		
		sequence_pos += header.pattern_length;
    }
	
	fread(&header.num_patterns, 1, sizeof(header.num_patterns), f);
	
	strncpy(mused.song.title, header.song_name, FZT_SONG_NAME_LEN + 1);
	
	mused.song.song_length = header.num_sequence_steps * header.pattern_length;
	mused.song.song_speed = mused.song.song_speed2 = header.speed;
	mused.song.song_rate = header.rate;
	mused.song.num_channels = FZT_SONG_MAX_CHANNELS;
	mused.sequenceview_steps = header.pattern_length;
	mused.song.num_patterns = header.num_patterns;
	mused.song.loop_point = header.loop_start * header.pattern_length;
	
	for(int i = 0; i < header.num_patterns; i++) 
	{
		for(int j = 0; j < header.pattern_length; j++)
		{
			fread(&pattern->step[j].note, 1, sizeof(pattern->step[j].note), f);
			fread(&pattern->step[j].inst_vol, 1, sizeof(pattern->step[j].inst_vol), f);
			fread(&pattern->step[j].command, 1, sizeof(pattern->step[j].command), f);
		}
		
		resize_pattern(&mused.song.pattern[i], header.pattern_length);
		
		convert_fzt_pattern(&mused.song.pattern[i], pattern, header.pattern_length);
    }
	
	fread(&header.num_instruments, 1, sizeof(header.num_instruments), f);
	
	fzt_instrument* inst = (fzt_instrument*)calloc(1, sizeof(fzt_instrument));
	
	for(int i = 0; i < header.num_instruments; i++)
	{
		set_default_fzt_instrument(inst);
		load_fzt_instrument(f, inst, header.version);
		
		mus_get_default_instrument(&mused.song.instrument[i]);
		
		strncpy(mused.song.instrument[i].name, inst->name, FZT_MUS_INST_NAME_LEN + 1);
		
		convert_fzt_instrument(&mused.song.instrument[i], inst);
	}
	
	for(int ch = 0; ch < FZT_SONG_MAX_CHANNELS; ch++) //filling in vibrato control bits between 0x04xy (begin) and 0x0400 (end); should work even across subsequent patterns
	{
		bool is_vibrato = false;
		
		for(int i = 0; i < header.num_sequence_steps; i++)
		{
			MusPattern* klystrack_pattern = &mused.song.pattern[mused.song.sequence[ch][i].pattern];
			
			for(int j = 0; j < header.pattern_length; j++)
			{
				MusStep* step = &klystrack_pattern->step[j];
				
				if((step->command[0] & 0xff00) == MUS_FX_VIBRATO)
				{
					if((step->command[0] & 0xff))
					{
						is_vibrato = true;
					}
					
					else
					{
						is_vibrato = false;
					}
				}
				
				if(step->note != MUS_NOTE_NONE && step->note != MUS_NOTE_RELEASE && step->note != MUS_NOTE_CUT && step->instrument != MUS_NOTE_NO_INSTRUMENT)
				{
					is_vibrato = false;
				}
				
				if(is_vibrato)
				{
					step->ctrl |= MUS_CTRL_VIB;
				}
			}
		}
	}
	
	abort:;
	
	if(sequence)
	{
		free(sequence);
	}
	
	if(pattern)
	{
		if(pattern->step)
		{
			free(pattern->step);
		}
		
		free(pattern);
	}
	
	if(inst)
	{
		free(inst);
	}
	
	if(abort)
	{
		debug("Aborting import.");
		return 0;
	}
	
	else
	{
		debug("Import successful.");
		return 1;
	}
}