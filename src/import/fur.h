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

enum //general effects which are not chip-specific
{
	FUR_EFF_ARPEGGIO = 0x0000,
	FUR_EFF_PORTA_UP = 0x0100,
	FUR_EFF_PORTA_DN = 0x0200,
	FUR_EFF_SLIDE = 0x0300,
	FUR_EFF_VIBRATO = 0x0400,
	FUR_EFF_TREMOLO = 0x0700,
	FUR_EFF_PANNING = 0x0800, //08xy, x for left ch and y for right ch
	FUR_EFF_GROOVE_SPEED_1 = 0x0900, //set groove or speed 1
	FUR_EFF_VOLUME_FADE = 0x0A00,
	FUR_EFF_SEQUENCE_POSITION_JUMP = 0x0B00,
	FUR_EFF_RETRIGGER = 0x0C00, //0Cxx; repeats current note every xx ticks
	FUR_EFF_SKIP_PATTERN = 0x0D00,
	FUR_EFF_SPEED_2 = 0x0F00,

	FUR_EFF_SET_LINEAR_PANNING = 0x8000,
	FUR_EFF_SAMPLE_OFFSET = 0x9000, //9xxx
	FUR_EFF_SET_TICK_RATE = 0xC000, //Cxxx

	FUR_EFF_ARP_SPEED = 0xE000,
	FUR_EFF_SLIDE_UP_SEMITONES = 0xE100,
	FUR_EFF_SLIDE_DN_SEMITONES = 0xE200,
	FUR_EFF_SET_VIBRATO_DIRECTION = 0xE300, //00 = up & down, 01 = only up, 02 = only down, who the fuck thought it's useful to have anything different than up & down
	FUR_EFF_SET_VIBRATO_DEPTH = 0xE400, //1 step is 1/16th of semitone
	FUR_EFF_PITCH = 0xE500, //I made klystrack version exactly match this effect
	FUR_EFF_LEGATO = 0xEA00,
	FUR_EFF_SAMPLE_BANK = 0xEB00,
	FUR_EFF_NOTE_CUT = 0xEC00,
	FUR_EFF_NOTE_DELAY = 0xED00,
	
	FUR_EFF_SET_BPM = 0xF000,
	FUR_EFF_PORTA_UP_FINE = 0xF100,
	FUR_EFF_PORTA_DN_FINE = 0xF200,
	FUR_EFF_VOLUME_UP_FINE = 0xF300,
	FUR_EFF_VOLUME_DN_FINE = 0xF400,
	FUR_EFF_DISABLE_MACRO = 0xF500,
	FUR_EFF_ENABLE_MACRO = 0xF600,
	FUR_EFF_VOLUME_UP_FINE_1_TICK = 0xF800,
	FUR_EFF_VOLUME_DN_FINE_1_TICK = 0xF900,
	FUR_EFF_FAST_VOLUME_SLIDE = 0xFA00,
	FUR_EFF_STOP_SONG = 0xFF00,
};