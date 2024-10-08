#ifndef ACTION_H
#define ACTION_H

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

void select_sequence_position(void *channel, void *position, void *);
void select_pattern_param(void *id, void *position, void *pattern);
void select_instrument_param(void *idx, void *, void *);
void select_instrument(void *idx, void *relative, void *pagey);
void select_wavetable(void *idx, void *, void *);
void select_local_sample(void *idx, void *unused1, void *unused2);
void select_local_sample_note(void *idx, void *unused1, void *unused2);
void select_instrument_page(void *page, void *relative, void *);
void change_default_pattern_length(void *length, void *, void *);
void select_program_step(void *idx, void *, void *);
void change_octave(void *delta, void *, void *);
void change_song_rate(void *delta, void *, void *);
void change_time_signature(void *beat, void *, void *);
void play(void *from_cursor, void*, void*);
void play_position(void *, void*, void*);
void play_one_row(void *, void*, void*);
void stop(void*,void*,void*);
void change_song_speed(void *speed, void *delta, void *);
void new_song_action(void *, void *, void *);
void kill_instrument(void *, void *, void *);
void generic_action(void *func, void *, void *);
void quit_action(void *, void *, void *);
void change_mode_action(void *mode, void *from_shortcut, void *);
void enable_channel(void *channel, void *, void *);

void expand_command(int channel, void *, void *);
void hide_command(int channel, void *, void *);

void solo_channel(void *channel, void *, void *);
void enable_reverb(void *unused1, void *unused2, void *unused3);
void select_all(void *, void *, void*);
void clear_selection(void *, void *, void*);
void cycle_focus(void *views, void *focus, void *mode);
void change_song_length(void *delta, void *, void *);
void change_loop_point(void *delta, void *, void *);
void change_seq_steps(void *delta, void *, void *);
void change_timesig(void *delta, void *, void *);
void show_about_box(void *unused1, void *unused2, void *unused3);
void change_channels(void *delta, void *unused1, void *unused2);
void change_master_volume(void *delta, void *unused1, void *unused2);
void begin_selection_action(void *unused1, void *unused2, void *unused3);
void end_selection_action(void *unused1, void *unused2, void *unused3);
void toggle_pixel_scale(void *, void*, void*);
void change_pixel_scale(void *scale, void*, void*);
void toggle_fullscreen(void *a, void*b, void*c);
void change_fullscreen(void *a, void*b, void*c);
void toggle_render_to_texture(void *a, void*b, void*c);
void change_render_to_texture(void *a, void*b, void*c);
void load_theme_action(void *a, void*b, void*c);
void load_keymap_action(void *a, void*b, void*c);
void unmute_all_action(void*, void*, void*);
void export_wav_action(void *a, void*b, void*c);
void export_fzt_action(void *a, void*b, void*c);
void export_hires_wav_action(void *a, void*b, void*c);
void export_channels_action(void *a, void*b, void*c);
void open_data(void *type, void*b, void*c);
void do_undo(void *stack, void*b, void*c);
void kill_wavetable_entry(void *a, void*b, void*c);
void open_menu_action(void*,void*,void*);
void flip_bit_action(void *bits, void *mask, void *);
void set_note_jump(void *steps, void *, void *);
void change_visualizer_action(void *vis, void *unused1, void *unused2);
void open_help(void *unused0, void *unused1, void *unused2);
void open_help_no_lock(void *unused0, void *unused1, void *unused2);
void open_songmessage(void *unused0, void *unused1, void *unused2);
void change_oversample(void *oversample, void *unused1, void *unused2);
void toggle_follow_play_position(void *unused1, void *unused2, void *unused3);
void toggle_visualizer(void *unused1, void *unused2, void *unused3);
void toggle_mouse_cursor(void *a, void*b, void*c);
void open_recent_file(void *path, void *b, void *c);

#endif
