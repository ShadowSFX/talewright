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

class ImageTexture;
class Texture2D;

// A game window (task 10.1.1; RM-MNU-016, D-049): a back tinted by a tone and a nine-slice border over it, the
// window's children laid out inside a padding (as PanelContainer fits them, scene/gui/panel_container.cpp:106-121),
// scroll and pause marks, and an openness that opens and closes the window vertically from its middle. While it opens
// or closes, the frame is drawn squashed to the opening band and the children are clipped to that band (the renderer
// clips an item and its children to its custom rect, servers/rendering/renderer_canvas_cull.cpp:413-425); closed, the
// window draws nothing.
// The skin comes from theme items of type JRPGWindow (icons back, border, cursor, arrows and icon_set; constants
// padding, frame_margin and icon_size), so a project theme or theme overrides replace it as for any control. The
// built-in skin (the generated window images and icon set, embedded at build time) is added to Godot's default theme
// the first time a window needs it, so it lives and dies with the default theme and its RenderingServer.
// Authority (D-007): client. Windows only present what the game decides.
class JRPGWindow : public Container {
	GDCLASS(JRPGWindow, Container);

public:
	static constexpr int OPEN_FRAMES = 8; // Frames, at 60 a second, to open or to close.
	static constexpr int DEFAULT_PADDING = 12; // Pixels between the window's edge and its contents.
	static constexpr int DEFAULT_FRAME_MARGIN = 16; // The skin's nine-slice margins (talewright/docs/assets.md).
	static constexpr int ARROW_SIZE = 16; // The arrows sheet: four 16 x 16 marks (up, down, pause, submenu).
	static constexpr int DEFAULT_ICON_SIZE = 32; // The icon set: square icons in rows (talewright/docs/assets.md).

private:
	enum Mark {
		MARK_SCROLL_UP,
		MARK_SCROLL_DOWN,
		MARK_PAUSE,
		MARK_SUBMENU,
	};

	Vector3i tone;
	double openness = 1.0;
	int opening = 0; // 1 while opening, -1 while closing.
	bool scroll_up_visible = false;
	bool scroll_down_visible = false;
	bool pause_visible = false;
	bool frame_visible = true;

	Ref<ImageTexture> toned_back; // The back with the tone added; null when the tone is zero.

	void _update_toned_back();
	void _set_openness(double p_openness);
	void _draw_mark(Mark p_mark, const Point2 &p_position);

protected:
	struct ThemeCache {
		Ref<Texture2D> back;
		Ref<Texture2D> border;
		Ref<Texture2D> cursor;
		Ref<Texture2D> arrows;
		Ref<Texture2D> icon_set;
		int padding = DEFAULT_PADDING;
		int frame_margin = DEFAULT_FRAME_MARGIN;
		int icon_size = DEFAULT_ICON_SIZE;
	} theme_cache;

	void _notification(int p_what);
	static void _bind_methods();
	virtual void _update_theme_item_cache() override;
	// Internal processing runs while the window opens or closes, or while a subclass needs it (a list with focus);
	// subclasses call _update_process() when their need changes.
	virtual bool _needs_process() const { return false; }
	void _update_process();

public:
	// Adds the built-in skin to Godot's default theme when that theme has none yet (and exists: it is made with the
	// RenderingServer). Windows call it before reading their theme items.
	static void ensure_default_skin();

	// Added to every pixel of the back, per channel, from -255 to 255 (System data's window tone).
	void set_tone(const Vector3i &p_tone);
	Vector3i get_tone() const { return tone; }

	// 0 closed (nothing drawn) to 1 open; set at once, with no signal.
	void set_openness(double p_openness);
	double get_openness() const { return openness; }
	// Opens or closes over OPEN_FRAMES frames; emits opened or closed when done.
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

	virtual Size2 get_minimum_size() const override;

	JRPGWindow();
};
