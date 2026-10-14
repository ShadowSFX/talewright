/**************************************************************************/
/*  jrpg_event.cpp                                                        */
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

#include "jrpg_event.h"

#include "jrpg_character_sheet.h"
#include "jrpg_character_sprite.h"
#include "jrpg_map.h"

#include "core/object/class_db.h"
#include "scene/resources/sprite_frames.h"

#include "modules/jrpg/core/jrpg_database.h" // IWYU pragma: keep (JRPGGameState returns Ref<JRPGDatabase> inline).
#include "modules/jrpg/core/jrpg_event_state.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_mover.h"

void JRPGEvent::setup(const Ref<JRPGEventData> &p_data, const Ref<JRPGEventState> &p_state, bool p_saved_state) {
	ERR_FAIL_COND(p_data.is_null() || p_state.is_null());
	data = p_data;
	state = p_state;
	keep_direction = p_saved_state;
	page_index = -1;
	set_name(vformat("Event%d", data->get_event_id()));
	mover->set_cell(state->get_cell());
	mover->set_direction(state->get_direction());
	_show_page();
	update_position();
}

int JRPGEvent::get_event_id() const {
	return data.is_valid() ? data->get_event_id() : 0;
}

Vector2i JRPGEvent::get_cell() const {
	return mover->get_cell();
}

JRPGSystemData::Direction JRPGEvent::get_direction() const {
	return mover->get_direction();
}

Ref<JRPGEventPage> JRPGEvent::get_page() const {
	return data.is_valid() && page_index >= 0 ? data->get_page(page_index) : Ref<JRPGEventPage>();
}

bool JRPGEvent::is_present() const {
	return page_index >= 0 && state.is_valid() && !state->is_erased();
}

bool JRPGEvent::blocks_characters() const {
	if (!is_present()) {
		return false;
	}
	const Ref<JRPGEventPage> page = get_page();
	bool through = page->is_through();
	if (state->get_through() != JRPGEventState::THROUGH_FROM_PAGE) {
		through = state->get_through() == JRPGEventState::THROUGH_ON; // A move route's Through ON/OFF (task 9.6).
	}
	return page->get_priority() == JRPGEventPage::PRIORITY_SAME && !through;
}

bool JRPGEvent::refresh_page(const Ref<JRPGGameState> &p_game_state, int p_map_id) {
	ERR_FAIL_COND_V(data.is_null(), false);
	const int found = data->find_active_page(p_game_state, p_map_id);
	if (found == page_index) {
		return false;
	}
	page_index = found;
	const Ref<JRPGEventPage> page = get_page();
	if (page.is_valid() && !keep_direction) {
		// RM-EVT-030: the event faces its new page's image direction (a saved facing stays for the page it was saved on).
		mover->set_direction(page->get_direction());
		state->set_direction(page->get_direction());
	}
	keep_direction = false;
	_show_page();
	update_position();
	return true;
}

JRPGMap *JRPGEvent::_find_map() const {
	for (Node *at = get_parent(); at; at = at->get_parent()) {
		if (JRPGMap *found = Object::cast_to<JRPGMap>(at)) {
			return found;
		}
	}
	return nullptr;
}

void JRPGEvent::_show_page() {
	const Ref<JRPGEventPage> page = get_page();
	set_visible(is_present());
	tile_texture.unref();
	set_z_index(JRPGMap::Z_BELOW);
	if (page.is_null()) {
		sprite->set_visible(false);
		queue_redraw();
		return;
	}
	const int z[] = { JRPGMap::Z_BELOW, JRPGMap::Z_CHARACTERS, JRPGMap::Z_ABOVE };
	set_z_index(z[page->get_priority()]);
	if (page->get_tile() > 0) {
		// A tile of the map's tileset, as the map draws it (RM-EVT-030).
		const JRPGMap *owner = map ? map : _find_map();
		Ref<Texture2D> texture;
		Rect2i region;
		if (owner && owner->get_palette_tile_region(page->get_tile(), texture, region)) {
			tile_texture = texture;
			tile_region = region;
		} else {
			WARN_PRINT(vformat("JRPG: event %d's page %d shows tile %d, which the map's tileset does not have.", get_event_id(), page_index + 1, page->get_tile()));
		}
		sprite->set_visible(false);
		queue_redraw();
		return;
	}
	const String reference = page->get_character();
	if (reference != shown_character) {
		shown_character = reference;
		String error;
		const Ref<SpriteFrames> frames = reference.is_empty() ? Ref<SpriteFrames>() : JRPGCharacterSheet::load_frames(reference, &error);
		if (!reference.is_empty() && frames.is_null()) {
			WARN_PRINT(vformat("JRPG: event %d's character %s cannot be shown: %s", get_event_id(), reference, error));
		}
		sprite->set_character_frames(frames);
	}
	sprite->set_visible(sprite->get_sprite_frames().is_valid());
	queue_redraw();
}

void JRPGEvent::set_cell_size(const Vector2i &p_size) {
	cell_size = p_size.maxi(1);
	sprite->set_cell_size(cell_size);
	update_position();
}

void JRPGEvent::update_position() {
	// The middle of the cell the mover is on, or between two on the way (as the player).
	set_position((mover->get_position() + Vector2(0.5, 0.5)) * Vector2(cell_size));
	if (!sprite->is_visible()) {
		return;
	}
	sprite->show_state(mover->get_direction(), mover->is_moving(), mover->get_walk_distance());
	const Ref<JRPGEventPage> page = get_page();
	if (page.is_valid() && !mover->is_moving()) {
		sprite->set_frame(page->get_pattern()); // Standing, the page's frame (AnimatedSprite2D keeps it in range).
	}
}

void JRPGEvent::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			map = _find_map();
			if (map && data.is_valid()) {
				map->register_event_node(get_event_id(), this);
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (map && data.is_valid()) {
				map->unregister_event_node(get_event_id(), this);
			}
			map = nullptr;
		} break;
		case NOTIFICATION_DRAW: {
			if (tile_texture.is_valid()) {
				// The tile fills the cell, centered on the node as the cell's middle.
				draw_texture_rect_region(tile_texture, Rect2(-Vector2(cell_size) / 2, cell_size), tile_region);
			}
		} break;
	}
}

void JRPGEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_data"), &JRPGEvent::get_data);
	ClassDB::bind_method(D_METHOD("get_state"), &JRPGEvent::get_state);
	ClassDB::bind_method(D_METHOD("get_mover"), &JRPGEvent::get_mover);
	ClassDB::bind_method(D_METHOD("get_event_id"), &JRPGEvent::get_event_id);
	ClassDB::bind_method(D_METHOD("get_cell"), &JRPGEvent::get_cell);
	ClassDB::bind_method(D_METHOD("get_direction"), &JRPGEvent::get_direction);
	ClassDB::bind_method(D_METHOD("refresh_page", "game_state", "map_id"), &JRPGEvent::refresh_page);
	ClassDB::bind_method(D_METHOD("get_page_index"), &JRPGEvent::get_page_index);
	ClassDB::bind_method(D_METHOD("get_page"), &JRPGEvent::get_page);
	ClassDB::bind_method(D_METHOD("is_present"), &JRPGEvent::is_present);
	ClassDB::bind_method(D_METHOD("blocks_characters"), &JRPGEvent::blocks_characters);
	ClassDB::bind_method(D_METHOD("set_cell_size", "size"), &JRPGEvent::set_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &JRPGEvent::get_cell_size);
	ClassDB::bind_method(D_METHOD("update_position"), &JRPGEvent::update_position);
}

JRPGEvent::JRPGEvent() {
	mover.instantiate();
	sprite = memnew(JRPGCharacterSprite);
	sprite->set_name("Sprite");
	sprite->set_visible(false);
	add_child(sprite, false, INTERNAL_MODE_FRONT);
}
