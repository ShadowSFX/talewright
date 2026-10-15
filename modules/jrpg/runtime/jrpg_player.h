/**************************************************************************/
/*  jrpg_player.h                                                         */
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

#include "core/templates/local_vector.h"
#include "scene/2d/node_2d.h"

#include "modules/jrpg/core/jrpg_mover.h"

class Camera2D;
class JRPGCharacterSprite;

// The player's character on the current map (task 8.6.2.2): a JRPGMover the scene manager steps on the simulation
// clock, drawn at the characters' z (JRPGMap::Z_CHARACTERS) where the mover says, with the camera that follows it.
// It reads the direction actions (jrpg_up, jrpg_down, jrpg_left, jrpg_right): of those held, the one pressed last.
// It shows its character (set_character: the party leader's) as a JRPGCharacterSprite; without one it draws a marker
// showing its facing.
// Authority (D-007): server. Online, the client sends the steps its player asks for and predicts them with the same
// mover; the zone server's mover decides.
class JRPGPlayer : public Node2D {
	GDCLASS(JRPGPlayer, Node2D);

public:
	static constexpr const char *DIRECTION_ACTIONS[4] = { "jrpg_up", "jrpg_right", "jrpg_down", "jrpg_left" }; // JRPGSystemData::Direction order.
	static constexpr const char *DASH_ACTION = "jrpg_dash";

private:
	Ref<JRPGMover> mover;
	Camera2D *camera = nullptr;
	JRPGCharacterSprite *sprite = nullptr;
	String character;
	Vector2i cell_size = Vector2i(48, 48);
	LocalVector<int> held; // Direction actions held, in the order they were pressed.
	int tapped = -1; // A direction pressed (and maybe released) since the last read.

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	Ref<JRPGMover> get_mover() const { return mover; }
	Camera2D *get_camera() const { return camera; }
	JRPGCharacterSprite *get_sprite() const { return sprite; }
	// Draws the player with the character sheet p_reference (a uid:// or res:// path, as an actor's character); an
	// empty or unusable reference shows the marker.
	void set_character(const String &p_reference);
	String get_character() const { return character; }
	// Places the character on p_cell facing p_direction at once (a transfer).
	void place(const Vector2i &p_cell, JRPGSystemData::Direction p_direction);
	// The size of the map's cells, in pixels.
	void set_cell_size(const Vector2i &p_size);
	Vector2i get_cell_size() const { return cell_size; }
	// Moves the node to where the mover is (call after stepping it).
	void update_position();
	// Reads the direction actions now: the direction of the one held that was pressed last, or -1 with none held.
	int read_direction();
	// A direction pressed since the last read_direction() counts once even if it was released before (a tap shorter
	// than a simulation step).
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;
	bool is_dash_held() const;

	Vector2i get_cell() const { return mover->get_cell(); }
	JRPGSystemData::Direction get_direction() const { return mover->get_direction(); }
	bool is_moving() const { return mover->is_moving(); }

	JRPGPlayer();
};
