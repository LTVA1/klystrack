#pragma once

#ifndef HELP_H
#define HELP_H

#include "mused.h"
#include "gfx/gfx.h"

typedef struct
{
	int mode;
	int selected_line;
	const char *title;
	SliderParam scrollbar;
	char **lines;
	int n_lines;
	int list_position;
	int quit;
	const Font *largefont, *smallfont;
	GfxSurface *gfx;
	int elemwidth, list_width;
} Data;

int helpbox(const char *title, GfxDomain *domain, GfxSurface *gfx, const Font *largefont, const Font *smallfont);

#endif
