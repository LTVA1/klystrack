#pragma once

#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include "wavetableview.h"
#include "../mused.h"
#include "../view.h"
#include "../event.h"
#include "gui/mouse.h"
#include "gui/dialog.h"
#include "gui/bevel.h"
#include "../theme.h"
#include "../mybevdefs.h"
#include "../action.h"
#include "../wave_action.h"

#define OSC_SIZE 150
//#define 
//#define SCALE_MULT 150
#define TRIGGER_LEVEL 400

void update_oscillscope_view(GfxDomain *dest, const SDL_Rect* area, int* sound_buffer, int size, int* buffer_counter, bool is_translucent, bool show_midlines);

#endif