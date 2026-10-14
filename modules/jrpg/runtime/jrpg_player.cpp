/**************************************************************************/
/*  jrpg_player.cpp                                                       */
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

#include "jrpg_player.h"

#include "jrpg_character_sheet.h"
#include "jrpg_character_sprite.h"
#include "jrpg_map.h"

#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/object/class_db.h"
#include "scene/2d/camera_2d.h"
#include "scene/resources/sprite_frames.h"

void JRPGPlayer::place(const Vector2i &p_cell, JRPGSystemData::Direction p_direction) {
	mover->set_cell(p_cell);
	mover->set_direction(p_direction);
	update_position();
}

void JRPGPlayer::set_character(const String &p_reference) {
	character = p_reference;
	String error;
	const Ref<SpriteFrames> frames = p_reference.is_empty() ? Ref<SpriteFrames>() : JRPGCharacterSheet::load_frames(p_reference, &error);
	if (!p_reference.is_empty() && frames.is_null()) {
		WARN_PRINT(vformat("JRPG: the player's character %s cannot be shown: %s", p_reference, error));
	}
	sprite->set_character_frames(frames);
	sprite->set_visible(frames.is_valid());
	queue_redraw(); // The marker shows, or goes.
	update_position();
}

void JRPGPlayer::set_cell_size(const Vector2i &p_size) {
	cell_size = p_size.maxi(1);
	sprite->set_cell_size(cell_size);
	update_position();
}

void JRPGPlayer::update_position() {
	// The middle of the cell the mover is on, or between two on the way.
	set_position((mover->get_position() + Vector2(0.5, 0.5)) * Vector2(cell_size));
	sprite->show_state(mover->get_direction(), mover->is_moving(), mover->get_walk_distance());
	if (!sprite->is_visible()) {
		queue_redraw(); // The marker's facing may have changed (a character sprite needs no drawing: epic 8 review A13).
	}
}

int JRPGPlayer::read_direction() {
	const InputMap *input_map = InputMap::get_singleton();
	Input *input = Input::get_singleton();
	for (int d = 0; d < 4; d++) {
		// A project without the JRPG actions (not set up) reads none, without errors for each frame.
		const bool pressed = input && input_map && input_map->has_action(DIRECTION_ACTIONS[d]) && input->is_action_pressed(DIRECTION_ACTIONS[d]);
		const int64_t at = held.find(d);
		if (pressed && at < 0) {
			held.push_back(d);
		} else if (!pressed && at >= 0) {
			held.remove_at(at);
		}
	}
	return held.is_empty() ? -1 : held[held.size() - 1];
}

bool JRPGPlayer::is_dash_held() const {
	const InputMap *input_map = InputMap::get_singleton();
	return Input::get_singleton() && input_map && input_map->has_action(DASH_ACTION) && Input::get_singleton()->is_action_pressed(DASH_ACTION);
}

void JRPGPlayer::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW || sprite->is_visible()) {
		return;
	}
	// A marker until character sprites (task 8.7): a disc, and a notch on the side it faces.
	const real_t radius = MIN(cell_size.x, cell_size.y) * 0.32;
	const Color ink(0.22, 0.14, 0.06);
	draw_circle(Vector2(), radius, Color(0.96, 0.78, 0.32));
	draw_circle(Vector2(), radius, ink, false, MAX(1.0, radius * 0.12));
	const Vector2 facing = Vector2(JRPGMover::get_offset(mover->get_direction()));
	const Vector2 side(-facing.y, facing.x);
	const Vector<Vector2> notch({ facing * radius * 1.4, facing * radius * 0.55 + side * radius * 0.45, facing * radius * 0.55 - side * radius * 0.45 });
	draw_colored_polygon(notch, ink);
}

void JRPGPlayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_mover"), &JRPGPlayer::get_mover);
	ClassDB::bind_method(D_METHOD("get_camera"), &JRPGPlayer::get_camera);
	ClassDB::bind_method(D_METHOD("get_sprite"), &JRPGPlayer::get_sprite);
	ClassDB::bind_method(D_METHOD("set_character", "reference"), &JRPGPlayer::set_character);
	ClassDB::bind_method(D_METHOD("get_character"), &JRPGPlayer::get_character);
	ClassDB::bind_method(D_METHOD("place", "cell", "direction"), &JRPGPlayer::place);
	ClassDB::bind_method(D_METHOD("set_cell_size", "size"), &JRPGPlayer::set_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &JRPGPlayer::get_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell"), &JRPGPlayer::get_cell);
	ClassDB::bind_method(D_METHOD("get_direction"), &JRPGPlayer::get_direction);
	ClassDB::bind_method(D_METHOD("is_moving"), &JRPGPlayer::is_moving);
}

JRPGPlayer::JRPGPlayer() {
	mover.instantiate();
	set_z_index(JRPGMap::Z_CHARACTERS); // Over the map's lower tiles, under its overhead ones.
	camera = memnew(Camera2D);
	camera->set_name("Camera");
	add_child(camera, false, INTERNAL_MODE_FRONT);
	sprite = memnew(JRPGCharacterSprite);
	sprite->set_name("Sprite");
	sprite->set_visible(false); // Until a character is set.
	add_child(sprite, false, INTERNAL_MODE_FRONT);
}
