#ifndef COPYPASTE_H
#define COPYPASTE_H

#include <stdbool.h>

#include "../klystron/src/snd/music.h"

/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)

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


typedef struct
{
	int start, end, keydown;
	int patternx_start, patternx_end;
	
	bool drag_selection; //if we are in mouse drag selection mode
	bool drag_selection_sequence;
	bool drag_selection_program;
	bool drag_selection_program_4op;
	
	int prev_start, prev_end, prev_patternx_start, prev_patternx_end; //so when we drag outside of our rect we start the selection, otherwise do nothing
	int channel;
	
	int prev_name_index;
	
} Selection;


void copy();
void paste();
void join_paste();
void cut();
void delete();
void begin_selection(int position);
void select_range(int position);

#endif
