#pragma once

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
#define OSC_MAX_CLAMP ((8192 + 4096 + 2048 - 128) * OSC_SIZE / 128 - OSC_SIZE * 4)
#define TRIGGER_LEVEL 200

void update_oscillscope_view(GfxDomain *dest, const SDL_Rect* area);