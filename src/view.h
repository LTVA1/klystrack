#ifndef VIEW_H
#define VIEW_H

#pragma once

/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2023 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (GfxDomain *dest_surface, the "Software"), to deal in the Software without
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

#include "gui/view.h"

#include "view/oscilloscope.h" //wasn't there

#include "../klystron/src/snd/freqs.h"

#define PLAYSTOP_INFO_W 78
#define SCROLLBAR 10

#define VIEW_WIDTH 166
#define TOP_VIEW_H 18
#define ALG_VIEW_H 130
#define SCROLLBAR_W 10

#define uppersig (Uint32)(mused.time_signature >> 8)
#define lowersig (Uint32)(mused.time_signature & 0xff)
#define compoundbeats (uppersig / 3)
#define compounddivider (lowersig)
#define simpletime(i, bar, beat, normal) ((((i) % (lowersig * uppersig) == 0) ? (bar) : ((i) % (lowersig) == 0) ? (beat) : (normal)))
#define compoundtime(i, bar, beat, normal) (((i) % (compoundbeats * 16 * 3 / compounddivider) == 0) ? (bar) : ((i) % (16 * 3 / compounddivider) == 0 ? (beat) : (normal)))
#define timesig(i, bar, beat, normal) (((uppersig != 3) && (uppersig % 3) == 0) ? compoundtime(i, bar, beat, normal) : simpletime(i, bar, beat, normal))
#define swap(a,b) { a ^= b; b ^= a; a ^= b; }


/*

Cyd envelope length in milliseconds

*/

#define envelope_length(slope) (slope!=0?(float)(((slope) * (slope) * 256 / (ENVELOPE_SCALE * ENVELOPE_SCALE))) / ((float)CYD_BASE_FREQ / 1000.0f) :0.0f)

void my_draw_view(const View* views, const SDL_Event *_event, GfxDomain *domain, int m);
int generic_field(const SDL_Event *e, const SDL_Rect *area, int focus, int param, const char *_label, const char *format, void *value, int width);
int generic_field_simple(const SDL_Event *e, const SDL_Rect *area, const char *_label, const char *format, void *value, int width);
void generic_flags(const SDL_Event *e, const SDL_Rect *_area, int focus, int p, const char *label, Uint32 *flags, Uint32 mask);
int generic_button(const SDL_Event *e, const SDL_Rect *area, int focus, int param, const char *_label, void (*action)(void*,void*,void*), void *p1, void *p2, void *p3);
char* notename(int note);
char* notename_default(int note);
void my_separator(const SDL_Rect *parent, SDL_Rect *rect);
void set_cursor(const SDL_Rect *location);
bool is_selected_param(int focus, int p);
float percent_to_dB(float percent);

/* 
"Controls"
*/

void song_name_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void instrument_name_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);

void instrument_disk_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void program_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);

void oscilloscope_view(GfxDomain *dest_surface, const SDL_Rect *dest1, const SDL_Event *event, void *param); //wasn't there
void four_op_menu_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void four_op_program_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);

void info_line(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void sequence_spectrum_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void pattern_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void songinfo1_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void songinfo2_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void songinfo3_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void playstop_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void instrument_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void instrument_view2(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void instrument_list(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void fx_name_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void fx_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void bevel_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void toolbar_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void fx_global_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void label(const char *_label, const SDL_Rect *area);
void inst_field(const SDL_Event *e, const SDL_Rect *area, int p, int length, char *text);

void fx_disk_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);
void wave_disk_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);

void local_sample_disk_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param);

bool check_mouse_hit(const SDL_Event *e, const SDL_Rect *area, int focus, int param);

char* my_itoa(int num, char *str);

#endif
