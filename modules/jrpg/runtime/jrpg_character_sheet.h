/**************************************************************************/
/*  jrpg_character_sheet.h                                                */
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

#include "core/object/ref_counted.h"
#include "core/variant/dictionary.h"

class SpriteFrames;
class Texture2D;

// Character sheets (spec 05, "Character sheets"; task 8.7.1): the SpriteFrames a character is drawn with, cut from its
// sheet. A sheet holds one character by default: a row per facing, up, right, down, left (then up-right, down-right,
// down-left, up-left on an 8-direction sheet), and a column per walk frame, column 0 standing. Other layouts set a
// grid of characters and the index of one, or another row order. Each direction becomes an animation named
// DIRECTION_NAMES[direction]; the y offset (pixels to draw the character higher) and object (no bush effect) travel as
// the frames' metadata.
// The editor's Character Sheet importer (JRPGCharacterSheetImporter) saves these SpriteFrames for an image; an image
// imported as a plain texture gets the default layout when a character is drawn with it.
class JRPGCharacterSheet : public RefCounted {
	GDCLASS(JRPGCharacterSheet, RefCounted);

public:
	static constexpr int DIRECTION_COUNT = 8;
	static constexpr const char *DIRECTION_NAMES[DIRECTION_COUNT] = { "up", "right", "down", "left", "up_right", "down_right", "down_left", "up_left" };
	static constexpr const char *META_Y_OFFSET = "jrpg_y_offset";
	static constexpr const char *META_OBJECT = "jrpg_object";

	struct Layout {
		int frames = 4; // Walk frames per row.
		int directions = 4; // 4 or 8 rows.
		Vector2i grid = Vector2i(1, 1); // Characters across and down the image.
		int index = 0; // The character, row by row in the grid.
		String row_order; // The direction names of the rows, comma-separated; empty: DIRECTION_NAMES in order.
		int y_offset = 0;
		bool object = false;
	};

private:
	static Ref<SpriteFrames> _load_frames_bound(const String &p_reference);

protected:
	static void _bind_methods();

public:
	// The layout described by p_options (the importer's option names: frames, directions, grid, index, row_order,
	// y_offset, object); missing options keep their defaults.
	static Layout layout_from(const Dictionary &p_options);
	// The frames of one character of p_texture. Null, with the reason in r_error, when the image does not divide into
	// the layout or the layout itself is wrong.
	static Ref<SpriteFrames> make_frames(const Ref<Texture2D> &p_texture, const Layout &p_layout, String *r_error = nullptr);
	// make_frames for scripts: p_options as layout_from() reads them.
	static Ref<SpriteFrames> make_frames_with(const Ref<Texture2D> &p_texture, const Dictionary &p_options);
	// The frames a character reference (a uid:// or res:// path, as database entries hold them) is drawn with:
	// SpriteFrames the importer made, as they are; an image, cut with the default layout. Null when there is none.
	static Ref<SpriteFrames> load_frames(const String &p_reference, String *r_error = nullptr);
	static int get_y_offset(const Ref<SpriteFrames> &p_frames);
	static bool is_object(const Ref<SpriteFrames> &p_frames);
};
