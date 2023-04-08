#ifndef MUSED_H
#define MUSED_H

/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2022 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

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

#include "snd/music.h"
#include "gfx/font.h"
#include "console.h"
#include "gui/slider.h"
#include "edit.h"

enum
{
	EDITPATTERN,
	EDITSEQUENCE,
	EDITCLASSIC,
	EDITINSTRUMENT,
	
	EDIT4OP,
	
	EDITFX,
	EDITWAVETABLE,
	/* Focuses */
	EDITPROG,
	EDITPROG4OP,
	EDITSONGINFO,
	
	EDITENVELOPE,
	EDITENVELOPE4OP,
	
	EDITSONGMESSAGE,
	/* Virtual modes, i.e. what are not modes itself but should be considered happening "inside" prev_mode */
	EDITBUFFER,
	MENU
};

#define N_VIEWS EDITPROG
#define VIRTUAL_MODE EDITBUFFER

#include "clipboard.h"
#include "copypaste.h"
#include "gui/menu.h"
#include "undo.h"
#include <stdbool.h>
#include "wavegen.h"
#include "diskop.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define NUM_PATTERNS 4096
#define NUM_INSTRUMENTS 255
#define NUM_SEQUENCES 2048

#define GAME_MAX_BULLETS 4096

enum
{
	//flags
	COMPACT_VIEW = 1,
	SONG_PLAYING = 2,
	FULLSCREEN = 4,
	MULTICHANNEL_PREVIEW = 8,
	SHOW_PATTERN_POS_OFFSET = 16,
	FOLLOW_PLAY_POSITION = 32,
	ANIMATE_CURSOR = 64,
	HIDE_ZEROS = 128,
	DELETE_EMPTIES = 256,
	EDIT_MODE = 512,
	SHOW_ANALYZER = 2048,
#ifdef MIDI
	MIDI_SYNC = 4096,
#endif
	SHOW_LOGO = 8192,
	SHOW_DECIMALS = 16384,
	LOOP_POSITION = 32768,
	LOCK_SEQUENCE_STEP_AND_PATTERN_LENGTH = 65536,
	SHOW_DELAY_IN_TICKS = 65536 << 1,
	EDIT_SEQUENCE_DIGITS = 65536 << 2,
	EXPAND_ONLY_CURRENT_TRACK = 65536 << 3,
	DISABLE_NOSTALGY = 65536 << 4,
	TOGGLE_EDIT_ON_STOP = 65536 << 5,
	STOP_EDIT_ON_PLAY = 65536 << 6,
	MULTIKEY_JAMMING = 65536 << 7,
	DISABLE_VU_METERS = 65536 << 8,
	WINDOW_MAXIMIZED = 65536 << 9,
	SHOW_WAVEGEN = 65536 << 10,
	DISABLE_RENDER_TO_TEXTURE = 65536 << 11,
	DISABLE_BACKUPS = 65536 << 12,
	START_WITH_TEMPLATE = 65536 << 13,
	USE_SYSTEM_CURSOR = 65536 << 14,
	SHOW_OSCILLOSCOPE_INST_EDITOR = (Uint32)(65536 << 15),
	
	//flags2
	SHOW_OSCILLOSCOPES_PATTERN_EDITOR = 1,
	SHOW_OSCILLOSCOPE_MIDLINES = 2,
	SHOW_REGISTERS_MAP = 4,
	SHOW_BPM = 8,
	SMOOTH_SCROLL = 16,
	SHOW_FLATS_INSTEAD_OF_SHARPS = 32,
	HIGHLIGHT_COMMANDS = 64,
	
	DRAG_SELECT_PATTERN = 128,
	DRAG_SELECT_SEQUENCE = 256,
	DRAG_SELECT_PROGRAM = 512,
	DRAG_SELECT_4OP = 1024,
	
	SHOW_OLD_SPECTRUM_VIS = 2048,
	
	ENABLE_AUTOSAVE = 4096,
	SHOW_AUTOSAVE_MESSAGE = 8192,
};

//#define SHOW_OSCILLOSCOPES_PATTERN_EDITOR ((Uint64)65536 << 16)
//#define SHOW_OSCILLOSCOPE_MIDLINES ((Uint64)65536 << 17)
//#define SHOW_REGISTERS_MAP ((Uint64)65536 << 18)

//const Uint64 SHOW_OSCILLOSCOPES_PATTERN_EDITOR = ((Uint64)65536 << 16);

enum
{
	VC_INSTRUMENT = 1,
	VC_VOLUME = 2,
	VC_CTRL = 4,
	VC_COMMAND = 8
};

enum
{
	VIS_SPECTRUM = 0,
	VIS_CATOMETER = 1,
	VIS_NUM_TOTAL
};

typedef struct
{
	Uint64 flags;
	Uint64 flags2;
	Uint32 visible_columns;
	int done;
	Console *console;
	
	Unicode_console *info_console;
	
	MusSong song;
	CydEngine cyd;
	MusEngine mus;
	
	int octave, instrument_page, current_instrument, default_pattern_length, selected_param, env_selected_param, fourop_selected_param, fourop_env_selected_param, selected_operator, editpos, mode, focus,
		current_patternx, current_patternpos, current_sequencepos, sequenceview_steps, single_pattern_edit, 
		prev_mode, current_sequenceparam, instrument_list_position,
		pattern_position, sequence_position, pattern_horiz_position, sequence_horiz_position,
		program_position, current_program_step,
		edit_reverb_param, selected_wavetable, wavetable_param, songinfo_param,
		loop_store_length, loop_store_loop, note_jump, wavetable_list_position, wavetable_preview_idx, sequence_digit;
		
	int point_env_editor_scroll;
	
	int vol_env_horiz_scroll;
	int vol_env_scale; //-1 = 2x, 1 = 1x, 2 = 0.5x, 3 = 0.33x etc.
	
	int pan_env_horiz_scroll;
	int pan_env_scale; //-1 = 2x, 1 = 1x, 2 = 0.5x, 3 = 0.33x etc.
	
	int vol_env_point, pan_env_point;
	
	int prev_vol_env_x, prev_vol_env_y;
	int prev_pan_env_x, prev_pan_env_y;
	
	int fourop_point_env_editor_scroll;
	
	int fourop_vol_env_horiz_scroll;
	int fourop_vol_env_scale; //-1 = 2x, 1 = 1x, 2 = 0.5x, 3 = 0.33x etc.
	
	int fourop_vol_env_point;
	
	int fourop_prev_vol_env_x, fourop_prev_vol_env_y;
	
	int fourop_program_position[CYD_FM_NUM_OPS];
	
	int current_instrument_program;
	int current_fourop_program[CYD_FM_NUM_OPS];
	
	int current_volume_envelope_point;
	int current_panning_envelope_point;
	
	int fourop_current_volume_envelope_point;
	
	int current_sequencetrack;
	Uint16 time_signature;
	Clipboard cp;
	char * edit_buffer;
	
	Uint8 unite_bits_buffer[MUS_PROG_LEN]; //wasn't there
	Uint8 unite_bits_to_paste;
	Uint8 paste_pointer;
	
	int edit_buffer_size;
	SliderParam sequence_slider_param, pattern_slider_param, program_slider_param, instrument_list_slider_param, 
		pattern_horiz_slider_param, sequence_horiz_slider_param, wavetable_list_slider_param, four_op_slider_param, point_env_slider_param, vol_env_horiz_slider_param, fourop_vol_env_horiz_slider_param, pan_env_horiz_slider_param;
	char previous_song_filename[1000], previous_export_filename[1000], previous_filebox_path[OD_T_N_TYPES][1000];
	/*---*/
	char * edit_backup_buffer;
	Selection selection;
	/* -- stat -- */
	int stat_song_position;
	int stat_pattern_position[MUS_MAX_CHANNELS];
	MusPattern *stat_pattern[MUS_MAX_CHANNELS];
	MusChannel *channel;
	int stat_pattern_number[MUS_MAX_CHANNELS], stat_note[MUS_MAX_CHANNELS];
	Uint64 time_played, play_start_at;
	/* ---- */
	char info_message[256];
	SDL_TimerID info_message_timer;
	GfxSurface *slider_bevel, *vu_meter, *analyzer, *logo, *catometer;
	Font smallfont, largefont, tinyfont, tinyfont_sequence_counter, tinyfont_sequence_normal;
	
	Font font16, font24; //16x16 and 24x24 pixels respectively
	
	SDL_Cursor *mouse_cursor;
	GfxSurface *mouse_cursor_surface;
	GfxSurface *icon_surface;
	
	GfxSurface *MIKEYCHAD_logic_icons; //for XOR, XNOR, NAND, NOR elements custom icons
	
	/* for menu */
	Font menufont, menufont_selected, headerfont, headerfont_selected, shortcutfont, shortcutfont_selected, buttonfont;
	
	char themename[100], keymapname[100];
	int pixel_scale;
	int mix_rate, mix_buffer;
	int window_w, window_h;
	int fx_bus, fx_room_size, fx_room_vol, fx_room_dec, fx_tap, fx_axis, fx_room_ticks, fx_room_prev_x, fx_room_prev_y;
	
	/*---vis---*/
	
	int current_visualizer;
	struct 
	{
		int cyd_env[MUS_MAX_CHANNELS];
		int spec_peak[256], spec_peak_decay[96];
		float prev_a;
	} vis;
	
	//float real_buffer[8192];
	//float imaginary_buffer[8192];
	
	float* real_buffer;
	float* imaginary_buffer;
	
	//float* test;
	
	/*---*/
	
	SDL_Rect cursor_target, cursor;
	
	/*----------*/
	
	UndoStack undo;
	UndoStack redo;
	SHType last_snapshot;
	int last_snapshot_a, last_snapshot_b;
	bool modified;
	
	/*------------*/
	
	GfxSurface *wavetable_preview;
	
	Uint16 wavetable_bits;
	int prev_wavetable_x, prev_wavetable_y;
	
#ifdef MIDI
	Uint32 midi_device;
	Uint8 midi_channel;
	bool midi_start;
	Uint16 midi_rate;
	Uint32 midi_last_clock;
	Uint8 tick_ctr;
#endif

	WgSettings wgset;
	int selected_wg_osc, selected_wg_preset;
	
	int oversample;
	
	/* Oscilloscope stuff */
	
	int output_buffer[8193]; //wasn't there
	Uint16 output_buffer_counter;
	
	Uint16 spectrum_output_buffer_counter;
	
	int channels_output_buffers[MUS_MAX_CHANNELS][8193]; //wasn't there
	int channels_output_buffer_counters[MUS_MAX_CHANNELS];
	
	/* For expand-collapse of additional command columns */
	
	int widths[MUS_MAX_CHANNELS][2]; //[0] normal width, [1] narrow width
	
	/*struct 
	{
		struct
		{
			Sint16 x, y;
			Uint8 speed_x, speed_y, tile_index;
			bool is_attracted;
			Uint8 attraction_level;
		} bullets[GAME_MAX_BULLETS];
		
		Sint16 player_x, player_y, enemy_x, enemy_y;
		Uint8 player_state, enemy_state;
		bool is_player_dead, is_enemy_dead;
		
		GfxSurface *game, *game_menu;
		
		Uint32 current_score, high_scores[20];
	} game;*/
	
	bool show_four_op_menu;
	
	Sint8 draw_passes_since_song_start;
	int fps;
	
	Uint8 frames_since_menu_close;
	bool jump_in_pattern; //after mouse drag selection ends we don't need the cursor to jump to the end
	bool jump_in_sequence;
	bool jump_in_program;
	bool jump_in_program_4op;
	
	bool import_mml_string;
	
	Uint32 time_between_autosaves; //in ms
	
	bool show_point_envelope_editor;
	bool show_4op_point_envelope_editor;
} Mused;

extern Mused mused;
extern GfxDomain *domain;
extern Uint32 pattern_color[16];

void default_settings();
void change_mode(int newmode);
void clear_pattern(MusPattern *pat);
void clear_pattern_range(MusPattern *pat, int first, int last);
void init(MusInstrument *instrument, MusPattern *pattern, MusSeqPattern** sequence, MusChannel *channel);//void init(MusInstrument *instrument, MusPattern *pattern, MusSeqPattern sequence[MUS_MAX_CHANNELS][NUM_SEQUENCES], MusChannel *channel);
void deinit();
void new_song();
void kt_default_instrument(MusInstrument *instrument);
void set_edit_buffer(char *buffer, size_t size);
void change_pixel_scale(void *a, void*b, void*c);
void mirror_flags();
void resize_pattern(MusPattern * pattern, Uint16 new_size);
void init_scrollbars();
void my_open_menu();
int viscol(int col);
void post_config_load();
void enable_callback(bool state);
int get_pattern(int abspos, int track);
int get_patternstep(int abspos, int track);
int current_pattern();
int current_pattern_for_channel(int channel);
int current_patternstep();
MusStep * get_current_step();
MusPattern * get_current_pattern();
void change_visualizer(int vis);
void set_info_message(const char *string, ...)  __attribute__ ((format (printf, 1, 2)));
void set_channels(int channels);
Uint32 get_playtime_at(int position);

#endif
