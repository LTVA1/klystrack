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

#ifndef IMPORTUTIL_H
#define IMPORTUTIL_H

#include <stdio.h>
#include "../edit.h"
#include "../mused.h"
#include "../action.h"
#include "../event.h"
#include "SDL_endian.h"
#include "../view.h"

char* citoa(int num, char* str, int base);
void read_uint32(FILE *f, Uint32* number); //little-endian
int checkbox_simple(GfxDomain *dest, const SDL_Event *event, const SDL_Rect *area, GfxSurface *gfx, const Font * font, int offset, int offset_pressed, int decal, const char* _label, Uint32 *flags, Uint32 mask);
void generic_flags_simple(const SDL_Event *e, const SDL_Rect *_area, const char *label, Uint32 *_flags, Uint32 mask);
Uint8 find_empty_command_column(MusStep* step);
void find_command_xm(Uint16 command, MusStep* step);
void find_command_s3m(Uint16 command, MusStep* step);
Uint16 find_command_pt(Uint16 command, int sample_length);
void convert_volume_and_command_xm(Uint16 command, Uint8 volume, MusStep* step);

#endif