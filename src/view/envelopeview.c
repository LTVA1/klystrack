/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2022 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

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

#include "envelopeview.h"

void point_envelope_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	if(!(mused.show_point_envelope_editor)) return;
	if(mused.show_four_op_menu) return;
	
	SDL_Rect area;
	copy_rect(&area, dest);
	console_set_clip(mused.console, &area);
	console_clear(mused.console);
	bevelex(domain, &area, mused.slider_bevel, BEV_THIN_FRAME, BEV_F_STRETCH_ALL);
	adjust_rect(&area, 2);
	
	gfx_domain_set_clip(domain, &area);
	
	adjust_rect(&area, 4);
	
	area.y -= mused.point_env_editor_scroll;
	
	bevelex(domain, &area, mused.slider_bevel, BEV_BACKGROUND, BEV_F_STRETCH_ALL);
	
	adjust_rect(&area, 4);
	
	console_set_clip(mused.console, &area);
	
	font_write_args(&mused.largefont, domain, &area, "test\ntest\ntest\ntest\ntest\ntest\n");
	
	slider_set_params(&mused.point_env_slider_param, 0, 2000, mused.point_env_editor_scroll, area.h + mused.point_env_editor_scroll, &mused.point_env_editor_scroll, 1, SLIDER_VERTICAL, mused.slider_bevel);
	
	gfx_domain_set_clip(domain, NULL);
}