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

#ifndef PLAINTEXT_H
#define PLAINTEXT_H

#pragma once

#define KLYSTRACK_PLAINTEXT_SIG "klystrack-plus"
#define KLYSTRACK_PATTERN_SEGMENT_SIG "pattern_segment"

#define KLYSTRACK_PLAINTEXT_NOTE_CUT "OFF"
#define KLYSTRACK_PLAINTEXT_NOTE_RELEASE "==="
#define KLYSTRACK_PLAINTEXT_NOTE_MACRO_RELEASE "M=="
#define KLYSTRACK_PLAINTEXT_NOTE_NOTE_RELEASE "N=="

void create_plain_text();

#endif