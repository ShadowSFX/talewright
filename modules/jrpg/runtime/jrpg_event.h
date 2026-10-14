/**************************************************************************/
/*  jrpg_event.h                                                          */
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

#include "scene/2d/node_2d.h"

#include "modules/jrpg/core/jrpg_event_data.h"

class JRPGCharacterSprite;
class JRPGEventState;
class JRPGGameState;
class JRPGMap;
class JRPGMover;
class Texture2D;

// A map event on the running map (task 9.1.2; spec 03, RM-EVT-030, RM-EVT-038): the node the scene manager builds for
// one JRPGEventData, inside the map. It shows its active page's graphic at its cell: a character from its sheet facing
// the page's direction and showing its frame, or a tile of the map's tileset; nothing without an active page or once
// erased. Its z follows the page's priority (JRPGMap::Z_BELOW, Z_CHARACTERS, Z_ABOVE). An event Same as characters and
// not through blocks characters stepping onto its cell. Its cell, facing and erased flag are the game state's
// JRPGEventState, so a save keeps them (spec 06); when its page changes it faces the new page's direction.
// Authority (D-007): server. Online, the zone decides the page and the position; the client node only shows them.
class JRPGEvent : public Node2D {
	GDCLASS(JRPGEvent, Node2D);

	Ref<JRPGEventData> data;
	Ref<JRPGEventState> state;
	Ref<JRPGMover> mover;
	JRPGCharacterSprite *sprite = nullptr;
	JRPGMap *map = nullptr; // The map it registered with while in the tree.
	Vector2i cell_size = Vector2i(48, 48);
	int page_index = -1;
	bool keep_direction = false; // The first page keeps a saved facing.
	String shown_character; // The sheet the sprite shows.
	Ref<Texture2D> tile_texture;
	Rect2i tile_region;

	JRPGMap *_find_map() const;
	void _show_page();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	// The event's data and runtime state; p_saved_state: the state comes from a save, so the first page keeps its
	// facing. Places the event at the state's cell.
	void setup(const Ref<JRPGEventData> &p_data, const Ref<JRPGEventState> &p_state, bool p_saved_state);
	Ref<JRPGEventData> get_data() const { return data; }
	Ref<JRPGEventState> get_state() const { return state; }
	Ref<JRPGMover> get_mover() const { return mover; }
	int get_event_id() const;
	Vector2i get_cell() const;
	JRPGSystemData::Direction get_direction() const;

	// Picks the active page in p_game_state on map p_map_id (JRPGEventData::find_active_page) and shows it; returns
	// whether the page changed.
	bool refresh_page(const Ref<JRPGGameState> &p_game_state, int p_map_id);
	int get_page_index() const { return page_index; }
	Ref<JRPGEventPage> get_page() const;
	// Shown: an active page and not erased.
	bool is_present() const;
	// Whether it stops characters stepping onto its cell: present, Same as characters and not through.
	bool blocks_characters() const;

	// The size of the map's cells, in pixels.
	void set_cell_size(const Vector2i &p_size);
	Vector2i get_cell_size() const { return cell_size; }
	// Moves the node to where the mover is (call after stepping it).
	void update_position();

	JRPGEvent();
};
