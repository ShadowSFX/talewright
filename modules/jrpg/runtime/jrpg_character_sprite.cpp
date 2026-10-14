/**************************************************************************/
/*  jrpg_character_sprite.cpp                                             */
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

#include "jrpg_character_sprite.h"

#include "jrpg_character_sheet.h"

#include "core/object/class_db.h"
#include "scene/resources/sprite_frames.h"

void JRPGCharacterSprite::set_character_frames(const Ref<SpriteFrames> &p_frames) {
	set_sprite_frames(p_frames);
	reset_walk();
	_update_offset();
}

void JRPGCharacterSprite::set_cell_size(const Vector2i &p_size) {
	cell_size = p_size.maxi(1);
	_update_offset();
}

void JRPGCharacterSprite::_update_offset() {
	// The node is at the middle of the cell; the frame's bottom goes on the cell's bottom edge, raised by the offset.
	const Ref<SpriteFrames> sheet = get_sprite_frames();
	const Ref<Texture2D> first = sheet.is_valid() && sheet->has_animation(get_animation()) && sheet->get_frame_count(get_animation()) > 0 ? sheet->get_frame_texture(get_animation(), 0) : Ref<Texture2D>();
	const Vector2 size = first.is_valid() ? first->get_size() : Vector2();
	set_offset(Vector2(-size.x / 2, cell_size.y / 2.0 - size.y - JRPGCharacterSheet::get_y_offset(sheet)));
}

void JRPGCharacterSprite::reset_walk() {
	walked = 0;
	has_last_position = false;
}

void JRPGCharacterSprite::show_state(const Vector2 &p_position, JRPGSystemData::Direction p_direction, bool p_moving) {
	const Ref<SpriteFrames> sheet = get_sprite_frames();
	ERR_FAIL_INDEX(int(p_direction), 4);
	if (sheet.is_null()) {
		return;
	}
	const StringName facing = JRPGCharacterSheet::DIRECTION_NAMES[p_direction];
	if (sheet->has_animation(facing) && get_animation() != facing) {
		set_animation(facing);
		_update_offset();
	}
	const int count = sheet->has_animation(get_animation()) ? sheet->get_frame_count(get_animation()) : 0;
	if (p_moving) {
		if (has_last_position) {
			walked += (p_position - last_position).length();
		}
		// Frame 1 on the first half cell, then round in order: 1, 2, 3, 0, 1, ... with four frames.
		set_frame(count > 0 ? (int(Math::floor(walked * FRAMES_PER_CELL)) + 1) % count : 0);
	} else {
		walked = 0;
		set_frame(0);
	}
	last_position = p_position;
	has_last_position = true;
}

void JRPGCharacterSprite::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_character_frames", "frames"), &JRPGCharacterSprite::set_character_frames);
	ClassDB::bind_method(D_METHOD("set_cell_size", "size"), &JRPGCharacterSprite::set_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &JRPGCharacterSprite::get_cell_size);
	ClassDB::bind_method(D_METHOD("show_state", "position", "direction", "moving"), &JRPGCharacterSprite::show_state);
	ClassDB::bind_method(D_METHOD("reset_walk"), &JRPGCharacterSprite::reset_walk);
	ClassDB::bind_method(D_METHOD("get_walked"), &JRPGCharacterSprite::get_walked);
	BIND_CONSTANT(FRAMES_PER_CELL);
}

JRPGCharacterSprite::JRPGCharacterSprite() {
	set_centered(false);
}
