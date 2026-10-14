/**************************************************************************/
/*  jrpg_system_data.h                                                    */
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

#include "jrpg_audio_cue.h"
#include "jrpg_battle_rules.h"

// Project-wide JRPG settings (the "System" part of the database): game title and id, starting party and
// position, title screen, system music and UI sounds, window tone and battle rules. Only the vertical-slice fields
// exist so far (plan/specs/02_database_model.md, "Slice field set").
class JRPGSystemData : public Resource {
	GDCLASS(JRPGSystemData, Resource);

public:
	// Facing directions in Talewright's clockwise-from-north order (spec 05, spec 07).
	enum Direction {
		DIRECTION_UP,
		DIRECTION_RIGHT,
		DIRECTION_DOWN,
		DIRECTION_LEFT,
	};

	enum Music {
		MUSIC_TITLE,
		MUSIC_BATTLE,
		MUSIC_VICTORY,
		MUSIC_DEFEAT,
		MUSIC_MAX,
	};

	enum Sound {
		SOUND_CURSOR,
		SOUND_OK,
		SOUND_CANCEL,
		SOUND_BUZZER,
		SOUND_MAX,
	};

private:
	String game_title;
	String game_id;
	String currency_unit = "Gold";
	PackedInt32Array starting_party;
	int start_map_id = 0; // 0 = not set; a new game cannot start until it is (RM-SYS-007).
	Vector2i start_cell;
	Direction start_direction = DIRECTION_DOWN;
	String title_image;
	bool draw_game_title = true;
	Ref<JRPGAudioCue> music[MUSIC_MAX];
	Ref<JRPGAudioCue> sounds[SOUND_MAX];
	Vector3i window_tone;
	Ref<JRPGBattleRules> battle_rules;

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);

public:
	void set_game_title(const String &p_title);
	String get_game_title() const { return game_title; }
	void set_game_id(const String &p_id);
	String get_game_id() const { return game_id; }
	// A fresh random id (16 hexadecimal characters) for new projects (RM-NEW-027 / RM-SYS-026).
	static String generate_game_id();
	void set_currency_unit(const String &p_unit);
	String get_currency_unit() const { return currency_unit; }
	void set_starting_party(const PackedInt32Array &p_actor_ids);
	PackedInt32Array get_starting_party() const { return starting_party; }

	void set_start_map_id(int p_map_id);
	int get_start_map_id() const { return start_map_id; }
	void set_start_cell(const Vector2i &p_cell);
	Vector2i get_start_cell() const { return start_cell; }
	void set_start_direction(Direction p_direction);
	Direction get_start_direction() const { return start_direction; }
	bool has_start_position() const { return start_map_id > 0; }

	void set_title_image(const String &p_image);
	String get_title_image() const { return title_image; }
	void set_draw_game_title(bool p_draw);
	bool is_drawing_game_title() const { return draw_game_title; }

	void set_music(Music p_slot, const Ref<JRPGAudioCue> &p_cue);
	Ref<JRPGAudioCue> get_music(Music p_slot) const;
	void set_sound(Sound p_slot, const Ref<JRPGAudioCue> &p_cue);
	Ref<JRPGAudioCue> get_sound(Sound p_slot) const;

	// Tint added to every in-game window, each component -255..255 (RM-SYS-005).
	void set_window_tone(const Vector3i &p_tone);
	Vector3i get_window_tone() const { return window_tone; }

	// Null until set; code that reads rules falls back to JRPGBattleRules' defaults.
	void set_battle_rules(const Ref<JRPGBattleRules> &p_rules);
	Ref<JRPGBattleRules> get_battle_rules() const { return battle_rules; }

	// Problems that prevent playing the project, as readable sentences (empty when ready).
	PackedStringArray validate() const;
};

VARIANT_ENUM_CAST(JRPGSystemData::Direction);
VARIANT_ENUM_CAST(JRPGSystemData::Music);
VARIANT_ENUM_CAST(JRPGSystemData::Sound);
