/**************************************************************************/
/*  jrpg_event_host.cpp                                                   */
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

#include "jrpg_event_host.h"

#include "core/object/class_db.h"

Error JRPGEventHost::transfer_player(int p_map_id, const Vector2i &p_cell, int p_direction, Fade p_fade) {
	Error result = ERR_UNAVAILABLE;
	GDVIRTUAL_CALL(_transfer_player, p_map_id, p_cell, p_direction, int(p_fade), result);
	return result;
}

bool JRPGEventHost::is_transferring() const {
	bool transferring = false;
	GDVIRTUAL_CALL(_is_transferring, transferring);
	return transferring;
}

Error JRPGEventHost::set_move_route(int p_character, const Ref<JRPGMoveRoute> &p_route) {
	Error result = ERR_UNAVAILABLE;
	GDVIRTUAL_CALL(_set_move_route, p_character, p_route, result);
	return result;
}

bool JRPGEventHost::is_route_running(int p_character) const {
	bool running = false;
	GDVIRTUAL_CALL(_is_route_running, p_character, running);
	return running;
}

void JRPGEventHost::_bind_methods() {
	ClassDB::bind_method(D_METHOD("transfer_player", "map_id", "cell", "direction", "fade"), &JRPGEventHost::transfer_player);
	ClassDB::bind_method(D_METHOD("is_transferring"), &JRPGEventHost::is_transferring);
	ClassDB::bind_method(D_METHOD("set_move_route", "character", "route"), &JRPGEventHost::set_move_route);
	ClassDB::bind_method(D_METHOD("is_route_running", "character"), &JRPGEventHost::is_route_running);

	GDVIRTUAL_BIND(_transfer_player, "map_id", "cell", "direction", "fade");
	GDVIRTUAL_BIND(_is_transferring);
	GDVIRTUAL_BIND(_set_move_route, "character", "route");
	GDVIRTUAL_BIND(_is_route_running, "character");

	BIND_CONSTANT(DIRECTION_RETAIN);
	BIND_CONSTANT(CHARACTER_PLAYER);
	BIND_ENUM_CONSTANT(FADE_BLACK);
	BIND_ENUM_CONSTANT(FADE_WHITE);
	BIND_ENUM_CONSTANT(FADE_NONE);
}
