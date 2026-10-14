/**************************************************************************/
/*  jrpg_system_data.cpp                                                  */
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

#include "jrpg_system_data.h"

#include "core/crypto/crypto_core.h"
#include "core/object/class_db.h"

void JRPGSystemData::set_game_title(const String &p_title) {
	game_title = p_title;
	emit_changed();
}

void JRPGSystemData::set_game_id(const String &p_id) {
	game_id = p_id;
	emit_changed();
}

String JRPGSystemData::generate_game_id() {
	uint8_t bytes[8];
	CryptoCore::RandomGenerator rng;
	ERR_FAIL_COND_V(rng.init() != OK, String());
	ERR_FAIL_COND_V(rng.get_random_bytes(bytes, sizeof(bytes)) != OK, String());
	return String::hex_encode_buffer(bytes, sizeof(bytes));
}

void JRPGSystemData::set_currency_unit(const String &p_unit) {
	currency_unit = p_unit;
	emit_changed();
}

void JRPGSystemData::set_starting_party(const PackedInt32Array &p_actor_ids) {
	starting_party = p_actor_ids;
	emit_changed();
}

void JRPGSystemData::set_start_map_id(int p_map_id) {
	start_map_id = MAX(0, p_map_id);
	emit_changed();
}

void JRPGSystemData::set_start_cell(const Vector2i &p_cell) {
	start_cell = p_cell;
	emit_changed();
}

void JRPGSystemData::set_start_direction(Direction p_direction) {
	ERR_FAIL_INDEX(int(p_direction), 4);
	start_direction = p_direction;
	emit_changed();
}

void JRPGSystemData::set_title_image(const String &p_image) {
	title_image = p_image;
	emit_changed();
}

void JRPGSystemData::set_draw_game_title(bool p_draw) {
	draw_game_title = p_draw;
	emit_changed();
}

void JRPGSystemData::set_music(Music p_slot, const Ref<JRPGAudioCue> &p_cue) {
	ERR_FAIL_INDEX(p_slot, MUSIC_MAX);
	music[p_slot] = p_cue;
	emit_changed();
}

Ref<JRPGAudioCue> JRPGSystemData::get_music(Music p_slot) const {
	ERR_FAIL_INDEX_V(p_slot, MUSIC_MAX, Ref<JRPGAudioCue>());
	return music[p_slot];
}

void JRPGSystemData::set_sound(Sound p_slot, const Ref<JRPGAudioCue> &p_cue) {
	ERR_FAIL_INDEX(p_slot, SOUND_MAX);
	sounds[p_slot] = p_cue;
	emit_changed();
}

Ref<JRPGAudioCue> JRPGSystemData::get_sound(Sound p_slot) const {
	ERR_FAIL_INDEX_V(p_slot, SOUND_MAX, Ref<JRPGAudioCue>());
	return sounds[p_slot];
}

void JRPGSystemData::set_window_tone(const Vector3i &p_tone) {
	window_tone = p_tone.clamp(Vector3i(-255, -255, -255), Vector3i(255, 255, 255));
	emit_changed();
}

void JRPGSystemData::set_battle_rules(const Ref<JRPGBattleRules> &p_rules) {
	battle_rules = p_rules;
	emit_changed();
}

PackedStringArray JRPGSystemData::validate() const {
	PackedStringArray problems;
	if (!has_start_position()) {
		problems.push_back("The player start position is not set.");
	}
	if (starting_party.is_empty()) {
		problems.push_back("The starting party is empty.");
	}
	return problems;
}

JRPGSystemData::JRPGSystemData() {
	battle_rules.instantiate();
}

void JRPGSystemData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_game_title", "title"), &JRPGSystemData::set_game_title);
	ClassDB::bind_method(D_METHOD("get_game_title"), &JRPGSystemData::get_game_title);
	ClassDB::bind_method(D_METHOD("set_game_id", "id"), &JRPGSystemData::set_game_id);
	ClassDB::bind_method(D_METHOD("get_game_id"), &JRPGSystemData::get_game_id);
	ClassDB::bind_static_method("JRPGSystemData", D_METHOD("generate_game_id"), &JRPGSystemData::generate_game_id);
	ClassDB::bind_method(D_METHOD("set_currency_unit", "unit"), &JRPGSystemData::set_currency_unit);
	ClassDB::bind_method(D_METHOD("get_currency_unit"), &JRPGSystemData::get_currency_unit);
	ClassDB::bind_method(D_METHOD("set_starting_party", "actor_ids"), &JRPGSystemData::set_starting_party);
	ClassDB::bind_method(D_METHOD("get_starting_party"), &JRPGSystemData::get_starting_party);
	ClassDB::bind_method(D_METHOD("set_start_map_id", "map_id"), &JRPGSystemData::set_start_map_id);
	ClassDB::bind_method(D_METHOD("get_start_map_id"), &JRPGSystemData::get_start_map_id);
	ClassDB::bind_method(D_METHOD("set_start_cell", "cell"), &JRPGSystemData::set_start_cell);
	ClassDB::bind_method(D_METHOD("get_start_cell"), &JRPGSystemData::get_start_cell);
	ClassDB::bind_method(D_METHOD("set_start_direction", "direction"), &JRPGSystemData::set_start_direction);
	ClassDB::bind_method(D_METHOD("get_start_direction"), &JRPGSystemData::get_start_direction);
	ClassDB::bind_method(D_METHOD("has_start_position"), &JRPGSystemData::has_start_position);
	ClassDB::bind_method(D_METHOD("set_title_image", "image"), &JRPGSystemData::set_title_image);
	ClassDB::bind_method(D_METHOD("get_title_image"), &JRPGSystemData::get_title_image);
	ClassDB::bind_method(D_METHOD("set_draw_game_title", "draw"), &JRPGSystemData::set_draw_game_title);
	ClassDB::bind_method(D_METHOD("is_drawing_game_title"), &JRPGSystemData::is_drawing_game_title);
	ClassDB::bind_method(D_METHOD("set_music", "slot", "cue"), &JRPGSystemData::set_music);
	ClassDB::bind_method(D_METHOD("get_music", "slot"), &JRPGSystemData::get_music);
	ClassDB::bind_method(D_METHOD("set_sound", "slot", "cue"), &JRPGSystemData::set_sound);
	ClassDB::bind_method(D_METHOD("get_sound", "slot"), &JRPGSystemData::get_sound);
	ClassDB::bind_method(D_METHOD("set_window_tone", "tone"), &JRPGSystemData::set_window_tone);
	ClassDB::bind_method(D_METHOD("get_window_tone"), &JRPGSystemData::get_window_tone);
	ClassDB::bind_method(D_METHOD("set_battle_rules", "rules"), &JRPGSystemData::set_battle_rules);
	ClassDB::bind_method(D_METHOD("get_battle_rules"), &JRPGSystemData::get_battle_rules);
	ClassDB::bind_method(D_METHOD("validate"), &JRPGSystemData::validate);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "game_title"), "set_game_title", "get_game_title");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "game_id"), "set_game_id", "get_game_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "currency_unit"), "set_currency_unit", "get_currency_unit");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "starting_party"), "set_starting_party", "get_starting_party");
	ADD_GROUP("Start Position", "start_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "start_map_id", PROPERTY_HINT_RANGE, "0,9999,1,or_greater"), "set_start_map_id", "get_start_map_id");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "start_cell"), "set_start_cell", "get_start_cell");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "start_direction", PROPERTY_HINT_ENUM, "Up,Right,Down,Left"), "set_start_direction", "get_start_direction");
	ADD_GROUP("Title Screen", "");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "title_image", PROPERTY_HINT_FILE, "*.png,*.webp,*.jpg"), "set_title_image", "get_title_image");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_game_title"), "set_draw_game_title", "is_drawing_game_title");
	ADD_GROUP("Music", "music_");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "music_title", PROPERTY_HINT_RESOURCE_TYPE, "JRPGAudioCue"), "set_music", "get_music", MUSIC_TITLE);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "music_battle", PROPERTY_HINT_RESOURCE_TYPE, "JRPGAudioCue"), "set_music", "get_music", MUSIC_BATTLE);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "music_victory", PROPERTY_HINT_RESOURCE_TYPE, "JRPGAudioCue"), "set_music", "get_music", MUSIC_VICTORY);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "music_defeat", PROPERTY_HINT_RESOURCE_TYPE, "JRPGAudioCue"), "set_music", "get_music", MUSIC_DEFEAT);
	ADD_GROUP("Sounds", "sound_");
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "sound_cursor", PROPERTY_HINT_RESOURCE_TYPE, "JRPGAudioCue"), "set_sound", "get_sound", SOUND_CURSOR);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "sound_ok", PROPERTY_HINT_RESOURCE_TYPE, "JRPGAudioCue"), "set_sound", "get_sound", SOUND_OK);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "sound_cancel", PROPERTY_HINT_RESOURCE_TYPE, "JRPGAudioCue"), "set_sound", "get_sound", SOUND_CANCEL);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "sound_buzzer", PROPERTY_HINT_RESOURCE_TYPE, "JRPGAudioCue"), "set_sound", "get_sound", SOUND_BUZZER);
	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "window_tone"), "set_window_tone", "get_window_tone");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "battle_rules", PROPERTY_HINT_RESOURCE_TYPE, "JRPGBattleRules"), "set_battle_rules", "get_battle_rules");

	BIND_ENUM_CONSTANT(DIRECTION_UP);
	BIND_ENUM_CONSTANT(DIRECTION_RIGHT);
	BIND_ENUM_CONSTANT(DIRECTION_DOWN);
	BIND_ENUM_CONSTANT(DIRECTION_LEFT);
	BIND_ENUM_CONSTANT(MUSIC_TITLE);
	BIND_ENUM_CONSTANT(MUSIC_BATTLE);
	BIND_ENUM_CONSTANT(MUSIC_VICTORY);
	BIND_ENUM_CONSTANT(MUSIC_DEFEAT);
	BIND_ENUM_CONSTANT(MUSIC_MAX);
	BIND_ENUM_CONSTANT(SOUND_CURSOR);
	BIND_ENUM_CONSTANT(SOUND_OK);
	BIND_ENUM_CONSTANT(SOUND_CANCEL);
	BIND_ENUM_CONSTANT(SOUND_BUZZER);
	BIND_ENUM_CONSTANT(SOUND_MAX);
}
