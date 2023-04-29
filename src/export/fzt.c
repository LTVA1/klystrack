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

#include "fzt.h"
#include "../import/fzt.h"

#include "export.h"
#include "gui/bevel.h"
#include "snd/cyd.h"
#include "macros.h"
#include "mused.h"
#include "gfx/gfx.h"
#include "gui/view.h"
#include "mybevdefs.h"
#include "gfx/font.h"
#include "theme.h"
#include <string.h>

extern GfxDomain *domain;
extern Mused mused;

void fzt_set_note(fzt_pattern_step* step, uint8_t note)
{
	step->note &= 0x80;
	step->note |= (note & 0x7f);
}

void fzt_set_instrument(fzt_pattern_step* step, uint8_t inst)
{
	step->note &= 0x7f;
	step->inst_vol &= 0x0f;

	step->note |= ((inst & 0x10) << 3);
	step->inst_vol |= ((inst & 0xf) << 4);
}

void fzt_set_volume(fzt_pattern_step* step, uint8_t vol)
{
	step->command &= 0x7fff;
	step->inst_vol &= 0xf0;

	step->command |= ((vol & 0x10) << 11);
	step->inst_vol |= (vol & 0xf);
}

void fzt_set_command(fzt_pattern_step* step, uint16_t command)
{
	step->command &= 0x8000;
	step->command |= command & (0x7fff);
}

bool redraw_progress(const char* bottom_string, int progress)
{
	bool abort = false;
	
	SDL_Rect area = {domain->screen_w / 2 - 150, domain->screen_h / 2 - 29, 300, 48 + 10};
	bevel(domain, &area, mused.slider_bevel, BEV_MENU);
	
	adjust_rect(&area, 8);
	area.h = 16;
	
	bevel(domain, &area, mused.slider_bevel, BEV_FIELD);
	
	adjust_rect(&area, 2);
	
	int t = area.w;
	area.w = area.w * progress / 100;
	
	gfx_rect(domain, &area, colors[COLOR_PROGRESS_BAR]);
	
	area.y += 16 + 4;
	area.w = t;
	
	font_write_args(&mused.largefont, domain, &area, "Exporting... Press ESC to abort.");
	
	area.y += 10;
	
	font_write_args(&mused.largefont, domain, &area, bottom_string);
	
	SDL_Event e;
	
	while (SDL_PollEvent(&e))
	{
		if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
		{
			abort = true;
		}
	}
	
	gfx_domain_flip(domain);
	
	return !abort;
}

bool check_instrument(MusInstrument* inst, Uint8 number)
{
	bool success = true;
	char buffer[200];
	
	if(inst->cydflags & CYD_CHN_ENABLE_LFSR)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses POKEY LFSR!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_YM_ENV)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses AY envelope!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_VOLUME_KEY_SCALING)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses volume key scaling!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_FX)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses FX bus!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_FM)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses FM synth!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_FIXED_NOISE_PITCH)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses fixed noise pitch!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_EXPONENTIAL_VOLUME)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses exponential volume!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_EXPONENTIAL_ATTACK)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses exponential attack!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_EXPONENTIAL_DECAY)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses exponential decay!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_EXPONENTIAL_RELEASE)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses exponential release!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_ENVELOPE_KEY_SCALING)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses envelope key scaling!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_AY8930_BUZZ_MODE)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses AY8930 envelope mode!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->cydflags & CYD_CHN_ENABLE_1_BIT_NOISE)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses 1-bit noise mode!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    // check musflags

    if(inst->flags & MUS_INST_DRUM)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses drum flag!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->flags & MUS_INST_INVERT_TREMOLO_BIT)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X inverts tremolo bit!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->flags & MUS_INST_INVERT_VIBRATO_BIT)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X inverts vibrato bit!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->flags & MUS_INST_LOCK_NOTE)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses lock note flag!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->flags & MUS_INST_MULTIOSC)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses multiosc. mode!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->flags & MUS_INST_QUARTER_FREQ)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses quarter frequency flag!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->flags & MUS_INST_QUARTER_FREQ)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses quarter frequency flag!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->flags & MUS_INST_USE_PANNING_ENVELOPE)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses custom panning envelope!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->flags & MUS_INST_USE_VOLUME_ENVELOPE)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses custom volume envelope!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->flags & MUS_INST_YM_BUZZ)
    {
		snprintf(buffer, sizeof(buffer), "Instrument %02X uses AY envelope!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    //check how many macros inst has

    if(inst->num_macros > 1)
    {
        snprintf(buffer, sizeof(buffer), "Instrument %02X uses several instrument programs!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    //check how long is inst program

    Uint8 prog_len = 0;

    for(int i = 0; i < MUS_PROG_LEN; i++)
    {
        if(inst->program[0][i] != MUS_FX_NOP)
        {
            prog_len = i;
        }
    }

	if(prog_len > FZT_INST_PROG_LEN - 1)
    {
        snprintf(buffer, sizeof(buffer), "Instrument %02X program is too long!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->base_note > MIDDLE_C + 12 * 3 + 11 || inst->base_note < C_ZERO)
    {
        snprintf(buffer, sizeof(buffer), "Instrument %02X base note is out of FZT note range!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    if(inst->mixmode != 0)
    {
        snprintf(buffer, sizeof(buffer), "Instrument %02X oscillators' mixmode is not supported in FZT!", number);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

	return success;
}

bool check_song(MusSong* song)
{
    bool success = true;
    char buffer[200];

    if(song->num_channels > FZT_SONG_MAX_CHANNELS)
    {
        snprintf(buffer, sizeof(buffer), "Song has too many channels!");
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
		success = false;
        return success;
    }

    Uint16 num_seq = song->num_sequences[0];

    for(int i = 1; i < song->num_channels; i++)
    {
        if(song->num_sequences[i] != num_seq)
        {
            snprintf(buffer, sizeof(buffer), "Song has different number of pattern entries on different channels!");
            msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
            success = false;
            return success;
        }
    }

    if(num_seq > 0xff)
    {
        snprintf(buffer, sizeof(buffer), "Song has too many pattern entries in sequence!");
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
        success = false;
        return success;
    }

    if(song->num_sequences[0] > 1)
    {
        Uint16 seq_step = song->sequence[0][1].position - song->sequence[0][0].position;

        if(seq_step > 0xff)
        {
            snprintf(buffer, sizeof(buffer), "Song has very long distance between patterns!");
            msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
            success = false;
            return success;
        }

        for(int i = 0; i < song->num_channels; i++)
        {
            for(int j = 1; j < song->num_sequences[i]; j++)
            {
                if(song->sequence[i][j].position - song->sequence[i][j - 1].position != seq_step)
                {
                    snprintf(buffer, sizeof(buffer), "Song has uneven pattern spacing!");
                    msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
                    success = false;
                    return success;
                }
            }
        }
    }

    return success;
}

bool check_pattern(MusPattern* pat, Uint8 num)
{
    bool success = true;
    char buffer[200];
    
    if(pat == NULL)
    {
        snprintf(buffer, sizeof(buffer), "Pattern %02X is an invalid pointer!", num);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
        success = false;
        return success;
    }

    if(pat->step == NULL)
    {
        snprintf(buffer, sizeof(buffer), "Pattern %02X has NULL data pointer!", num);
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
        success = false;
        return success;
    }

    for(int i = 0; i < pat->num_steps; i++)
    {
        MusStep* step = &pat->step[i];

        if(step->ctrl & MUS_CTRL_TREM)
        {
            snprintf(buffer, sizeof(buffer), "Pattern %02X has tremolo control bit at step %02X!", num, i);
            msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
            success = false;
            return success;
        }

        if((step->note < C_ZERO || step->note > MIDDLE_C + 12 * 3 + 11) && step->note != MUS_NOTE_NONE && step->note != MUS_NOTE_RELEASE && step->note != MUS_NOTE_CUT)
        {
            snprintf(buffer, sizeof(buffer), "Pattern %02X note at step %02X is out of FZT note range!", num, i);
            msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
            success = false;
            return success;
        }

        for(int j = 1; j < MUS_MAX_COMMANDS; j++)
        {
            if(step->command[j] != 0)
            {
                snprintf(buffer, sizeof(buffer), "Pattern %02X has command on column %d at step %02X!", num, j + 1, i);
                msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
                success = false;
                return success;
            }
        }
    }

    return success;
}

bool do_checks(MusSong* song, CydWavetableEntry* wavetable_entries)
{
    bool success = true;
    char buffer[200];
	
	success = redraw_progress("", 0);
	
	if(!success)
	{
		goto abort;
	}

    int n_patterns = -1;

    for (int c = 0; c < song->num_channels; ++c)
    {
        for (int i = 0; i < song->num_sequences[c]; ++i)
        {
            if (n_patterns < song->sequence[c][i].pattern)
            {
                n_patterns = song->sequence[c][i].pattern;
            }
        }
    }

    success = redraw_progress("", 5);

    if(!success)
	{
		goto abort;
	}

    Uint8 n_inst = 0;

    for (int i = 0; i <= n_patterns; ++i)
    {
        for (int s = 0; s < song->pattern[i].num_steps; ++s)
        {
            if (song->pattern[i].step[s].instrument != MUS_NOTE_NO_INSTRUMENT)
            {
                n_inst = my_max(n_inst, song->pattern[i].step[s].instrument + 1);
            }
        }
    }

    n_patterns += 1;

    if(n_inst > FZT_MUS_NOTE_INSTRUMENT_NONE - 1)
    {
        snprintf(buffer, sizeof(buffer), "Song has too many instruments!");
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
        success = false;
        goto abort;
    }

    if(n_patterns > 0xff)
    {
        snprintf(buffer, sizeof(buffer), "Song has too many patterns!");
        msgbox(domain, mused.slider_bevel, &mused.largefont, buffer, MB_OK);
        success = false;
        goto abort;
    }
	
	for(int i = 0; i < n_inst; i++)
	{
		success = check_instrument(&song->instrument[i], i);

        if(!success)
		{
			goto abort;
		}
		
		snprintf(buffer, sizeof(buffer), "Checking instrument %02X", i);
		success = redraw_progress(buffer, 5 + i * 10 / n_inst);
		
		if(!success)
		{
			goto abort;
		}
	}

    snprintf(buffer, sizeof(buffer), "Checking song");
    success = redraw_progress(buffer, 5 + 10);

    if(!success)
    {
        goto abort;
    }

    success = check_song(song);

    if(!success)
    {
        goto abort;
    }

    for(int i = 0; i < n_patterns; i++)
    {
        success = check_pattern(&song->pattern[i], i);

        if(!success)
		{
			goto abort;
		}
		
		snprintf(buffer, sizeof(buffer), "Checking pattern %02X", i);
		success = redraw_progress(buffer, 20 + i * 20 / n_patterns);
		
		if(!success)
		{
			goto abort;
		}
    }

	abort:;

    return success;
}

bool export_fzt(MusSong* song, CydWavetableEntry* wavetable_entries, FILE *f)
{
	bool success = true;

    success = do_checks(song, wavetable_entries);

    if(!success)
    {
        goto abort;
    }
	
	abort:;

	fclose(f);
	
	return success;
}