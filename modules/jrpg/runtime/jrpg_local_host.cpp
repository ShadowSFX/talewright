/**************************************************************************/
/*  jrpg_local_host.cpp                                                   */
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

#include "jrpg_local_host.h"

#include "core/object/object.h"

#include "modules/jrpg/core/jrpg_database.h" // IWYU pragma: keep (the scene manager returns these references inline).
#include "modules/jrpg/core/jrpg_game_state.h" // IWYU pragma: keep
#include "modules/jrpg/core/jrpg_map_cell_cache.h" // IWYU pragma: keep
#include "modules/jrpg/core/jrpg_map_list.h" // IWYU pragma: keep
#include "modules/jrpg/runtime/jrpg_scene_manager.h"

JRPGSceneManager *JRPGLocalHost::_manager() const {
	return Object::cast_to<JRPGSceneManager>(ObjectDB::get_instance(manager));
}

void JRPGLocalHost::set_manager(JRPGSceneManager *p_manager) {
	manager = p_manager ? p_manager->get_instance_id() : ObjectID();
}

Error JRPGLocalHost::transfer_player(int p_map_id, const Vector2i &p_cell, int p_direction, Fade p_fade) {
	JRPGSceneManager *scene_manager = _manager();
	ERR_FAIL_NULL_V(scene_manager, ERR_UNAVAILABLE);
	static const JRPGSceneManager::Fade FADES[3] = { JRPGSceneManager::FADE_BLACK, JRPGSceneManager::FADE_WHITE, JRPGSceneManager::FADE_NONE };
	ERR_FAIL_INDEX_V(int(p_fade), 3, ERR_INVALID_PARAMETER);
	return scene_manager->transfer(p_map_id, p_cell, p_direction, FADES[p_fade]); // Refusals warn there.
}

Error JRPGLocalHost::set_move_route(int p_character, const Ref<JRPGMoveRoute> &p_route) {
	JRPGSceneManager *scene_manager = _manager();
	ERR_FAIL_NULL_V(scene_manager, ERR_UNAVAILABLE);
	return scene_manager->set_move_route(p_character, p_route);
}

bool JRPGLocalHost::is_route_running(int p_character) const {
	const JRPGSceneManager *scene_manager = _manager();
	return scene_manager && scene_manager->is_route_running(p_character);
}

bool JRPGLocalHost::is_transferring() const {
	const JRPGSceneManager *scene_manager = _manager();
	return scene_manager && scene_manager->is_transferring();
}
