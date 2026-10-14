/**************************************************************************/
/*  jrpg_actor_data.cpp                                                   */
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

#include "jrpg_actor_data.h"

#include "core/object/class_db.h"

void JRPGActorData::set_nickname(const String &p_nickname) {
	nickname = p_nickname;
	emit_changed();
}

void JRPGActorData::set_class_id(int p_class_id) {
	class_id = MAX(0, p_class_id);
	emit_changed();
}

void JRPGActorData::set_initial_level(int p_level) {
	initial_level = CLAMP(p_level, 1, LEVEL_LIMIT);
	emit_changed();
}

void JRPGActorData::set_max_level(int p_level) {
	max_level = CLAMP(p_level, 1, LEVEL_LIMIT);
	emit_changed();
}

void JRPGActorData::set_profile(const String &p_profile) {
	profile = p_profile;
	emit_changed();
}

void JRPGActorData::set_face(const String &p_face) {
	face = p_face;
	emit_changed();
}

void JRPGActorData::set_character(const String &p_character) {
	character = p_character;
	emit_changed();
}

void JRPGActorData::set_battler(const String &p_battler) {
	battler = p_battler;
	emit_changed();
}

void JRPGActorData::set_initial_equipment(const PackedInt32Array &p_items) {
	initial_equipment = p_items;
	emit_changed();
}

void JRPGActorData::set_traits(const Array &p_traits) {
	traits = p_traits;
	emit_changed();
}

void JRPGActorData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_nickname", "nickname"), &JRPGActorData::set_nickname);
	ClassDB::bind_method(D_METHOD("get_nickname"), &JRPGActorData::get_nickname);
	ClassDB::bind_method(D_METHOD("set_class_id", "class_id"), &JRPGActorData::set_class_id);
	ClassDB::bind_method(D_METHOD("get_class_id"), &JRPGActorData::get_class_id);
	ClassDB::bind_method(D_METHOD("set_initial_level", "level"), &JRPGActorData::set_initial_level);
	ClassDB::bind_method(D_METHOD("get_initial_level"), &JRPGActorData::get_initial_level);
	ClassDB::bind_method(D_METHOD("set_max_level", "level"), &JRPGActorData::set_max_level);
	ClassDB::bind_method(D_METHOD("get_max_level"), &JRPGActorData::get_max_level);
	ClassDB::bind_method(D_METHOD("get_start_level"), &JRPGActorData::get_start_level);
	ClassDB::bind_method(D_METHOD("set_profile", "profile"), &JRPGActorData::set_profile);
	ClassDB::bind_method(D_METHOD("get_profile"), &JRPGActorData::get_profile);
	ClassDB::bind_method(D_METHOD("set_face", "face"), &JRPGActorData::set_face);
	ClassDB::bind_method(D_METHOD("get_face"), &JRPGActorData::get_face);
	ClassDB::bind_method(D_METHOD("set_character", "character"), &JRPGActorData::set_character);
	ClassDB::bind_method(D_METHOD("get_character"), &JRPGActorData::get_character);
	ClassDB::bind_method(D_METHOD("set_battler", "battler"), &JRPGActorData::set_battler);
	ClassDB::bind_method(D_METHOD("get_battler"), &JRPGActorData::get_battler);
	ClassDB::bind_method(D_METHOD("set_initial_equipment", "item_ids"), &JRPGActorData::set_initial_equipment);
	ClassDB::bind_method(D_METHOD("get_initial_equipment"), &JRPGActorData::get_initial_equipment);
	ClassDB::bind_method(D_METHOD("set_traits", "traits"), &JRPGActorData::set_traits);
	ClassDB::bind_method(D_METHOD("get_traits"), &JRPGActorData::get_traits);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "nickname"), "set_nickname", "get_nickname");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "class_id", PROPERTY_HINT_RANGE, "0,9999,1,or_greater"), "set_class_id", "get_class_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "initial_level", PROPERTY_HINT_RANGE, "1,99,1"), "set_initial_level", "get_initial_level");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_level", PROPERTY_HINT_RANGE, "1,99,1"), "set_max_level", "get_max_level");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "profile", PROPERTY_HINT_MULTILINE_TEXT), "set_profile", "get_profile");
	ADD_GROUP("Images", "");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "face", PROPERTY_HINT_FILE, "*.png,*.webp"), "set_face", "get_face");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "character", PROPERTY_HINT_FILE, "*.png,*.webp"), "set_character", "get_character");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "battler", PROPERTY_HINT_FILE, "*.png,*.webp"), "set_battler", "get_battler");
	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "initial_equipment"), "set_initial_equipment", "get_initial_equipment");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "traits", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_traits", "get_traits");

	BIND_CONSTANT(LEVEL_LIMIT);
}
