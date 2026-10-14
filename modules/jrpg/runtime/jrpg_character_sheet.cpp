/**************************************************************************/
/*  jrpg_character_sheet.cpp                                              */
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

#include "jrpg_character_sheet.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/object/class_db.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/sprite_frames.h"
#include "scene/scene_string_names.h"

JRPGCharacterSheet::Layout JRPGCharacterSheet::layout_from(const Dictionary &p_options) {
	Layout layout;
	layout.frames = p_options.get("frames", layout.frames);
	layout.directions = p_options.get("directions", layout.directions);
	layout.grid = p_options.get("grid", layout.grid);
	layout.index = p_options.get("index", layout.index);
	layout.row_order = p_options.get("row_order", layout.row_order);
	layout.y_offset = p_options.get("y_offset", layout.y_offset);
	layout.object = p_options.get("object", layout.object);
	return layout;
}

Ref<SpriteFrames> JRPGCharacterSheet::make_frames(const Ref<Texture2D> &p_texture, const Layout &p_layout, String *r_error) {
	const auto refuse = [&](const String &p_reason) {
		if (r_error) {
			*r_error = p_reason;
		}
		return Ref<SpriteFrames>();
	};
	if (p_texture.is_null()) {
		return refuse("there is no image.");
	}
	if (p_layout.frames < 1 || (p_layout.directions != 4 && p_layout.directions != 8)) {
		return refuse(vformat("a sheet has 1 or more frames a row and 4 or 8 directions, not %d and %d.", p_layout.frames, p_layout.directions));
	}
	if (p_layout.grid.x < 1 || p_layout.grid.y < 1 || p_layout.index < 0 || p_layout.index >= p_layout.grid.x * p_layout.grid.y) {
		return refuse(vformat("character %d is not in a grid of %d x %d characters.", p_layout.index, p_layout.grid.x, p_layout.grid.y));
	}
	// The rows' directions: the given order, or the default one.
	Vector<int> rows;
	const PackedStringArray names = p_layout.row_order.strip_edges().is_empty() ? PackedStringArray() : p_layout.row_order.split(",", false);
	for (int row = 0; row < p_layout.directions; row++) {
		if (names.is_empty()) {
			rows.push_back(row);
			continue;
		}
		const String name = row < names.size() ? names[row].strip_edges().to_lower() : String();
		int direction = -1;
		for (int d = 0; d < p_layout.directions && direction < 0; d++) {
			direction = name == DIRECTION_NAMES[d] ? d : -1;
		}
		if (direction < 0 || rows.has(direction)) {
			PackedStringArray known;
			for (int d = 0; d < p_layout.directions; d++) {
				known.push_back(DIRECTION_NAMES[d]);
			}
			return refuse(vformat("the row order names each of the %d directions once (%s), not \"%s\".", p_layout.directions, String(", ").join(known), p_layout.row_order));
		}
		rows.push_back(direction);
	}
	if (names.size() > p_layout.directions) {
		return refuse(vformat("the row order names %d rows; the sheet has %d.", names.size(), p_layout.directions));
	}
	const Vector2i size = p_texture->get_size();
	const Vector2i character = size / p_layout.grid;
	if (size.x % p_layout.grid.x != 0 || size.y % p_layout.grid.y != 0 || character.x % p_layout.frames != 0 || character.y % p_layout.directions != 0 || character.x < p_layout.frames || character.y < p_layout.directions) {
		return refuse(vformat("a %d x %d image does not divide into %d x %d characters of %d frames by %d directions.", size.x, size.y, p_layout.grid.x, p_layout.grid.y, p_layout.frames, p_layout.directions));
	}
	const Vector2i frame_size(character.x / p_layout.frames, character.y / p_layout.directions);
	const Vector2i origin(p_layout.index % p_layout.grid.x * character.x, p_layout.index / p_layout.grid.x * character.y);

	Ref<SpriteFrames> frames;
	frames.instantiate();
	frames->remove_animation(SceneStringName(default_));
	for (int row = 0; row < p_layout.directions; row++) {
		const StringName animation = DIRECTION_NAMES[rows[row]];
		frames->add_animation(animation);
		frames->set_animation_loop(animation, true);
		frames->set_animation_speed(animation, 8.0); // Only for scripts that play it: characters step their frames.
		for (int column = 0; column < p_layout.frames; column++) {
			Ref<AtlasTexture> frame;
			frame.instantiate();
			frame->set_atlas(p_texture);
			frame->set_region(Rect2(origin + Vector2i(column, row) * frame_size, frame_size));
			frames->add_frame(animation, frame);
		}
	}
	frames->set_meta(META_Y_OFFSET, p_layout.y_offset);
	frames->set_meta(META_OBJECT, p_layout.object);
	return frames;
}

Ref<SpriteFrames> JRPGCharacterSheet::make_frames_with(const Ref<Texture2D> &p_texture, const Dictionary &p_options) {
	String error;
	const Ref<SpriteFrames> frames = make_frames(p_texture, layout_from(p_options), &error);
	ERR_FAIL_COND_V_MSG(frames.is_null(), frames, "JRPGCharacterSheet: " + error);
	return frames;
}

Ref<SpriteFrames> JRPGCharacterSheet::load_frames(const String &p_reference, String *r_error) {
	const String path = ResourceUID::ensure_path(p_reference);
	if (path.is_empty() || !ResourceLoader::exists(path)) {
		if (r_error) {
			*r_error = vformat("%s is missing.", p_reference);
		}
		return Ref<SpriteFrames>();
	}
	const Ref<Resource> resource = ResourceLoader::load(path);
	const Ref<SpriteFrames> imported = resource;
	if (imported.is_valid()) {
		return imported;
	}
	return make_frames(resource, Layout(), r_error);
}

Ref<SpriteFrames> JRPGCharacterSheet::_load_frames_bound(const String &p_reference) {
	return load_frames(p_reference);
}

int JRPGCharacterSheet::get_y_offset(const Ref<SpriteFrames> &p_frames) {
	return p_frames.is_valid() ? int(p_frames->get_meta(META_Y_OFFSET, 0)) : 0;
}

bool JRPGCharacterSheet::is_object(const Ref<SpriteFrames> &p_frames) {
	return p_frames.is_valid() && bool(p_frames->get_meta(META_OBJECT, false));
}

void JRPGCharacterSheet::_bind_methods() {
	ClassDB::bind_static_method("JRPGCharacterSheet", D_METHOD("make_frames", "texture", "options"), &JRPGCharacterSheet::make_frames_with, DEFVAL(Dictionary()));
	ClassDB::bind_static_method("JRPGCharacterSheet", D_METHOD("load_frames", "reference"), &JRPGCharacterSheet::_load_frames_bound);
	ClassDB::bind_static_method("JRPGCharacterSheet", D_METHOD("get_y_offset", "frames"), &JRPGCharacterSheet::get_y_offset);
	ClassDB::bind_static_method("JRPGCharacterSheet", D_METHOD("is_object", "frames"), &JRPGCharacterSheet::is_object);
}
