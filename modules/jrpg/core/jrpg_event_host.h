/**************************************************************************/
/*  jrpg_event_host.h                                                     */
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

#include "core/math/vector2i.h"
#include "core/object/gdvirtual.gen.h"
#include "core/object/ref_counted.h"

// What events ask of the world beyond presenting things to the player (task 9.5.3, D-071): move the player to another
// place, and later open the scenes other epics build (menu, battle, save, game over, title). A single-player game's
// host is its scene manager (JRPGSceneManager); online, the zone server's host acts on the world and tells the
// clients. The event runner gives its host to every interpreter (JRPGInterpreter::get_host). A script may implement
// it by overriding the virtual methods.
// Authority (D-007): server.
class JRPGEventHost : public RefCounted {
	GDCLASS(JRPGEventHost, RefCounted);

public:
	static constexpr int DIRECTION_RETAIN = -1; // The player keeps facing the way they face.
	enum Fade {
		FADE_BLACK,
		FADE_WHITE,
		FADE_NONE,
	};

protected:
	static void _bind_methods();

	GDVIRTUAL4R(Error, _transfer_player, int, Vector2i, int, int)
	GDVIRTUAL0RC(bool, _is_transferring)

public:
	// Moves the player to p_cell of map p_map_id facing p_direction (0-3, or DIRECTION_RETAIN), through p_fade. OK
	// once the transfer started (or happened, without a fade); an error, with a warning, when it is refused (a map the
	// game lacks, a cell outside it, another transfer under way). ERR_UNAVAILABLE when the host cannot transfer.
	virtual Error transfer_player(int p_map_id, const Vector2i &p_cell, int p_direction, Fade p_fade);
	// A transfer is under way (its fade has not ended).
	virtual bool is_transferring() const;
};

VARIANT_ENUM_CAST(JRPGEventHost::Fade);
