/**************************************************************************/
/*  jrpg_misc_data.cpp                                                    */
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

#include "jrpg_misc_data.h"

#include "core/object/class_db.h"

void JRPGAnimationData::set_effect(const String &p_effect) {
	effect = p_effect;
	emit_changed();
}

void JRPGAnimationData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_effect", "effect"), &JRPGAnimationData::set_effect);
	ClassDB::bind_method(D_METHOD("get_effect"), &JRPGAnimationData::get_effect);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "effect", PROPERTY_HINT_FILE, "*.tscn,*.scn,*.tres,*.res"), "set_effect", "get_effect");
}

void JRPGTilesetData::set_tileset_uid(const String &p_uid) {
	tileset_uid = p_uid;
	emit_changed();
}

void JRPGTilesetData::set_palette(const Array &p_palette) {
	palette = p_palette;
	emit_changed();
}

void JRPGTilesetData::set_flags(const PackedInt32Array &p_flags) {
	flags = p_flags;
	emit_changed();
}

void JRPGTilesetData::set_flags_hash(const String &p_hash) {
	flags_hash = p_hash;
	emit_changed();
}

void JRPGTilesetData::set_edge_mode(EdgeMode p_mode) {
	ERR_FAIL_INDEX(int(p_mode), EDGE_BORDER + 1);
	edge_mode = p_mode;
	emit_changed();
}

int JRPGTilesetData::get_tile_flags(int p_palette_index) const {
	return (p_palette_index > 0 && p_palette_index < flags.size()) ? flags[p_palette_index] : 0;
}

int JRPGTilesetData::pack_flags(PassMode p_pass, int p_no_entry, int p_no_exit, int p_flags, int p_terrain_tag) {
	const uint32_t known = FLAG_OVERHEAD | FLAG_LADDER | FLAG_BUSH | FLAG_COUNTER | FLAG_DAMAGE;
	uint32_t word = uint32_t(p_pass) & FLAG_PASS_MASK;
	word |= (uint32_t(p_no_entry) & 0xF) << FLAG_NO_ENTRY_SHIFT;
	word |= (uint32_t(p_no_exit) & 0xF) << FLAG_NO_EXIT_SHIFT;
	word |= uint32_t(p_flags) & known;
	word |= (uint32_t(p_terrain_tag) & 0xF) << FLAG_TERRAIN_TAG_SHIFT;
	return int(word);
}

void JRPGTilesetData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tileset_uid", "uid"), &JRPGTilesetData::set_tileset_uid);
	ClassDB::bind_method(D_METHOD("get_tileset_uid"), &JRPGTilesetData::get_tileset_uid);
	ClassDB::bind_method(D_METHOD("set_palette", "palette"), &JRPGTilesetData::set_palette);
	ClassDB::bind_method(D_METHOD("get_palette"), &JRPGTilesetData::get_palette);
	ClassDB::bind_method(D_METHOD("set_flags", "flags"), &JRPGTilesetData::set_flags);
	ClassDB::bind_method(D_METHOD("get_flags"), &JRPGTilesetData::get_flags);
	ClassDB::bind_method(D_METHOD("set_flags_hash", "hash"), &JRPGTilesetData::set_flags_hash);
	ClassDB::bind_method(D_METHOD("get_flags_hash"), &JRPGTilesetData::get_flags_hash);
	ClassDB::bind_method(D_METHOD("set_edge_mode", "mode"), &JRPGTilesetData::set_edge_mode);
	ClassDB::bind_method(D_METHOD("get_edge_mode"), &JRPGTilesetData::get_edge_mode);
	ClassDB::bind_method(D_METHOD("get_tile_flags", "palette_index"), &JRPGTilesetData::get_tile_flags);
	ClassDB::bind_static_method("JRPGTilesetData", D_METHOD("pack_flags", "pass", "no_entry", "no_exit", "flags", "terrain_tag"), &JRPGTilesetData::pack_flags);
	ClassDB::bind_static_method("JRPGTilesetData", D_METHOD("get_pass_mode", "flags"), &JRPGTilesetData::get_pass_mode);
	ClassDB::bind_static_method("JRPGTilesetData", D_METHOD("get_no_entry", "flags"), &JRPGTilesetData::get_no_entry);
	ClassDB::bind_static_method("JRPGTilesetData", D_METHOD("get_no_exit", "flags"), &JRPGTilesetData::get_no_exit);
	ClassDB::bind_static_method("JRPGTilesetData", D_METHOD("get_terrain_tag", "flags"), &JRPGTilesetData::get_terrain_tag);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "tileset_uid", PROPERTY_HINT_FILE, "*.tres,*.res"), "set_tileset_uid", "get_tileset_uid");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "palette", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_palette", "get_palette");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "flags"), "set_flags", "get_flags");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "flags_hash"), "set_flags_hash", "get_flags_hash");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "edge_mode", PROPERTY_HINT_ENUM, "Extend,Border"), "set_edge_mode", "get_edge_mode");

	BIND_ENUM_CONSTANT(EDGE_EXTEND);
	BIND_ENUM_CONSTANT(EDGE_BORDER);
	BIND_ENUM_CONSTANT(PASS_OPEN);
	BIND_ENUM_CONSTANT(PASS_BLOCKED);
	BIND_ENUM_CONSTANT(PASS_TRANSPARENT);
	BIND_CONSTANT(FLAG_OVERHEAD);
	BIND_CONSTANT(FLAG_LADDER);
	BIND_CONSTANT(FLAG_BUSH);
	BIND_CONSTANT(FLAG_COUNTER);
	BIND_CONSTANT(FLAG_DAMAGE);
}

void JRPGCommonEventData::set_trigger(Trigger p_trigger) {
	ERR_FAIL_INDEX(int(p_trigger), TRIGGER_PARALLEL + 1);
	trigger = p_trigger;
	emit_changed();
}

void JRPGCommonEventData::set_switch_id(int p_id) {
	switch_id = MAX(1, p_id);
	emit_changed();
}

void JRPGCommonEventData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_trigger", "trigger"), &JRPGCommonEventData::set_trigger);
	ClassDB::bind_method(D_METHOD("get_trigger"), &JRPGCommonEventData::get_trigger);
	ClassDB::bind_method(D_METHOD("set_switch_id", "id"), &JRPGCommonEventData::set_switch_id);
	ClassDB::bind_method(D_METHOD("get_switch_id"), &JRPGCommonEventData::get_switch_id);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "trigger", PROPERTY_HINT_ENUM, "None,Autorun,Parallel"), "set_trigger", "get_trigger");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "switch_id", PROPERTY_HINT_RANGE, "1,9999,1,or_greater"), "set_switch_id", "get_switch_id");

	BIND_ENUM_CONSTANT(TRIGGER_NONE);
	BIND_ENUM_CONSTANT(TRIGGER_AUTORUN);
	BIND_ENUM_CONSTANT(TRIGGER_PARALLEL);
}
