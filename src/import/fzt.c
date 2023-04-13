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

void load_fzt_instrument(FILE *f, fzt_instrument* inst, Uint8 version)
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
		fread(&inst->program, 1, progsteps * sizeof(inst->program[0]), f);
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
		
		//klystrack_step->note = ((fzt_get_note(fzt_step) == FZT_MUS_NOTE_NONE) ? MUS_NOTE_NONE : fzt_get_note(fzt_step) + C_ZERO);
		klystrack_step->instrument = ((fzt_get_instrument(fzt_step) == FZT_MUS_NOTE_INSTRUMENT_NONE) ? MUS_NOTE_NO_INSTRUMENT : fzt_get_instrument(fzt_step));
		klystrack_step->volume = ((fzt_get_volume(fzt_step) == FZT_MUS_NOTE_VOLUME_NONE) ? MUS_NOTE_NO_VOLUME : (fzt_get_volume(fzt_step) * 4));
		klystrack_step->command[0] = fzt_get_command(fzt_step);
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
	fzt_pattern* patterns = NULL;
	fzt_instrument* instruments = NULL;
	
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
	
	sequence = (fzt_sequence*)malloc(sizeof(fzt_sequence));
	
	patterns = (fzt_pattern*)malloc(sizeof(fzt_pattern) * header.num_patterns);
	
	for(int i = 0; i < header.num_patterns; i++)
	{
		patterns[i].step = (fzt_pattern_step*)malloc(sizeof(fzt_pattern_step) * header.pattern_length);
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
        //fread(patterns[i].step, 1, sizeof(fzt_pattern_step) * header.pattern_length, f);
		
		for(int j = 0; j < header.pattern_length; j++)
		{
			fread(&patterns[i].step[j].note, 1, sizeof(patterns[i].step[j].note), f);
			fread(&patterns[i].step[j].inst_vol, 1, sizeof(patterns[i].step[j].inst_vol), f);
			fread(&patterns[i].step[j].command, 1, sizeof(patterns[i].step[j].command), f);
		}
		
		resize_pattern(&mused.song.pattern[i], header.pattern_length);
		
		convert_fzt_pattern(&mused.song.pattern[i], &patterns[i], header.pattern_length);
    }
	
	fread(&header.num_instruments, 1, sizeof(header.num_instruments), f);
	
	instruments = (fzt_instrument*)malloc(sizeof(fzt_instrument) * header.num_instruments);
	
	for(int i = 0; i < header.num_instruments; i++)
	{
		set_default_fzt_instrument(&instruments[i]);
		load_fzt_instrument(f, &instruments[i], header.version);
		
		strncpy(mused.song.instrument[i].name, instruments[i].name, FZT_MUS_INST_NAME_LEN + 1);
	}
	
	abort:;
	
	if(sequence)
	{
		free(sequence);
	}
	
	if(patterns)
	{
		for(int i = 0; i < header.num_patterns; i++)
		{
			if(patterns[i].step)
			{
				free(patterns[i].step);
			}
		}
		
		free(patterns);
	}
	
	if(instruments)
	{
		free(instruments);
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