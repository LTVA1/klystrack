#include "help.h"

#include "gui/msgbox.h"
#include "gui/bevel.h"
#include "gui/bevdefs.h"
#include "gui/dialog.h"
#include "gfx/gfx.h"
#include "gui/view.h"
#include "gui/mouse.h"
#include "gui/toolutil.h"
#include "gui/slider.h"
#include "command.h"
#include "shortcutdefs.h"
#include <string.h>

#define SCROLLBAR 10
#define TOP_LEFT 0
#define TOP_RIGHT 0
#define MARGIN 8
#define SCREENMARGIN 8 //#define SCREENMARGIN 32
#define TITLE 14
#define FIELD 14
#define CLOSE_BUTTON 12
#define ELEMWIDTH data.elemwidth
#define LIST_WIDTH data.list_width
#define BUTTONS 16



Data data;

extern const KeyShortcut shortcuts[];

static void help_list_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);
static void title_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);
static void window_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);
static void buttons_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param);

static const View filebox_view[] =
{
	{{ SCREENMARGIN, SCREENMARGIN, -SCREENMARGIN, -SCREENMARGIN }, window_view, &data, -1},
	{{ MARGIN+SCREENMARGIN, SCREENMARGIN+MARGIN, -MARGIN-SCREENMARGIN, TITLE - 2 }, title_view, &data, -1},
	{{ -SCROLLBAR-MARGIN-SCREENMARGIN, SCREENMARGIN+MARGIN + TITLE, SCROLLBAR, -MARGIN-SCREENMARGIN-BUTTONS }, slider, &data.scrollbar, -1},
	{{ SCREENMARGIN+MARGIN, SCREENMARGIN+MARGIN + TITLE, -SCROLLBAR-MARGIN-1-SCREENMARGIN, -MARGIN-SCREENMARGIN-BUTTONS }, help_list_view, &data, -1},
	{{ SCREENMARGIN+MARGIN, -SCREENMARGIN-MARGIN-BUTTONS+2, -MARGIN-SCREENMARGIN, BUTTONS-2 }, buttons_view, &data, -1},
	{{0, 0, 0, 0}, NULL}
};


static void deinit_lines()
{
	for (int i = 0; i < data.n_lines; ++i)
		if (data.lines[i]) free(data.lines[i]);
		
	free(data.lines);
	data.lines = NULL;
	data.n_lines = 0;
}

const char* comments[] = {
	"",
	"",
	"                  ==== K L Y S T R A C K - P L U S ====",
	"",
	"",
	"So, this is klystrack fork which adds a bunch of new stuff. But, will you ask,",
	"why would you need something new in this tracker? It already is kinda good,",
	"no?",
	"",
	"And I will answer that of course yes. I've used original klystrack for almost",
	"a year, and despite my absolute inability in all connected with music, I",
	"believe that klystrack (for a tracker) is very good program which is easy to",
	"use and understand. But, to my surprise, it lacks some additional capabilities",
	"such as combined SID waves and 4-op FM synth, which I, of course, can't use to",
	"create something beautiful, but I want other people to have some kind of",
	"universal tracker where they would be able to compose for almost any chip at",
	"the same time. Of course, there is E-Famitracker as kind of \"one tracker to",
	"rule them all\", but its channels have fixed capabilities and purpose. E.g.",
	"NES square channel only has 16-step linear volume and only 4 fixed pulse",
	"widths (btw that was the reason I started searching for different tracker",
	"which ended with me discovering klystrack), and it does not have functions of",
	"the noise or DPCM channel, and so on. On the other hand, in klystrack you",
	"already have a lot of channels and every channel can do anything other channel",
	"can do, which is cool. And there even was 2-op FM! And I started digging it,",
	"and found that there are other beautiful chips like OPL3 and OPN family.",
	"And sadly klystrack lacked their specific functions, so one day I remembered",
	"that somewhen I studied a bit of programming and so I forked this tracker and",
	"started to add new functions.",
	"",
	"Of course, now these are minor additions, but with each I better understand",
	"how tracker works, and so I can add more complex stuff in the future. 4-op is",
	"already there and other stuff is on its way too!",
	//"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xad\xa8\xa9\xaa\xab",
	//   ░   ▒   ▓   █   │   ┤   ╡   ╢   ╖   ╕   ╣   ║   ╗   ╝   ╜   ╛   ┐   └   ┴   ┬   ├   ─   ┼   ╞   ╟   ╚   ╔   ╩   ╦   ╠   ═   ╬   ╧   ╨   ╤   ╥   ╙   ╘   ╒   ╓   ╫   ╪   ┘   ┌
	
	
	//" \xae  \xaf    \xb0       \xb1     \xb2     \xb3       \xb4     \xb5",
	//1dig  1dig     1dig       1dig     arrow1    arrow2   empty     full
	//ptr  open p   between     close p   in        in      check     check
	//num  tr num   ptr num     tr num    ptr       ptr      box      box
	"",
	"                                                                       -- LTVA",
	"",
	"                        = TUTORIAL ABOUT NEW FEATURES =                         ",
	"",
	"\x9a\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x9e\x8c \n\x8b\BASE \xaf\xb0\xb1\xb2\xb3\xaf\xb0\xb1\xb2\xb3 \xb4L  \x8b \n\x8b\xb5\DRUM      \xb5KSYNC   \x8b \n\x8b\xb4VIB       \xb5SET PW  \x8b \n\x8b\xb5SET CUT   SLIDE\xaf\xb1\xb2\xb3\x8b \n\x98\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x87 \n\x8b\xb4PUL \xaf\xb0\xb1\xb2\xb3 \xb4SAW     \x8b \n\x8b\xb5TRI \xb4NOI  \xb4METAL   \x8b \n\x8b\xb4POKEY \xae\xb2\xb3 \xb4 1/4TH  \x8b \n\x98\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x87 \n\x8b\xb4WAVE\xaf\xb1\xb2\xb3 \xb4OENV \xb4L  \x8b \n\x98\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x87 \n\x8bOSC. MIX MODE \xaf\xb0\xb1\xb2\xb3 \x8b \n\x98\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x87 \n\x8bVOL \xaf\xb1\xb2\xb3 \xb4RELATIVE  \x8b \n\x8b\ATK \xaf\xb1\xb2\xb3 DEC \xaf\xb1\xb2\xb3   \x8b \n\x8bSUS \xaf\xb1\xb2\xb3 REL \xaf\xb1\xb2\xb3   \x8b \n\x98\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x87 \n\x8b\xb4\BUZZ  DETUNE\xaf\xb1\xb2\xb3   \x8b \n\x8bSHAPE\xae\xb2\xb3 FINE\xaf\xb0\xb1\xb2\xb3  \x8b \n\x98\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x87 \n\x8b\xb4SYNC     SRC \xaf\xb1\xb2\xb3  \x8b \n\x8b\xb4RING MOD SRC \xaf\xb1\xb2\xb3  \x8b \n\x98\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x95\x87 \n\x8b\xb4\FILTER  TYPE\xaf\xb0\xb1\xb2\xb3  \x8b \n\x8b\CUT\xaf\xb0\xb1\xb2\xb3 RES   \xae\xb2\xb3  \x8b \n\x8bSLOPE \xaf\xb0\xb0\xb0\xb0\xb0\xb0\xb0\xb0\xb1\xb2\xb3  \x8b",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"Well, here you can see your instrument editor main section. Let's talk about",
	"what was added and what it does. First of all is \"OSC. MIX MODE\" setting. It",
	"basically changes how signals from your instrument oscillators (PULSE, TRI, SAW,",
	"etc.) are mixed before resulting signal is supplied into output buffer and in",
	"your ears.",
	"",
	"In vanilla klystrack these signals (16-bit unsigned int's) were bitwise",
	"ANDed (except WAVE (sample) signal)      _______________________________",
	"together. So, take a look if you don't  |0 0 1 1 0 1 1 0 0 1 0 1 1 0 1 1|",
	"know how bitwise AND works. As you      |1 1 1 0 0 1 1 1 1 0 1 0 0 1 1 1|",
	"could already have guessed, it is not   |=============(AND)=============|",
	"the \"traditional\" mixing which          |0 0 1 0 0 1 1 0 0 0 0 0 0 0 1 1|",
	"happens in real world or in your        |_______________________________|",
	"favourite DAW. In klystrack such",
	"mixing happened only between channels",
	"when tracker was forming final output. As I guess, bitwise AND was added because",
	"it is the mixing that SID should do on single channel. Anyway, I found that there",
	"are some interesting waves (e.g.",
	"PULSE + TRI wave (used by Fearofdark            /|    /|    /|    /|      ",
	"himself! although only as sample))             / |   / |   / |   / |      ",
	"and C64 SID combined waves which                 |  |  |  |  |  |  |  |   ",
	"have very interesting fractal shape              |  |  |  |  |  |  |  |   ",
	"and unique sound. So I added a bunch              \\ |   \\ |   \\ |   \\ |",
	"of new mixing modes. The first one                 \\|    \\|    \\|    \\|",
	"is \"SUM\" mode which does the exact",
	"same thing as if your oscillators were             P U L S E + T R I",
	"playing on different channels with the",
	"same note set, except the volume of resulting signal is divided by number",
	"of enabled oscillators (so pulse+tri is divided by 2, pulse+tri+noise by 3",
	"etc.) so perceived volume stays around the same.",
	"",
	"The next mode is bitwise OR. It does precisely what it name means. Not many",
	"interesting waves, but you can do something interesting with it.",
	"",
	"C64 mode adds combined 8580 SID waves. The most used one is pulse+tri. To",
	"make it produce sound you need to set your pulse width to 000 for full wave",
	"and to somewhere between 000 and 7ff to have only part of the wave. Other",
	"waves can be affected by pulse width too. Pulse+tri wave is not very close",
	"to original (at least in v0.7) because all waves are generated algorithmically",
	"each time you launch klystrack and in jsSID emulator where I stole these waves",
	"there is no pulse+tri generation so I had to figure it out myself and with ugly",
	"postprocessing I finally got something similar to original wave, although on",
	"oscilloscope it is not very close to original.",
	"",
	"XOR mode does XOR. With some settings you can get interesting fractal waves.",
	"",
	"The next here is filter slope. It means how hard filter suppresses frequencies",
	"around cutoff frequency which are not falling into range of frequencies filter",
	"have to leave as is. In lowpass filter these are the frequencies higher than",
	"cutoff frequency.",
	"",
	"The higher dB/oct number is, the harder filter filters these frequencies. If",
	"you increase this number, you get kinda the same effect as decreasing cutoff",
	"frequency, but there is some difference which may be useful in some cases.",
	"Basically higher number there means that inside klystrack-plus sound engine",
	"you have a chain of 1, 2, 4, 8, 16 or 32 filters. If you want more, just ask",
	"about it lol.",
	"",
	"Then a bit about new filter modes. These are just sums of signals of different",
	"filters divided by a number of modes these filters have. For example LHP mode",
	"means you have signals from lowpass and highpass filters. Their signals are",
	"summed and result is divided by 2.",
	"",
	/*"      ,-.  ,--,                       ___                                    ,-.",
	"  ,--/ /|,--.'|                     ,--.'|_                              ,--/ /|",
	",--. :/ ||  | :                     |  | :,'  __  ,-.                  ,--. :/ |",
	":  : ' / :  : '            .--.--.  :  : ' :,' ,'/ /|                  :  : ' /",
	"|  '  /  |  ' |       .--,/  /    .;__,'  / '  | |' |,--.--.     ,---. |  '  /",
	"'  |  :  '  | |     /_ ./|  :  /`.|  |   |  |  |   ,/       \\   /     \\'  |  :  ",
	"|  |   \\ |  | :  , ' , ' |  : ;_ :__,' | :  '  :  /.--.  .-. | /    / '|  |   \\ ",
	"'  : |. \\'  : |_/___/ \\: |\\  \\    `.'  : |__|  | '  \\__\\/: . ..   ' / ': |.    \\ ",
	"|  | ' \\ |  | '.'.  \\  ' | `----.   |  | '.';  : |  ,| .--.; |'  ; :__|| ' \\     \\ ",
	"'  : |--';  :   ;\\ ;   :/  /`--' ;  :    |  ,; /  /  ,.  |'   | '.''  : |--'", 
	";  |,'   |  ,   /  \\  \\  '--'.     /|  ,   / ---';  :   .'   |   :   ;  |,'", 
	"'--'      ---`-'    :  \\  \\`--'---'  ---`-'       |  ,     .-./\\   \\  /'--'", 
	"                     \\  ';                        `--`---'     `----'",
	"                      `--`",*/
	"",
	
	
	/*"      ,-.  ,--,                       ___                                    ,-.",
	"  ,--/ /|,--.'|                     ,--.'|_                              ,--/ /|",
	",--. :/ ||  | :                     |  | :,'  __  ,-.                  ,--. :/ |",
	":  : ' / :  : '            .--.--.  :  : ' :,' ,'/ /|                  :  : ' / ",
	"|  '  /  |  ' |       .--,/  /    .;__,'  / '  | |' |,--.--.     ,---. |  '  /  ",
	"'  |  :  '  | |     /_ ./|  :  /`.|  |   |  |  |   ,/       \   /     \'  |  :  ",
	"|  |   \ |  | :  , ' , ' |  : ;_ :__,'| :  '  :  /.--.  .-. | /    / '|  |   \ ",
	"'  : |. \'  : |_/___/ \: |\  \    `.'  : |__|  | '  \__\/: . ..    ' / '  : |. \ ",
	"|  | ' \ |  | '.'.  \  ' | `----.   |  | '.';  : |  ,\" .--.; |'  ; :__|  | '\ \ ",
	"'  : |--';  :   ;\ ;   :/  /`--' ;  :    |  ,; /  /  ,.  |'   | '.''  : |--' ", 
	";  |,'   |  ,   /  \  \  '--'.     /|  ,   / ---';  :   .'   |   :   ;  |,'    ", 
	"'--'      ---`-'    :  \  \`--'---'  ---`-'       |  ,     .-./\   \  /'--'      ", 
	"                    \  ';                        `--`---'     `----'            ",
	"                     `--`                                                        ",
	"",*/
	NULL
};

static void init_lines(void * section, void * unused1, void * unused2)
{
	deinit_lines();
	
	data.mode = CASTPTR(int, section);
	
	switch (data.mode)
	{
		case 0:
		{
			data.n_lines = 0;
			
			const InstructionDesc *commands = list_all_commands();
			
			for (const InstructionDesc *d = commands; d->name; ++d)
				++data.n_lines;
				
			data.lines = realloc(data.lines, sizeof(*data.lines) * data.n_lines);
		
			for (int i = 0; i < data.n_lines; ++i)
			{
				int params = 0;
				char paramstr[] = "xxxx";
		
				while ((~0 << (params * 4 + 4) & commands[i].mask) == commands[i].mask)
					++params;
			
				paramstr[params] = '\0';
	
				char buffer[500];
			
				snprintf(buffer, sizeof(buffer), "%0*X%s  %s", 4-params, commands[i].opcode >> (params * 4), paramstr, commands[i].name);
				
				if (strlen(buffer) > LIST_WIDTH / data.smallfont->w - 4)
				{
					strcpy(&buffer[LIST_WIDTH / data.smallfont->w - 4], "...");
				}
				
				data.lines[i] = strdup(buffer);
			}
		}
		break;
		
		case 1:
		{
			data.n_lines = 0;
			
			for (const KeyShortcut *s = shortcuts; s->action; ++s)
				if (s->description) ++data.n_lines;
				
			data.lines = realloc(data.lines, sizeof(*data.lines) * data.n_lines);
		
			for (int i = 0; i < data.n_lines; ++i)
			{
				if (shortcuts[i].description)
				{
					char buffer[500];
				
					snprintf(buffer, sizeof(buffer), "%-10s  %s", get_shortcut_string(&shortcuts[i]), shortcuts[i].description);
					
					if (strlen(buffer) > LIST_WIDTH / data.smallfont->w - 4)
					{
						strcpy(&buffer[LIST_WIDTH / data.smallfont->w - 4], "...");
					}
					
					data.lines[i] = strdup(buffer);
				}
			}
		}
		break;
		
		case 2:
		{
			data.n_lines = 0;
			
			while(comments[data.n_lines] != NULL)
				++data.n_lines;
				
			data.lines = realloc(data.lines, sizeof(*data.lines) * data.n_lines);
		
			for (int i = 0; i < data.n_lines; ++i)
			{
				char buffer[50000];
				
				snprintf(buffer, sizeof(buffer), "%s", comments[i]);
					
				/*if (strlen(buffer) > LIST_WIDTH / data.smallfont->w - 4)
				{
					strcpy(&buffer[LIST_WIDTH / data.smallfont->w - 4], "...");
				}*/
					
				data.lines[i] = strdup(buffer);
			}
		}
		break;
	}
}


static void buttons_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	SDL_Rect button;
	
	copy_rect(&button, area);
	
	button.w = strlen("Commands") * data.smallfont->w + 12;
	
	button_text_event(dest_surface, event, &button, data.gfx, data.smallfont, data.mode == 0 ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "Commands", init_lines, 0, 0, 0);
	button.x += button.w + 1;

	button.w = strlen("Shortcuts") * data.smallfont->w + 12;
	button_text_event(dest_surface, event, &button, data.gfx, data.smallfont, data.mode == 1 ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "Shortcuts", init_lines, MAKEPTR(1), 0, 0);
	button.x += button.w + 1;
	
	button.w = strlen("Authors' comments") * data.smallfont->w + 12;
	button_text_event(dest_surface, event, &button, data.gfx, data.smallfont, data.mode == 2 ? BEV_BUTTON_ACTIVE : BEV_BUTTON, BEV_BUTTON_ACTIVE, "Authors' comments", init_lines, MAKEPTR(2), 0, 0);
	button.x += button.w + 1;
}


void window_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	bevel(dest_surface, area, data.gfx, BEV_MENU);
}


void title_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	const char* title = data.title;
	SDL_Rect titlearea, button;
	copy_rect(&titlearea, area);
	titlearea.w -= CLOSE_BUTTON - 4;
	copy_rect(&button, area);
	adjust_rect(&button, titlearea.h - CLOSE_BUTTON);
	button.w = CLOSE_BUTTON;
	button.x = area->w + area->x - CLOSE_BUTTON;
	font_write(data.largefont, dest_surface, &titlearea, title);
	if (button_event(dest_surface, event, &button, data.gfx, BEV_BUTTON, BEV_BUTTON_ACTIVE, DECAL_CLOSE, NULL, MAKEPTR(1), 0, 0) & 1)
		data.quit = 1;
}


void help_list_view(GfxDomain *dest_surface, const SDL_Rect *area, const SDL_Event *event, void *param)
{
	SDL_Rect content, pos;
	copy_rect(&content, area);
	adjust_rect(&content, 1);
	copy_rect(&pos, &content);
	pos.h = data.largefont->h;
	bevel(dest_surface, area, data.gfx, BEV_FIELD);
	
	gfx_domain_set_clip(dest_surface, &content);
	
	for (int i = data.list_position; i < data.n_lines && pos.y < content.h + content.y; ++i)
	{
		if (data.selected_line == i)
		{
			bevel(dest_surface, &pos, data.gfx, BEV_SELECTED_ROW);
		}
	
		font_write(data.smallfont, dest_surface, &pos, data.lines[i]);
		
		if (pos.y + pos.h <= content.h + content.y) slider_set_params(&data.scrollbar, 0, data.n_lines - 1, data.list_position, i, &data.list_position, 1, SLIDER_VERTICAL, data.gfx);
		
		//check_event(event, &pos, pick_file_action, MAKEPTR(i), 0, 0);
		
		update_rect(&content, &pos);
	}
	
	gfx_domain_set_clip(dest_surface, NULL);
	
	//check_mouse_wheel_event(event, area, &data.scrollbar);
}


int helpbox(const char *title, GfxDomain *domain, GfxSurface *gfx, const Font *largefont, const Font *smallfont)
{
	set_repeat_timer(NULL);
	
	memset(&data, 0, sizeof(data));
	data.title = title;
	data.largefont = largefont;
	data.smallfont = smallfont;
	data.gfx = gfx;
	data.elemwidth = domain->screen_w - SCREENMARGIN * 2 - MARGIN * 2 - 16 - 2;
	data.list_width = domain->screen_w - SCREENMARGIN * 2 - MARGIN * 2 - SCROLLBAR - 2;
	
	init_lines(0, 0, 0);
	
	slider_set_params(&data.scrollbar, 0, data.n_lines - 1, data.list_position, 0, &data.list_position, 1, SLIDER_VERTICAL, data.gfx);
	
	/*for (int i = 0; i < data.n_files; ++i)
	{
		if (strcmp(data.files[i].name, last_picked_file) == 0)
		{
			data.selected_file = i;
			
			// We need to draw the view once so the slider gets visibility info
			
			
			SDL_Event e = {0};
			
			draw_view(gfx_domain_get_surface(domain), filebox_view, &e);
			slider_move_position(&data.selected_file, &data.list_position, &data.scrollbar, 0);
			break;
		}
	}*/
	
	while (!data.quit)
	{
		SDL_Event e = { 0 };
		int got_event = 0;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				case SDL_QUIT:
				
				set_repeat_timer(NULL);
				SDL_PushEvent(&e);
				
				mused.frames_since_menu_close = 0;
				
				deinit_lines();
				return 0;
				
				break;
				
				case SDL_KEYDOWN:
				{
					switch (e.key.keysym.sym)
					{
						case SDLK_F1:
						case SDLK_ESCAPE:
						
						set_repeat_timer(NULL);
						deinit_lines();
						return 0;
						
						break;
						
						/*case SDLK_KP_ENTER:
						case SDLK_RETURN:
						if (data.selected_file != -1) data.picked_file = &data.files[data.selected_file];
						else goto enter_pressed;
						break;*/
						
						case SDLK_DOWN:
						slider_move_position(&data.selected_line, &data.list_position, &data.scrollbar, 1);
						break;
						
						case SDLK_UP:
						slider_move_position(&data.selected_line, &data.list_position, &data.scrollbar, -1);
						break;
						
						case SDLK_PAGEUP:
						case SDLK_PAGEDOWN:
						{
							int items = data.scrollbar.visible_last - data.scrollbar.visible_first;
							
							if (e.key.keysym.sym == SDLK_PAGEUP)
								items = -items;
							
							slider_move_position(&data.selected_line, &data.list_position, &data.scrollbar, items);
						}
						break;
						
						default: break;
					}
				
				
				}
				break;
			
				case SDL_USEREVENT:
					e.type = SDL_MOUSEBUTTONDOWN;
				break;
				
				case SDL_MOUSEMOTION:
					if (domain)
					{
						e.motion.xrel /= domain->scale;
						e.motion.yrel /= domain->scale;
						e.button.x /= domain->scale;
						e.button.y /= domain->scale;
					}
				break;
				
				case SDL_MOUSEBUTTONDOWN:
					if (domain)
					{
						e.button.x /= domain->scale;
						e.button.y /= domain->scale;
					}
				break;
				
				case SDL_MOUSEBUTTONUP:
				{
					if (e.button.button == SDL_BUTTON_LEFT)
						mouse_released(&e);
				}
				break;
			}
			
			if (e.type == SDL_MOUSEWHEEL)
			{
				if (e.wheel.y > 0)
				{
					data.list_position -= 2;
					*(data.scrollbar.position) -= 2;
				}
				
				else
				{
					data.list_position += 2;
					*(data.scrollbar.position) += 2;
				}
				
				data.list_position = my_max(0, my_min((data.n_lines - 1) - ((domain->window_h - (33 + 30)) / 18), data.list_position));
				*(data.scrollbar.position) = my_max(0, my_min((data.n_lines - 1) - ((domain->window_h - (33 + 30)) / 18), *(data.scrollbar.position)));
			}
			
			if (e.type != SDL_MOUSEMOTION || (e.motion.state)) ++got_event;
			
			// Process mouse click events immediately, and batch mouse motion events
			// (process the last one) to fix lag with high poll rate mice on Linux.
			//fix from here https://github.com/kometbomb/klystrack/pull/300
			if (should_process_mouse(&e))
				break;
		}
		
		if (got_event || gfx_domain_is_next_frame(domain))
		{
			draw_view(domain, filebox_view, &e);
			gfx_domain_flip(domain);
		}
		else
			SDL_Delay(5);
	}
	
	deinit_lines();
	
	mused.frames_since_menu_close = 0;
	
	return 0;
}

