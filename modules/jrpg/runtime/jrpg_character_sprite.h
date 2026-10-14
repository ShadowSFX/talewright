/**************************************************************************/
/*  jrpg_character_sprite.h                                               */
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

#include "scene/2d/animated_sprite_2d.h"

#include "modules/jrpg/core/jrpg_system_data.h"

// A character drawn from its sheet on the map (task 8.7.2): the SpriteFrames JRPGCharacterSheet cuts, with the
// character's feet on the bottom of its cell (the sheet's y offset raises it) and the animation of its facing. While it
// walks, the frames go round in order, one every half cell walked, so steps look the same at every speed; standing, it
// shows frame 0. The frames are set, never played, so the simulation decides what is shown.
// Authority (D-007): client. It only presents what the mover says (the server's mover, online).
class JRPGCharacterSprite : public AnimatedSprite2D {
	GDCLASS(JRPGCharacterSprite, AnimatedSprite2D);

public:
	static constexpr int FRAMES_PER_CELL = 2;

private:
	Vector2i cell_size = Vector2i(48, 48);

	void _update_offset();

protected:
	static void _bind_methods();

public:
	void set_character_frames(const Ref<SpriteFrames> &p_frames);
	void set_cell_size(const Vector2i &p_size);
	Vector2i get_cell_size() const { return cell_size; }
	// Shows a character facing p_direction, walking (p_walk_distance cells since it last stood still, as
	// JRPGMover::get_walk_distance() counts them) or standing.
	void show_state(JRPGSystemData::Direction p_direction, bool p_moving, real_t p_walk_distance);
	// The walk frame at p_walk_distance cells into a walk, with p_frame_count frames: 1 on the first half cell, then
	// round in order (1, 2, 3, 0, ... with four frames).
	static int get_walk_frame(real_t p_walk_distance, int p_frame_count);

	JRPGCharacterSprite();
};
