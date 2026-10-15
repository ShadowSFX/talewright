/**************************************************************************/
/*  jrpg_event_data.h                                                     */
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

#include "jrpg_system_data.h"

#include "core/io/resource.h"
#include "core/math/vector2i.h"
#include "core/variant/binder_common.h" // IWYU pragma: keep (VARIANT_ENUM_CAST).
#include "core/variant/typed_array.h"

class JRPGCommandList;
class JRPGGameState;
class JRPGMoveRoute;

// One page of a map event (spec 03, RM-EVT-003, RM-EVT-022..043): when it is the event's active page (its appearance
// conditions), how the event looks and moves while it is, how it meets characters (priority) and what starts it
// (trigger). Its commands run in the command model (task 9.2); its custom move route (move_route) is followed when
// the movement type is Custom (task 9.6.1).
// Authority (D-007, D-025): server. The zone decides which page is active (a per-player event's from the triggering
// player's character state); clients only show the page they are told.
class JRPGEventPage : public Resource {
	GDCLASS(JRPGEventPage, Resource);

public:
	enum SelfSwitch {
		SELF_SWITCH_A,
		SELF_SWITCH_B,
		SELF_SWITCH_C,
		SELF_SWITCH_D,
	};
	enum MovementType {
		MOVEMENT_FIXED,
		MOVEMENT_RANDOM,
		MOVEMENT_APPROACH,
		MOVEMENT_CUSTOM,
	};
	enum Priority {
		PRIORITY_BELOW, // Characters walk over it (a tile graphic's own passability decides instead).
		PRIORITY_SAME, // Blocks characters, drawn among them.
		PRIORITY_ABOVE, // Characters walk under it.
	};
	enum Trigger {
		TRIGGER_ACTION,
		TRIGGER_PLAYER_TOUCH,
		TRIGGER_EVENT_TOUCH,
		TRIGGER_AUTORUN,
		TRIGGER_PARALLEL,
	};

	static constexpr int MIN_SPEED = 1;
	static constexpr int MAX_SPEED = 6;
	static constexpr int MIN_FREQUENCY = 1;
	static constexpr int MAX_FREQUENCY = 5;

private:
	// Appearance conditions (RM-EVT-024..029): each enabled one must hold.
	bool switch1_enabled = false;
	int switch1_id = 1;
	bool switch2_enabled = false;
	int switch2_id = 1;
	bool variable_enabled = false;
	int variable_id = 1;
	int variable_value = 0;
	bool self_switch_enabled = false;
	SelfSwitch self_switch = SELF_SWITCH_A;
	bool item_enabled = false;
	int item_id = 1;
	bool actor_enabled = false;
	int actor_id = 1;

	// Graphic (RM-EVT-030): a character image (uid:// or res:// string, as actors hold theirs) with a facing and a frame,
	// or a tile of the map's tileset (its palette index; 0 = none), or nothing.
	String character;
	JRPGSystemData::Direction direction = JRPGSystemData::DIRECTION_DOWN;
	int pattern = 0;
	int tile = 0;

	// Autonomous movement (RM-EVT-031..033) and options (RM-EVT-034..037).
	MovementType movement_type = MOVEMENT_FIXED;
	int move_speed = 3;
	int move_frequency = 3;
	bool walking_animation = true;
	bool stepping_animation = false;
	bool direction_fix = false;
	bool through = false;
	Ref<JRPGMoveRoute> move_route; // The Custom movement's route; null: none.

	Priority priority = PRIORITY_BELOW;
	Trigger trigger = TRIGGER_ACTION;
	Ref<JRPGCommandList> commands; // Null: no commands.

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);
	void _validate_property(PropertyInfo &p_property) const;

public:
	void set_switch1_enabled(bool p_enabled);
	bool is_switch1_enabled() const { return switch1_enabled; }
	void set_switch1_id(int p_id);
	int get_switch1_id() const { return switch1_id; }
	void set_switch2_enabled(bool p_enabled);
	bool is_switch2_enabled() const { return switch2_enabled; }
	void set_switch2_id(int p_id);
	int get_switch2_id() const { return switch2_id; }
	void set_variable_enabled(bool p_enabled);
	bool is_variable_enabled() const { return variable_enabled; }
	void set_variable_id(int p_id);
	int get_variable_id() const { return variable_id; }
	void set_variable_value(int p_value);
	int get_variable_value() const { return variable_value; }
	void set_self_switch_enabled(bool p_enabled);
	bool is_self_switch_enabled() const { return self_switch_enabled; }
	void set_self_switch(SelfSwitch p_switch);
	SelfSwitch get_self_switch() const { return self_switch; }
	void set_item_enabled(bool p_enabled);
	bool is_item_enabled() const { return item_enabled; }
	void set_item_id(int p_id);
	int get_item_id() const { return item_id; }
	void set_actor_enabled(bool p_enabled);
	bool is_actor_enabled() const { return actor_enabled; }
	void set_actor_id(int p_id);
	int get_actor_id() const { return actor_id; }

	void set_character(const String &p_character);
	String get_character() const { return character; }
	void set_direction(JRPGSystemData::Direction p_direction);
	JRPGSystemData::Direction get_direction() const { return direction; }
	void set_pattern(int p_pattern);
	int get_pattern() const { return pattern; }
	void set_tile(int p_tile);
	int get_tile() const { return tile; }

	void set_movement_type(MovementType p_type);
	MovementType get_movement_type() const { return movement_type; }
	void set_move_speed(int p_speed);
	int get_move_speed() const { return move_speed; }
	void set_move_frequency(int p_frequency);
	int get_move_frequency() const { return move_frequency; }
	void set_walking_animation(bool p_enabled);
	bool has_walking_animation() const { return walking_animation; }
	void set_stepping_animation(bool p_enabled);
	bool has_stepping_animation() const { return stepping_animation; }
	void set_direction_fix(bool p_enabled);
	bool is_direction_fix() const { return direction_fix; }
	void set_through(bool p_enabled);
	bool is_through() const { return through; }

	void set_priority(Priority p_priority);
	Priority get_priority() const { return priority; }
	void set_trigger(Trigger p_trigger);
	Trigger get_trigger() const { return trigger; }
	// What the page runs when it starts (task 9.2.2.3); null for none.
	void set_move_route(const Ref<JRPGMoveRoute> &p_route);
	Ref<JRPGMoveRoute> get_move_route() const;
	void set_commands(const Ref<JRPGCommandList> &p_commands);
	Ref<JRPGCommandList> get_commands() const;

	bool has_any_condition() const { return switch1_enabled || switch2_enabled || variable_enabled || self_switch_enabled || item_enabled || actor_enabled; }
	// Whether every enabled condition holds in p_state for event p_event_id of map p_map_id (its self switches). A page
	// without conditions always qualifies. The variable condition is "value >= the page's number" and needs a number:
	// a variable holding text never meets it.
	bool are_conditions_met(const Ref<JRPGGameState> &p_state, int p_map_id, int p_event_id) const;
	// The self switch letter ("A".."D") a SelfSwitch stands for.
	static String get_self_switch_letter(SelfSwitch p_switch);
};

VARIANT_ENUM_CAST(JRPGEventPage::SelfSwitch);
VARIANT_ENUM_CAST(JRPGEventPage::MovementType);
VARIANT_ENUM_CAST(JRPGEventPage::Priority);
VARIANT_ENUM_CAST(JRPGEventPage::Trigger);

// A map event (spec 03, RM-EVT-001..004, RM-EVT-019..021): data stored in its map's JRPGMapData, never a scene node,
// so the core and zone servers read it without a scene and duplicating nodes can never clone an id. Its active page
// is the highest-numbered page whose conditions all hold (RM-EVT-004); with none, the event is absent from the map.
// Authority (D-007): server.
class JRPGEventData : public Resource {
	GDCLASS(JRPGEventData, Resource);

	int event_id = 0;
	String display_name;
	String note;
	Vector2i cell;
	TypedArray<JRPGEventPage> pages;

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);
	void _validate_property(PropertyInfo &p_property) const;

public:
	static constexpr int MAX_PAGES = 20; // RM-EVT-003.

	// Unique within its map; JRPGMapData::add_event gives the next free one (RM-EVT-019).
	void set_event_id(int p_id);
	int get_event_id() const { return event_id; }
	void set_display_name(const String &p_name);
	String get_display_name() const { return display_name; }
	void set_note(const String &p_note);
	String get_note() const { return note; }
	// Where the event starts when its map is entered.
	void set_cell(const Vector2i &p_cell);
	Vector2i get_cell() const { return cell; }
	// Up to MAX_PAGES pages; null entries are left out and extra pages are cut, with an error.
	void set_pages(const TypedArray<JRPGEventPage> &p_pages);
	TypedArray<JRPGEventPage> get_pages() const { return pages; }
	int get_page_count() const { return pages.size(); }
	Ref<JRPGEventPage> get_page(int p_index) const;

	// The index of the active page in p_state (map p_map_id), or -1 when no page qualifies.
	int find_active_page(const Ref<JRPGGameState> &p_state, int p_map_id) const;
};
