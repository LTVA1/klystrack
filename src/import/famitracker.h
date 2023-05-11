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

#define FT_HEADER_SIG "FamiTracker Module"
#define DN_FT_HEADER_SIG "Dn-FamiTracker Module"
#define END_SIG "END"

typedef struct
{
	char name[FAMITRACKER_BLOCK_SIGNATURE_LENGTH + 1];
	Uint32 version;
	Uint32 length;
	
	Uint32 position; // position of actual data so 16+4+4 bytes after block start
} ftm_block;

int import_famitracker(FILE *f, int type);

#endif