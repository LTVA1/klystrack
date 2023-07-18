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

#ifndef HUBBARD_H
#define HUBBARD_H

#include <stdio.h>

typedef struct
{
	unsigned short int org;
	unsigned char data[65536];
	struct
	{
		unsigned short int songtab;
		unsigned short int patternptrhi, patternptrlo;
		unsigned short int songs[16][3];
		unsigned short int instruments;
	} addr;
	
	// decoded data
	
	struct { 
		unsigned short int pulse_width;
		unsigned char waveform;
		unsigned char a, d, s, r;
		unsigned char vibrato_depth;
		unsigned char pwm;
		unsigned char fx;
	} instrument[256];
	
	struct {
		struct {
			unsigned char note;
			unsigned char length;
			unsigned char legato;
			unsigned char instrument;
			char portamento;
		} note[256];
		int length;
	} pattern[256];
	
	int n_patterns;
	
	struct
	{
		unsigned char pattern[256];
		int length;
	} track[3];
	
	int n_tracks;
	int n_subsongs, n_instruments;
	int vib_type;
} hubbard_t;

int import_hubbard(FILE *f);

#endif
