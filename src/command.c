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

#include "command.h"
#include "snd/music.h"
#include "snd/freqs.h"
#include "macros.h"
#include <string.h>
#include "mused.h"
#include "view.h"

static const InstructionDesc instruction_desc[] =
{
	{MUS_FX_END, 0xffff, "Program end", "PrgEnd", 0, 0},
	{MUS_FX_NOP, 0xffff, "No operation", "Nop", 0, 0},
	{MUS_FX_JUMP, 0xff00, "Goto", NULL, -1, -1},
	{MUS_FX_LABEL, 0xff00, "Loop begin", "Begin", 0, 0},
	{MUS_FX_LOOP, 0xff00, "Loop end", "Loop", -1, -1},
	{MUS_FX_RELEASE_POINT, 0xff00, "Program jumps here when release is triggered", "\xf9 Release", 0, 0},
	
	{MUS_FX_ARPEGGIO, 0xff00, "Set arpeggio note", "Arp", -1, -1},
	{MUS_FX_ARPEGGIO_DOWN, 0xff00, "Set arpeggio note (but XX semitones down)", "ArpDown", -1, -1},
	{MUS_FX_SET_2ND_ARP_NOTE, 0xff00, "Set 2nd arpeggio note (for multiosc)", "Arp2nd", -1, -1},
	{MUS_FX_SET_3RD_ARP_NOTE, 0xff00, "Set 3rd arpeggio note (for multiosc)", "Arp3rd", -1, -1},
	
	{MUS_FX_SET_CSM_TIMER_NOTE, 0xff00, "Set CSM timer note", "CSMnote", -1, -1},
	{MUS_FX_SET_CSM_TIMER_FINETUNE, 0xff00, "Set CSM timer finetune", "CSMfine", -1, -1},
	{MUS_FX_CSM_TIMER_PORTA_UP, 0xff00, "CSM timer portamento up", "CSMportUp", -1, -1},
	{MUS_FX_CSM_TIMER_PORTA_DN, 0xff00, "CSM timer portamento down", "CSMportDn", -1, -1},
	
	/*{MUS_FX_SET_FREQUENCY_LOWER_BYTE, 0xff00, "Set frequency lower  byte (0x0000FF)", "FreqLowByte", -1, -1},
	{MUS_FX_SET_FREQUENCY_LOWER_BYTE, 0xff00, "Set frequency middle byte (0x00FF00)", "FreqMidByte", -1, -1},
	{MUS_FX_SET_FREQUENCY_LOWER_BYTE, 0xff00, "Set frequency higher byte (0xFF0000)", "FreqHighByte", -1, -1},*/
	
	{MUS_FX_SET_NOISE_CONSTANT_PITCH, 0xff00, "Set noise note in \"LOCK NOISE PITCH\" mode", "NoiPitNote", 0, FREQ_TAB_SIZE - 1}, //wasn't there
	{MUS_FX_ARPEGGIO_ABS, 0xff00, "Set absolute arpeggio note", "AbsArp", 0, FREQ_TAB_SIZE - 1},
	{MUS_FX_SET_EXT_ARP, 0xff00, "Set external arpeggio notes", "ExtArp", -1, -1},
	{MUS_FX_PORTA_UP, 0xff00, "Portamento up", "PortUp", -1, -1},
	{MUS_FX_PORTA_DN, 0xff00, "Portamento down", "PortDn", -1, -1},
	{MUS_FX_PORTA_UP_LOG, 0xff00, "Portamento up (curve)", "PortUpC", -1, -1},
	{MUS_FX_PORTA_DN_LOG, 0xff00, "Portamento down (curve)", "PortDnC", -1, -1},
	{MUS_FX_EXT_NOTE_DELAY, 0xfff0, "Note delay", "Delay", -1, -1},
	{MUS_FX_VIBRATO, 0xff00, "Vibrato", "Vib", -1, -1},
	{MUS_FX_TREMOLO, 0xff00, "Tremolo", "Trem", -1, -1}, //wasn't there
	{MUS_FX_FM_VIBRATO, 0xff00, "FM modulator vibrato", "FM vib", -1, -1}, //wasn't there
	{MUS_FX_FM_TREMOLO, 0xff00, "FM modulator tremolo", "FM trem", -1, -1}, //wasn't there
	{MUS_FX_PWM, 0xff00, "Pulse width modification", "PWM", -1, -1}, //wasn't there
	{MUS_FX_SLIDE, 0xff00, "Slide", "Slide", -1, -1},
	{MUS_FX_FAST_SLIDE, 0xff00, "Fast slide (16x faster)", "FastSlide", -1, -1},
	{MUS_FX_PORTA_UP_SEMI, 0xff00, "Portamento up (semitones)", "PortUpST", -1, -1},
	{MUS_FX_PORTA_DN_SEMI, 0xff00, "Portamento down (semitones)", "PortDnST", -1, -1},
	{MUS_FX_EXT_TOGGLE_FILTER, 0xfff0, "Toggle filter", "ToggleFilt", -1, -1},
	{MUS_FX_CUTOFF_UP, 0xff00, "Filter cutoff up", "CutoffUp", -1, -1},
	{MUS_FX_CUTOFF_DN, 0xff00, "Filter cutoff down", "CutoffDn", -1, -1},
	{MUS_FX_CUTOFF_SET, 0xff00, "Set filter cutoff", "Cutoff", 0, 0xff},
	{MUS_FX_CUTOFF_SET_COMBINED, 0xff00, "Set combined cutoff", "CutoffAHX", 0, 0xff},
	{MUS_FX_RESONANCE_SET, 0xff00, "Set filter resonance", "Resonance", 0, 0xf}, //was `0, 3},`
	{MUS_FX_RESONANCE_UP, 0xff00, "Filter resonance up", "ResUp", 0, 0xff},
	{MUS_FX_RESONANCE_DOWN, 0xff00, "Filter resonance down", "ResDn", 0, 0xff},
	{MUS_FX_FILTER_TYPE, 0xff00, "Set filter type", "FltType", 0, 0x7},
	{MUS_FX_FILTER_SLOPE, 0xfff0, "Set filter slope", "FltSlope", 0, 5}, //wasn't there
	{MUS_FX_PW_DN, 0xff00, "Pulse width down", "PWDn", -1, -1},
	{MUS_FX_PW_UP, 0xff00, "Pulse width up", "PWUp", -1, -1},
	{MUS_FX_PW_SET, 0xff00, "Set pulse width", "PW", -1, -1},
	{MUS_FX_SET_VOLUME, 0xff00, "Set volume", "Volume", 0, 0xff},
	{MUS_FX_SET_ABSOLUTE_VOLUME, 0xff00, "Set absolute volume (doesn't obey \"relative\" flag)", "AbsVol", 0, 0xff},
	{MUS_FX_FADE_GLOBAL_VOLUME, 0xff00, "Global volume fade", "GlobFade", -1, -1},
	{MUS_FX_SET_GLOBAL_VOLUME, 0xff00, "Set global volume", "GlobVol", 0, MAX_VOLUME},
	{MUS_FX_SET_CHANNEL_VOLUME, 0xff00, "Set channel volume", "ChnVol", 0, MAX_VOLUME},
	{MUS_FX_SET_VOL_KSL_LEVEL, 0xff00, "Set volume key scaling level", "VolKslLev", 0, 0xff}, //wasn't there
	{MUS_FX_SET_FM_VOL_KSL_LEVEL, 0xff00, "Set FM modulator volume key scaling level", "VolKslFMlev", 0, 0xff}, //wasn't there
	{MUS_FX_SET_ENV_KSL_LEVEL, 0xff00, "Set envelope key scaling level", "EnvKslLev", 0, 0xff}, //wasn't there
	{MUS_FX_SET_FM_ENV_KSL_LEVEL, 0xff00, "Set FM modulator envelope key scaling level", "EnvKslFMlev", 0, 0xff}, //wasn't there
	{MUS_FX_SET_WAVEFORM, 0xff00, "Set waveform", "Waveform", 0, 0xff},
	{MUS_FX_EXT_SET_NOISE_MODE, 0xfff0, "Set noise mode", "NoiMode", 0, 0x7}, //wasn't there
	{MUS_FX_EXT_OSC_MIX, 0xfff0, "Set oscillators' mix mode", "OscMix", 0, 4}, //wasn't there
	{MUS_FX_EXT_SINE_ACC_SHIFT, 0xfff0, "Set sine wave phase shift", "SinePhaseShift", 0, 0xF}, //wasn't there
	{MUS_FX_SET_WAVETABLE_ITEM, 0xff00, "Set wavetable item", "Wavetable", 0, CYD_WAVE_MAX_ENTRIES - 1},
	{MUS_FX_SET_FXBUS, 0xff00, "Set FX bus", "SetFxBus", 0, CYD_MAX_FX_CHANNELS - 1},
	{MUS_FX_SET_RINGSRC, 0xff00, "Set ring modulation source (FF=off)", "SetRingSrc", 0, 0xff},
	{MUS_FX_SET_SYNCSRC, 0xff00, "Set sync source (FF=off)", "SetSyncSrc", 0, 0xff},
	{MUS_FX_SET_DOWNSAMPLE, 0xff00, "Set downsample", "SetDnSmp", 0, 0xff},
	{MUS_FX_SET_SPEED, 0xff00, "Set speed (program period in instrument program)", "ProgPeriod", -1, -1},
	{MUS_FX_SET_SPEED1, 0xff00, "Set speed 1 (00-FF)", "Speed1Full", -1, -1}, //wasn't there
	{MUS_FX_SET_SPEED2, 0xff00, "Set speed 2 (00-FF)", "Speed2Full", -1, -1}, //wasn't there
	{MUS_FX_SET_GROOVE, 0xff00, "Set groove", "Groove", 0, MUS_MAX_GROOVES - 1}, //wasn't there
	{MUS_FX_SET_RATE, 0xff00, "Set rate lower byte", "RateLowByte", -1, -1},
	{MUS_FX_SET_RATE_HIGHER_BYTE, 0xff00, "Set rate higher byte", "RateHighByte", -1, -1}, //wasn't there
	{MUS_FX_LOOP_PATTERN, 0xff00, "Loop pattern", "PatLoop", -1, -1},
	{MUS_FX_FT2_PATTERN_LOOP, 0xfff0, "Loop pattern (FastTracker II style)", "PatLoopFT2", -1, -1},
	{MUS_FX_SKIP_PATTERN, 0xff00, "Skip pattern", "PatSkip", -1, -1},
	{MUS_FX_TRIGGER_RELEASE, 0xff00, "Trigger release", "Release", 0, 0xff},
	{MUS_FX_TRIGGER_MACRO_RELEASE, 0xff00, "Trigger macro release", "ProgRelease", 0, 0xff},
	{MUS_FX_TRIGGER_CARRIER_RELEASE, 0xff00, "Trigger FM carrier release", "FMcarRelease", 0, 0xff}, //wasn't there
	{MUS_FX_TRIGGER_FM_RELEASE, 0xff00, "Trigger FM modulator release", "FMmodRelease", 0, 0xff}, //wasn't there
	{MUS_FX_RESTART_PROGRAM, 0xff00, "Restart program", "Restart", 0, 0},
	{MUS_FX_FADE_VOLUME, 0xff00, "Fade volume", "VolFade", -1, -1},
	{MUS_FX_FM_FADE_VOLUME, 0xff00, "Fade FM modulator volume", "FMvolFade", -1, -1}, //wasn't there
	{MUS_FX_EXT_FADE_VOLUME_UP, 0xfff0, "Fine fade volume in", "VolUpFine", 0, 0xf},
	{MUS_FX_EXT_FADE_VOLUME_DN, 0xfff0, "Fine fade volume out", "VolDnFine", 0, 0xf},
	{MUS_FX_FM_EXT_FADE_VOLUME_UP, 0xfff0, "Fine fade FM modulator volume in", "FMvolUpFine", 0, 0xf}, //wasn't there
	{MUS_FX_FM_EXT_FADE_VOLUME_DN, 0xfff0, "Fine fade FM modulator volume out", "FMvolDnFine", 0, 0xf}, //wasn't there
	{MUS_FX_EXT_PORTA_UP, 0xfff0, "Fine portamento up", "PortUpFine", 0, 0xf},
	{MUS_FX_EXT_PORTA_DN, 0xfff0, "Fine portamento down", "PortDnFine", 0, 0xf},
	{MUS_FX_EXT_FINE_PORTA_UP, 0xfff0, "Extra fine portamento up", "PortUpExtraFine", 0, 0xf},
	{MUS_FX_EXT_FINE_PORTA_DN, 0xfff0, "Extra fine portamento down", "PortDnExtraFine", 0, 0xf},
	{MUS_FX_EXT_NOTE_CUT, 0xfff0, "Note cut", "NoteCut", 0, 0xf},
	{MUS_FX_EXT_RETRIGGER, 0xfff0, "Retrigger", "Retrig", 0, 0xf},
	
	{MUS_FX_PHASE_RESET, 0xfff0, "Oscillator phase reset on tick X", "OscPhRes", 0, 0xf},
	{MUS_FX_NOISE_PHASE_RESET, 0xfff0, "Noise osc. ph. reset (in LOCK NOI PITCH mode)", "NoiPhRes", 0, 0xf},
	{MUS_FX_WAVE_PHASE_RESET, 0xfff0, "Wave phase reset on tick X", "WavePhRes", 0, 0xf},
	{MUS_FX_FM_PHASE_RESET, 0xfff0, "FM modulator phase reset", "FMmodPhRes", 0, 0xf},
	
	{MUS_FX_WAVETABLE_OFFSET, 0xf000, "Wavetable offset", "WaveOffs", 0, 0xfff},
	{MUS_FX_WAVETABLE_OFFSET_UP, 0xff00, "Wavetable offset up", "WaveOffsUp", 0, 0xff}, //wasn't there
	{MUS_FX_WAVETABLE_OFFSET_DOWN, 0xff00, "Wavetable offset down", "WaveOffsDn", 0, 0xff}, //wasn't there
	{MUS_FX_WAVETABLE_OFFSET_UP_FINE, 0xfff0, "Wavetable offset up (fine)", "WaveOffsUpFine", 0, 0xf}, //wasn't there
	{MUS_FX_WAVETABLE_OFFSET_DOWN_FINE, 0xfff0, "Wavetable offset down (fine)", "WaveOffsDnFine", 0, 0xf}, //wasn't there
	
	{MUS_FX_WAVETABLE_END_POINT, 0xf000, "Wavetable end offset", "WaveEndOffs", 0, 0xfff}, //wasn't there
	{MUS_FX_WAVETABLE_END_POINT_UP, 0xff00, "Wavetable end offset up", "WaveEndOffsUp", 0, 0xff}, //wasn't there
	{MUS_FX_WAVETABLE_END_POINT_DOWN, 0xff00, "Wavetable end offset down", "WaveEndOffsDn", 0, 0xff}, //wasn't there
	{MUS_FX_WAVETABLE_END_POINT_UP_FINE, 0xfff0, "Wavetable end offset up (fine)", "WaveEndOffsUpFine", 0, 0xf}, //wasn't there
	{MUS_FX_WAVETABLE_END_POINT_DOWN_FINE, 0xfff0, "Wavetable end offset down (fine)", "WaveEndOffsDnFine", 0, 0xf}, //wasn't there
	
	{MUS_FX_FM_WAVETABLE_OFFSET, 0xff00, "FM wavetable offset", "FMWaveOffs", 0, 0xff}, //wasn't there
	{MUS_FX_FM_WAVETABLE_OFFSET_UP, 0xff00, "FM wavetable offset up", "FMWaveOffsUp", 0, 0xff}, //wasn't there
	{MUS_FX_FM_WAVETABLE_OFFSET_DOWN, 0xff00, "FM wavetable offset down", "FMWaveOffsDn", 0, 0xff}, //wasn't there
	{MUS_FX_FM_WAVETABLE_OFFSET_UP_FINE, 0xfff0, "FM wavetable offset up (fine)", "FMWaveOffsUpFine", 0, 0xf}, //wasn't there
	{MUS_FX_FM_WAVETABLE_OFFSET_DOWN_FINE, 0xfff0, "FM wavetable offset down (fine)", "FMWaveOffsDnFine", 0, 0xf}, //wasn't there
	
	{MUS_FX_FM_WAVETABLE_END_POINT, 0xff00, "FM wavetable end offset", "FMWaveEndOffs", 0, 0xff}, //wasn't there
	{MUS_FX_FM_WAVETABLE_END_POINT_UP, 0xff00, "FM wavetable end offset up", "FMWaveEndOffsUp", 0, 0xff}, //wasn't there
	{MUS_FX_FM_WAVETABLE_END_POINT_DOWN, 0xff00, "FM wavetable end offset down", "FMWaveEndOffsDn", 0, 0xff}, //wasn't there
	{MUS_FX_FM_WAVETABLE_END_POINT_UP_FINE, 0xfff0, "FM wavetable end offset up (fine)", "FMWaveEndOffsUpFine", 0, 0xf}, //wasn't there
	{MUS_FX_FM_WAVETABLE_END_POINT_DOWN_FINE, 0xfff0, "FM wavetable end offset down (fine)", "FMWaveEndOffsDnFine", 0, 0xf}, //wasn't there
	
	{MUS_FX_SET_PANNING, 0xff00, "Set panning", "SetPan", -1, -1},
	{MUS_FX_PAN_LEFT, 0xff00, "Pan left", "PanLeft", -1, -1},
	{MUS_FX_PAN_RIGHT, 0xff00, "Pan right", "PanRight", -1, -1},
	{MUS_FX_BUZZ_UP, 0xff00, "Tune buzz up", "BuzzUp", -1, -1},
	{MUS_FX_BUZZ_DN, 0xff00, "Tune buzz down", "BuzzDn", -1, -1},
	{MUS_FX_BUZZ_SHAPE, 0xff00, "Set buzz shape", "BuzzShape", 0, 3},
	{MUS_FX_BUZZ_SET, 0xff00, "Set buzz finetune", "BuzzFine", -1, -1},
	{MUS_FX_CUTOFF_FINE_SET, 0xf000, "Set filter cutoff (fine)", "CutFine", 0, CYD_CUTOFF_MAX - 1},
	{MUS_FX_PW_FINE_SET, 0xf000, "Set pulse width (fine)", "PWFine", 0, 0xfff}, //wasn't there
	{MUS_FX_BUZZ_SET_SEMI, 0xff00, "Set buzz semitone", "BuzzSemi", -1, -1},
	
	{MUS_FX_SET_ATTACK_RATE, 0xff00, "Set attack rate", "AtkRate", 0, 0x7f},
	{MUS_FX_SET_DECAY_RATE, 0xff00, "Set decay rate", "DecRate", 0, 0x7f},
	{MUS_FX_SET_SUSTAIN_LEVEL, 0xff00, "Set sustain level", "SusLev", 0, 0x3f},
	{MUS_FX_SET_SUSTAIN_RATE, 0xff00, "Set sustain rate (only in FM op program)", "SusRate", 0, 0x3f},
	{MUS_FX_SET_RELEASE_RATE, 0xff00, "Set release rate", "RelRate", 0, 0x7f},
	
	{MUS_FX_SET_EXPONENTIALS, 0xfff0, "Set exponential settings", "SetExpSett", 0, 0xf}, //wasn't there
	{MUS_FX_FM_SET_EXPONENTIALS, 0xfff0, "Set FM modulator exponential settings", "SetFMexpSett", 0, 0xf}, //wasn't there
	
	{MUS_FX_FM_SET_MODULATION, 0xff00, "Set FM modulation", "FMMod", 0, 0xff},
	{MUS_FX_FM_SET_FEEDBACK, 0xfff0, "Set FM feedback", "FMFB", 0, 15},
	{MUS_FX_FM_SET_HARMONIC, 0xff00, "Set FM multiplier", "FMMult", 0, 255},
	{MUS_FX_FM_SET_WAVEFORM, 0xff00, "Set FM waveform", "FMWave", 0, 255},
	
	{MUS_FX_FM_SET_4OP_ALGORITHM, 0xff00, "Set 4-op FM algorithm", "4opFMalg", 0, 13},
	{MUS_FX_FM_SET_4OP_MASTER_VOLUME, 0xff00, "Set 4-op FM master volume", "4opFMvol", 0, 255},
	{MUS_FX_FM_4OP_MASTER_FADE_VOLUME_UP, 0xfff0, "4-op FM master volume up", "4opFMvolUp", 0, 0xf},
	{MUS_FX_FM_4OP_MASTER_FADE_VOLUME_DOWN, 0xfff0, "4-op FM master volume down", "4opFMvolDown", 0, 0xf},
	
	{MUS_FX_FM_4OP_SET_DETUNE, 0xfff0, "Set 4-op FM operator detune", "4opFMdet", 0, 15},
	{MUS_FX_FM_4OP_SET_COARSE_DETUNE, 0xfff0, "Set 4-op FM operator coarse detune", "4opFMdet2", 0, 3},
	
	{MUS_FX_FM_4OP_SET_SSG_EG_TYPE, 0xfff0, "Set 4-op FM operator SSG-EG mode", "4opFMssgEgMode", 0, 0xf},
	
	{MUS_FX_FM_TRIGGER_OP1_RELEASE, 0xff00, "Trigger FM operator 1 release", "TrigFmOp1rel", 0, 255},
	{MUS_FX_FM_TRIGGER_OP2_RELEASE, 0xff00, "Trigger FM operator 2 release", "TrigFmOp2rel", 0, 255},
	{MUS_FX_FM_TRIGGER_OP3_RELEASE, 0xff00, "Trigger FM operator 3 release", "TrigFmOp3rel", 0, 255},
	{MUS_FX_FM_TRIGGER_OP4_RELEASE, 0xff00, "Trigger FM operator 4 release", "TrigFmOp4rel", 0, 255},
	
	{MUS_FX_FM_SET_OP1_ATTACK_RATE, 0xff00, "Set FM operator 1 attack rate", "FmOp1atkRate", 0, 127},
	{MUS_FX_FM_SET_OP2_ATTACK_RATE, 0xff00, "Set FM operator 2 attack rate", "FmOp2atkRate", 0, 127},
	{MUS_FX_FM_SET_OP3_ATTACK_RATE, 0xff00, "Set FM operator 3 attack rate", "FmOp3atkRate", 0, 127},
	{MUS_FX_FM_SET_OP4_ATTACK_RATE, 0xff00, "Set FM operator 4 attack rate", "FmOp4atkRate", 0, 127},
	
	{MUS_FX_FM_SET_OP1_DECAY_RATE, 0xff00, "Set FM operator 1 decay rate", "FmOp1decRate", 0, 127},
	{MUS_FX_FM_SET_OP2_DECAY_RATE, 0xff00, "Set FM operator 2 decay rate", "FmOp2decRate", 0, 127},
	{MUS_FX_FM_SET_OP3_DECAY_RATE, 0xff00, "Set FM operator 3 decay rate", "FmOp3decRate", 0, 127},
	{MUS_FX_FM_SET_OP4_DECAY_RATE, 0xff00, "Set FM operator 4 decay rate", "FmOp4decRate", 0, 127},
	
	{MUS_FX_FM_SET_OP1_SUSTAIN_LEVEL, 0xff00, "Set FM operator 1 sustain level", "FmOp1susLev", 0, 63},
	{MUS_FX_FM_SET_OP2_SUSTAIN_LEVEL, 0xff00, "Set FM operator 2 sustain level", "FmOp2susLev", 0, 63},
	{MUS_FX_FM_SET_OP3_SUSTAIN_LEVEL, 0xff00, "Set FM operator 3 sustain level", "FmOp3susLev", 0, 63},
	{MUS_FX_FM_SET_OP4_SUSTAIN_LEVEL, 0xff00, "Set FM operator 4 sustain level", "FmOp4susLev", 0, 63},
	
	{MUS_FX_FM_SET_OP1_SUSTAIN_RATE, 0xff00, "Set FM operator 1 sustain rate", "FmOp1susRate", 0, 127},
	{MUS_FX_FM_SET_OP2_SUSTAIN_RATE, 0xff00, "Set FM operator 2 sustain rate", "FmOp2susRate", 0, 127},
	{MUS_FX_FM_SET_OP3_SUSTAIN_RATE, 0xff00, "Set FM operator 3 sustain rate", "FmOp3susRate", 0, 127},
	{MUS_FX_FM_SET_OP4_SUSTAIN_RATE, 0xff00, "Set FM operator 4 sustain rate", "FmOp4susRate", 0, 127},
	
	{MUS_FX_FM_SET_OP1_RELEASE_RATE, 0xff00, "Set FM operator 1 release rate", "FmOp1relRate", 0, 127},
	{MUS_FX_FM_SET_OP2_RELEASE_RATE, 0xff00, "Set FM operator 2 release rate", "FmOp2relRate", 0, 127},
	{MUS_FX_FM_SET_OP3_RELEASE_RATE, 0xff00, "Set FM operator 3 release rate", "FmOp3relRate", 0, 127},
	{MUS_FX_FM_SET_OP4_RELEASE_RATE, 0xff00, "Set FM operator 4 release rate", "FmOp4relRate", 0, 127},
	
	{MUS_FX_FM_SET_OP1_MULT, 0xff00, "Set FM operator 1 multiplier", "FmOp1Mult", 0, 255},
	{MUS_FX_FM_SET_OP2_MULT, 0xff00, "Set FM operator 2 multiplier", "FmOp2Mult", 0, 255},
	{MUS_FX_FM_SET_OP3_MULT, 0xff00, "Set FM operator 3 multiplier", "FmOp3Mult", 0, 255},
	{MUS_FX_FM_SET_OP4_MULT, 0xff00, "Set FM operator 4 multiplier", "FmOp4Mult", 0, 255},
	
	{MUS_FX_FM_SET_OP1_VOL, 0xff00, "Set FM operator 1 volume", "FmOp1Vol", 0, 255},
	{MUS_FX_FM_SET_OP2_VOL, 0xff00, "Set FM operator 2 volume", "FmOp2Vol", 0, 255},
	{MUS_FX_FM_SET_OP3_VOL, 0xff00, "Set FM operator 3 volume", "FmOp3Vol", 0, 255},
	{MUS_FX_FM_SET_OP4_VOL, 0xff00, "Set FM operator 4 volume", "FmOp4Vol", 0, 255},
	
	{MUS_FX_FM_SET_OP1_DETUNE, 0xfff0, "Set FM operator 1 detune", "FmOp1Det", 0, 15},
	{MUS_FX_FM_SET_OP2_DETUNE, 0xfff0, "Set FM operator 2 detune", "FmOp2Det", 0, 15},
	{MUS_FX_FM_SET_OP3_DETUNE, 0xfff0, "Set FM operator 3 detune", "FmOp3Det", 0, 15},
	{MUS_FX_FM_SET_OP4_DETUNE, 0xfff0, "Set FM operator 4 detune", "FmOp4Det", 0, 15},
	
	{MUS_FX_FM_SET_OP1_COARSE_DETUNE, 0xfff0, "Set FM operator 1 coarse detune", "FmOp1Det2", 0, 3},
	{MUS_FX_FM_SET_OP2_COARSE_DETUNE, 0xfff0, "Set FM operator 2 coarse detune", "FmOp2Det2", 0, 3},
	{MUS_FX_FM_SET_OP3_COARSE_DETUNE, 0xfff0, "Set FM operator 3 coarse detune", "FmOp3Det2", 0, 3},
	{MUS_FX_FM_SET_OP4_COARSE_DETUNE, 0xfff0, "Set FM operator 4 coarse detune", "FmOp4Det2", 0, 3},
	
	{MUS_FX_FM_SET_OP1_FEEDBACK, 0xfff0, "Set FM operator 1 feedback", "FmOp1FB", 0, 15},
	{MUS_FX_FM_SET_OP2_FEEDBACK, 0xfff0, "Set FM operator 2 feedback", "FmOp2FB", 0, 15},
	{MUS_FX_FM_SET_OP3_FEEDBACK, 0xfff0, "Set FM operator 3 feedback", "FmOp3FB", 0, 15},
	{MUS_FX_FM_SET_OP4_FEEDBACK, 0xfff0, "Set FM operator 4 feedback", "FmOp4FB", 0, 15},
	
	{MUS_FX_FM_SET_OP1_SSG_EG_TYPE, 0xfff0, "Set FM operator 1 SSG-EG mode", "FmOp1ssgEgMode", 0, 0xf},
	{MUS_FX_FM_SET_OP2_SSG_EG_TYPE, 0xfff0, "Set FM operator 2 SSG-EG mode", "FmOp2ssgEgMode", 0, 0xf},
	{MUS_FX_FM_SET_OP3_SSG_EG_TYPE, 0xfff0, "Set FM operator 3 SSG-EG mode", "FmOp3ssgEgMode", 0, 0xf},
	{MUS_FX_FM_SET_OP4_SSG_EG_TYPE, 0xfff0, "Set FM operator 4 SSG-EG mode", "FmOp4ssgEgMode", 0, 0xf},
	
	{0, 0, NULL}
};

const InstructionDesc * get_instruction_desc(Uint16 command)
{
	for (int i = 0; instruction_desc[i].name != NULL; ++i)
	{
		if (instruction_desc[i].opcode == (command & instruction_desc[i].mask))
		{
			return &instruction_desc[i];
		}
	}
	
	return false;
}


Uint16 get_instruction_mask(Uint16 command)
{
	for (int i = 0; instruction_desc[i].name != NULL; ++i)
	{
		if (instruction_desc[i].opcode == (command & instruction_desc[i].mask))
		{
			return instruction_desc[i].mask;
		}
	}
	
	return false;
}


bool is_valid_command(Uint16 command)
{
	return get_instruction_desc(command) != NULL;
}

	
void get_command_desc(char *text, size_t buffer_size, Uint16 inst)
{
	const InstructionDesc *i = get_instruction_desc(inst);

	if (i == NULL) 
	{
		strcpy(text, "Unknown\n");
		return;
	}

	const char *name = i->name;
	const Uint16 fi = i->opcode;
	
	static const char * ssg_eg_types[] = { "\\\\\\\\", "\\___", "\\/\\/", "\\\xfd\xfd\xfd", "////", "/\xfd\xfd\xfd", "/\\/\\", "/___" };
	
	Uint32 p = 0x7F800000; //a hack to display infinity
	
	if ((fi & 0xff00) == MUS_FX_SET_WAVEFORM)
	{
		if (inst & 0xff)
			snprintf(text, buffer_size, "%s (%s%s%s%s%s%s%s)\n", name, (inst & MUS_FX_WAVE_NOISE) ? "N" : "", (inst & MUS_FX_WAVE_SAW) ? "S" : "", (inst & MUS_FX_WAVE_TRIANGLE) ? "T" : "", 
				(inst & MUS_FX_WAVE_PULSE) ? "P" : "", (inst & MUS_FX_WAVE_LFSR) ? "L" : "", (inst & MUS_FX_WAVE_WAVE) ? "W" : "", (inst & MUS_FX_WAVE_SINE) ? (((inst & (MUS_FX_WAVE_WAVE|MUS_FX_WAVE_NOISE|MUS_FX_WAVE_SAW|MUS_FX_WAVE_TRIANGLE|MUS_FX_WAVE_PULSE|MUS_FX_WAVE_LFSR|MUS_FX_WAVE_WAVE)) ? " Sine" : "Sine")) : "");
		else
			snprintf(text, buffer_size, "%s (None)\n", name);
	}
	
	else if ((fi & 0xff00) == MUS_FX_FILTER_TYPE)
	{
		static const char *fn[FLT_TYPES] = {"LP", "HP", "BP", "LHP", "HBP", "LBP", "ALL"}; //was `{"LP", "HP", "BP"};`
		snprintf(text, buffer_size, "%s (%s)\n", name, fn[(inst & 0xf) % FLT_TYPES]);
	}
	
	else if ((fi & 0xff00) == MUS_FX_BUZZ_SHAPE)
	{
		snprintf(text, buffer_size, "%s (%c)\n", name, ((inst & 0xf) & 3) + 0xf0);
	}
	
	else if ((fi & 0xff00) == MUS_FX_SET_FXBUS)
	{
		snprintf(text, buffer_size, "%s (%s)\n", name, mused.song.fx[(inst & 0xf) % CYD_MAX_FX_CHANNELS].name);
	}
	
	else if ((fi & 0xff00) == MUS_FX_SET_WAVETABLE_ITEM)
	{
		snprintf(text, buffer_size, "%s (%s)\n", name, mused.song.wavetable_names[(inst & 0xff)]);
	}
	
	else if ((fi & 0xff00) == MUS_FX_SET_VOLUME || (fi & 0xff00) == MUS_FX_SET_ABSOLUTE_VOLUME)
	{
		snprintf(text, buffer_size, "%s (%+.1f dB)\n", name, percent_to_dB((float)(inst & 0xff) / MAX_VOLUME));
	}
	
	else if ((fi & 0xfff0) == MUS_FX_EXT_SET_NOISE_MODE)
	{
		if (inst & 0xf)
		{
			snprintf(text, buffer_size, "%s (%s%s%s)\n", name, (inst & 0b1) ? "1-bit" : "", (inst & 0b10) ? ((inst & 0b1) ? " Metal" : "Metal") : "", (inst & 0b100) ? ((inst & 0b10) || (inst & 0b1) ? " Fixed pitch" : "Fixed pitch") : "");
		}
		
		else
		{
			snprintf(text, buffer_size, "%s (Default)\n", name);
		}
	}
	
	else if ((fi & 0xfff0) == MUS_FX_SET_EXPONENTIALS || (fi & 0xfff0) == MUS_FX_FM_SET_EXPONENTIALS)
	{
		if (inst & 0xf)
		{
			snprintf(text, buffer_size, "%s (%s%s%s%s)\n", name, (inst & 0b1) ? "V" : "", (inst & 0b10) ? "A" : "", (inst & 0b100) ? "D" : "", (inst & 0b1000) ? "R" : "");
		}
		
		else
		{
			snprintf(text, buffer_size, "%s (Default)\n", name);
		}
	}
	
	else if ((fi & 0xfff0) == MUS_FX_FM_4OP_SET_DETUNE || (fi & 0xfff0) == MUS_FX_FM_SET_OP1_DETUNE || (fi & 0xfff0) == MUS_FX_FM_SET_OP2_DETUNE || (fi & 0xfff0) == MUS_FX_FM_SET_OP3_DETUNE || (fi & 0xfff0) == MUS_FX_FM_SET_OP4_DETUNE)
	{
		snprintf(text, buffer_size, "%s (%+1d)\n", name, my_min(7, my_max((inst & 0xF) - 7, -7)));
	}
	
	#define envelope_length(slope) (slope != 0 ? (float)(((slope) * (slope) * 256 / (ENVELOPE_SCALE * ENVELOPE_SCALE))) / ((float)CYD_BASE_FREQ / 1000.0f) : 0.0f)
	
	else if ((fi & 0xff00) == MUS_FX_SET_ATTACK_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP1_ATTACK_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP2_ATTACK_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP3_ATTACK_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP4_ATTACK_RATE || (fi & 0xff00) == MUS_FX_SET_DECAY_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP1_DECAY_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP2_DECAY_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP3_DECAY_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP4_DECAY_RATE || (fi & 0xff00) == MUS_FX_SET_RELEASE_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP1_RELEASE_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP2_RELEASE_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP3_RELEASE_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP4_RELEASE_RATE)
	{
		snprintf(text, buffer_size, "%s (%s %.1f ms)", name, (inst & 0x40) ? "Global," : "Local,", envelope_length((inst & 0x3f)));
	}
	
	else if ((fi & 0xff00) == MUS_FX_SET_SUSTAIN_LEVEL || (fi & 0xff00) == MUS_FX_FM_SET_OP1_SUSTAIN_LEVEL || (fi & 0xff00) == MUS_FX_FM_SET_OP2_SUSTAIN_LEVEL || (fi & 0xff00) == MUS_FX_FM_SET_OP3_SUSTAIN_LEVEL || (fi & 0xff00) == MUS_FX_FM_SET_OP4_SUSTAIN_LEVEL)
	{
		snprintf(text, buffer_size, "%s (%s)", name, (inst & 0x20) ? "Global" : "Local");
	}
	
	else if ((fi & 0xff00) == MUS_FX_SET_SUSTAIN_RATE)
	{
		snprintf(text, buffer_size, "%s (%.1f ms)", name, (inst & 0x3f) == 0 ? *(float *)&p : envelope_length(64 - (inst & 0x3f)));
	}
	
	else if ((fi & 0xff00) == MUS_FX_FM_SET_OP1_SUSTAIN_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP2_SUSTAIN_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP3_SUSTAIN_RATE || (fi & 0xff00) == MUS_FX_FM_SET_OP4_SUSTAIN_RATE)
	{
		snprintf(text, buffer_size, "%s (%s %.1f ms)", name, (inst & 0x40) ? "Global" : "Local", (inst & 0x3f) == 0 ? *(float *)&p : envelope_length(64 - (inst & 0x3f)));
	}
	
	else if ((fi & 0xfff0) == MUS_FX_FM_SET_OP1_SSG_EG_TYPE || (fi & 0xfff0) == MUS_FX_FM_SET_OP2_SSG_EG_TYPE || (fi & 0xfff0) == MUS_FX_FM_SET_OP3_SSG_EG_TYPE || (fi & 0xfff0) == MUS_FX_FM_SET_OP4_SSG_EG_TYPE || (fi & 0xfff0) == MUS_FX_FM_4OP_SET_SSG_EG_TYPE)
	{
		snprintf(text, buffer_size, "%s (%s%s)\n", name, (inst & 8) ? "Enabled, " : "Disabled, ", ssg_eg_types[inst & 0x7]);
	}
	
	else if ((fi & 0xff00) == MUS_FX_SET_CSM_TIMER_NOTE)
	{
		snprintf(text, buffer_size, "%s (%s)", name, notename((inst & 0xff)));
	}
	
	else if ((fi & 0xff00) == MUS_FX_SET_CSM_TIMER_FINETUNE)
	{
		snprintf(text, buffer_size, "%s (+%d)", name, (inst & 0xff));
	}
	
	else if ((fi & 0xfff0) == MUS_FX_EXT_TOGGLE_FILTER)
	{
		snprintf(text, buffer_size, "%s (%s)", name, (inst & 0xf) ? "enable" : "disable");
	}
	
	else if ((fi & 0xff00) == MUS_FX_SET_PANNING)
	{
		char tmp[4] = "\xfa\xf9";
		
		//bool is_max_pan = (inst & 0xff) == CYD_PAN_CENTER;
		
		if ((inst & 0xff) != CYD_PAN_CENTER && (inst & 0xff) != CYD_PAN_CENTER - 1)
		{
			snprintf(tmp, sizeof(tmp), "%c%X", (inst & 0xff) < CYD_PAN_CENTER ? '\xf9' : '\xfa', (inst & 0xff) < CYD_PAN_CENTER ? 0x7f - (inst & 0xff) : (inst & 0xff) - 0x80);
		}
		
		snprintf(text, buffer_size, "%s (%s)\n", name, ((inst & 0xff) == CYD_PAN_CENTER || (inst & 0xff) == CYD_PAN_CENTER - 1) ? "Center" : tmp);
	}
	
	else if ((fi & 0xfff0) == MUS_FX_FT2_PATTERN_LOOP)
	{
		if(inst & 0xf)
		{
			if((inst & 0xf) > 1)
			{
				snprintf(text, buffer_size, "%s (loop end; loop %d times)", name, (inst & 0xf));
			}
			
			else
			{
				snprintf(text, buffer_size, "%s (loop end; loop once)", name);
			}
		}
		
		else
		{
			snprintf(text, buffer_size, "%s (loop begin)", name);
		}
	}
	
	else snprintf(text, buffer_size, "%s\n", name);
}

Uint16 validate_command(Uint16 command)
{
	const InstructionDesc *i = get_instruction_desc(command);
	
	/*if (i)
	{
		if (i->maxv != -1 && i->minv != -1)
		{
			Uint16 v = ((~i->mask) & command) & ~0x8000;
			Uint16 c = command & 0x8000;
			v = my_min(i->maxv, my_max(i->minv, v));
			command = (command & i->mask) | v | c;
		}
	}
	
	if(command >= 0xf000)
	{
		return command & 0x0fff;
	}*/
	if(i)
	{
		if((command & i->mask) > i->maxv && i->maxv != -1 && i->minv != -1)
		{
			return (command & i->mask) | my_min((command & ~(i->mask)), i->maxv);
		}
	}
	
	return command;
}


const InstructionDesc* list_all_commands()
{
	return instruction_desc;
}