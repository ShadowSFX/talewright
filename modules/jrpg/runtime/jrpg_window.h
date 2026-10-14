/**************************************************************************/
/*  jrpg_window.h                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                               TALEWRIGHT                               */
/*         a JRPG-focused fork of the Godot Engine (MIT License)          */
/**************************************************************************/
/* Copyright (c) 2026-present Talewright contributors.                    */
/* Based on the Godot Engine, Copyright (c) 2014-present Godot Engine     */
/* contributors and (c) 2007-2014 Juan Linietsky, Ariel Manzur.           */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "scene/gui/container.h"

#include "modules/jrpg/core/jrpg_system_data.h"

class AudioStreamPlayer;
class ImageTexture;
class JRPGDatabase;
class Texture2D;

// A game window (task 10.1.1; RM-MNU-016, D-049): a back tinted by a tone and a nine-slice border over it, the
// window's children laid out inside a padding (as PanelContainer fits them, scene/gui/panel_container.cpp:106-121),
// scroll and pause marks, and an openness that opens and closes the window vertically from its middle. While it opens
// or closes, the frame is drawn squashed to the opening band and the children are clipped to that band (the renderer
// clips an item and its children to its custom rect, servers/rendering/renderer_canvas_cull.cpp:413-425); closed, the
// window draws nothing.
// The skin comes from theme items of type JRPGWindow (icons back, border, cursor, arrows and icon_set; constants
// padding, frame_margin, mark_offset and icon_size), so a project theme or theme overrides replace it as for any
// control; skin parts may be AtlasTextures (regions of one image), as for StyleBoxTexture. The
// built-in skin (the generated window images and icon set, embedded at build time) is added to Godot's default theme
// the first time a window needs it, so it lives and dies with the default theme and its RenderingServer.
// Windows play the interface sounds (cursor, OK, cancel, buzzer: RM-MNU-019) and take the window tone (RM-MNU-016,
// RM-SYS-005) of the System data of their database (the project's when none is set; epic 10 review A5, A7).
// Authority (D-007): client. Windows only present what the game decides.
class JRPGWindow : public Container {
	GDCLASS(JRPGWindow, Container);

public:
	static constexpr int OPEN_FRAMES = 8; // Frames, at 60 a second, to open or to close.
	static constexpr int DEFAULT_PADDING = 12; // Pixels between the window's edge and its contents.
	static constexpr int DEFAULT_FRAME_MARGIN = 16; // The skin's nine-slice margins (talewright/docs/assets.md).
	static constexpr int ARROW_SIZE = 16; // The arrows sheet: four 16 x 16 marks (up, down, pause, submenu).
	static constexpr int DEFAULT_ICON_SIZE = 32; // The icon set: square icons in rows (talewright/docs/assets.md).
	// From the window's edge to the line the scroll and pause marks are centered on: the middle of the padding, over the
	// built-in skin's border, so the marks stay inside the window and never cover the first or last row (epic 10
	// review A16).
	static constexpr int DEFAULT_MARK_OFFSET = 6;

private:
	enum Mark {
		MARK_SCROLL_UP,
		MARK_SCROLL_DOWN,
		MARK_PAUSE,
		MARK_SUBMENU,
	};

	Vector3i tone;
	bool use_system_tone = true;
	double openness = 1.0;
	int opening = 0; // 1 while opening, -1 while closing.
	bool scroll_up_visible = false;
	bool scroll_down_visible = false;
	bool pause_visible = false;
	bool frame_visible = true;

	Ref<ImageTexture> toned_back; // The back with the tone in use added; null when that tone is zero.
	Ref<JRPGDatabase> database; // Null: the project's (JRPGDatabase::PROJECT_PATH).
	mutable Ref<JRPGDatabase> project_database; // The project's, loaded once (not on every sound).
	ObjectID sound_player_id; // Not a pointer: scripts can free the player.
	int last_sound = -1;

	void _update_toned_back();
	void _set_openness(double p_openness);
	void _draw_mark(Mark p_mark, const Point2 &p_position);
	void _emit_reached(bool p_open);

protected:
	struct ThemeCache {
		Ref<Texture2D> back;
		Ref<Texture2D> border;
		Ref<Texture2D> cursor;
		Ref<Texture2D> arrows;
		Ref<Texture2D> icon_set;
		int padding = DEFAULT_PADDING;
		int frame_margin = DEFAULT_FRAME_MARGIN;
		int mark_offset = DEFAULT_MARK_OFFSET;
		int icon_size = DEFAULT_ICON_SIZE;
	} theme_cache;

	void _notification(int p_what);
	void _validate_property(PropertyInfo &p_property) const;
	static void _bind_methods();
	virtual void _update_theme_item_cache() override;
	// Internal processing runs while the window opens or closes, or while a subclass needs it (a list with focus);
	// subclasses call _update_process() when their need changes.
	virtual bool _needs_process() const { return false; }
	void _update_process();
	// Draws p_texture stretched over p_rect as a nine-slice with p_margin on every side, from the texture's own region
	// when it is an AtlasTexture (as StyleBoxTexture::draw, scene/resources/style_box_texture.cpp:166-185; epic 10
	// review A6). Call while drawing.
	void draw_nine_patch(const Ref<Texture2D> &p_texture, const Rect2 &p_rect, real_t p_margin, const Color &p_modulate = Color(1, 1, 1)) const;

public:
	// Adds the built-in skin to Godot's default theme when that theme has none yet (and exists: it is made with the
	// RenderingServer). Windows call it before reading their theme items.
	static void ensure_default_skin();
	// Where draw_nine_patch draws p_texture over p_rect: the target rectangle and the source region in the texture
	// drawn (an AtlasTexture's atlas). False when there is nothing to draw.
	static bool get_nine_patch_region(const Ref<Texture2D> &p_texture, const Rect2 &p_rect, Rect2 &r_rect, Rect2 &r_source);

	// Added to every pixel of the back, per channel, from -255 to 255. The window's own tone, used when
	// use_system_tone is off; by default the window takes its System data's window tone.
	void set_tone(const Vector3i &p_tone);
	Vector3i get_tone() const { return tone; }
	void set_use_system_tone(bool p_use);
	bool is_using_system_tone() const { return use_system_tone; }
	// The tone the back is drawn with: the System data's window tone (zero without System data) or the window's own.
	// The game state's Change Window Color override joins with that command (epic 24).
	Vector3i get_tone_in_use() const;

	// 0 closed (nothing drawn) to 1 open; set at once, with no signal.
	void set_openness(double p_openness);
	double get_openness() const { return openness; }
	// Opens or closes over OPEN_FRAMES frames; emits opened or closed when done. A window already in the state asked for
	// emits it on the next idle frame, so a caller can still connect or await after the call (epic 10 review A19). A
	// window closing when open() is called turns round (closed is not emitted), and the other way round.
	void open();
	void close();
	bool is_open() const { return openness >= 1.0; }
	bool is_closed() const { return openness <= 0.0; }
	bool is_opening() const { return opening > 0; }
	bool is_closing() const { return opening < 0; }

	// Marks drawn while the window is open: scroll up (top middle), scroll down and pause (bottom middle).
	void set_scroll_up_visible(bool p_visible);
	bool is_scroll_up_visible() const { return scroll_up_visible; }
	void set_scroll_down_visible(bool p_visible);
	bool is_scroll_down_visible() const { return scroll_down_visible; }
	void set_pause_visible(bool p_visible);
	bool is_pause_visible() const { return pause_visible; }
	// Whether the back and border are drawn (message windows on a dimmed or transparent background draw none).
	void set_frame_visible(bool p_visible);
	bool is_frame_visible() const { return frame_visible; }

	// Where the contents go: the window's rectangle inside the padding, in its own coordinates.
	Rect2 get_content_rect() const;
	// The rectangle drawn now: the whole window when open, the band it opens through while opening or closing.
	Rect2 get_open_rect() const;
	// The back as drawn: the skin's back with the tone added (the skin's own back when the tone is zero).
	Ref<Texture2D> get_back_texture() const;
	// The skin's cursor and arrows, for windows that draw their own contents (lists).
	Ref<Texture2D> get_cursor_texture() const { return theme_cache.cursor; }
	Ref<Texture2D> get_arrows_texture() const { return theme_cache.arrows; }
	// The icon set (icon_index n is at column n % per row, row n / per row) and the region of icon p_index in it; an
	// empty region when the set has no such icon.
	Ref<Texture2D> get_icon_set_texture() const { return theme_cache.icon_set; }
	int get_icon_size() const { return theme_cache.icon_size; }
	Rect2 get_icon_region(int p_index) const;

	// The database set on this window (null: the project's is used, without being stored in the window).
	void set_database(const Ref<JRPGDatabase> &p_database);
	Ref<JRPGDatabase> get_database() const;
	// The database the window reads its sounds and tone from: the one set, else the project's (or null).
	Ref<JRPGDatabase> get_effective_database() const;
	// Plays one of the System data's interface sounds (silent when the database has none). The stream is set only when
	// it changes, so a sound played again overlaps itself (up to four at once); another sound stops the one playing.
	void play_sound(JRPGSystemData::Sound p_sound);
	// The sound played last (a JRPGSystemData::Sound), or -1.
	int get_last_sound() const { return last_sound; }
	// The internal player of the interface sounds ("Sounds"), made with the first sound played; null before (and after
	// something freed it, until the next sound).
	AudioStreamPlayer *get_sound_player() const;

	virtual Size2 get_minimum_size() const override;
	// Clicks land only on the part that shows: all of it when open, the opening band meanwhile, none when closed.
	virtual bool has_point(const Point2 &p_point) const override;

	JRPGWindow();
	~JRPGWindow(); // Out of line: JRPGDatabase is only declared here.
};
