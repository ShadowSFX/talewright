/**************************************************************************/
/*  jrpg_map_data.h                                                       */
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
#include "jrpg_autotile_resolver.h"

#include "core/io/resource.h"
#include "core/math/vector2i.h"
#include "core/templates/bit_field.h"
#include "core/variant/binder_common.h" // IWYU pragma: keep (VARIANT_ENUM_CAST).

// One map's data (spec 05, "JRPGMapData"; D-006): a built-in sub-resource of the map scene, so edits follow the
// scene's own undo history, and readable without nodes (JRPGMapFiles) by tools and servers. Tile layers hold palette
// indexes of the map's tileset (0 = empty; an autotile cell holds its kind, the resolver picks the shape), one int per
// cell row by row; shadows (4 quarter bits: 1 top left, 2 top right, 4 bottom left, 8 bottom right) and regions (0..255)
// one byte per cell. Every array always holds size.x * size.y cells: data loaded or set in another shape is fitted
// (padded or cut) with a warning.
class JRPGMapData : public Resource {
	GDCLASS(JRPGMapData, Resource);

public:
	// The map's looping axes, in the documented order (spec 05): none, vertical, horizontal, both.
	enum ScrollType {
		SCROLL_NONE,
		SCROLL_LOOP_VERTICAL,
		SCROLL_LOOP_HORIZONTAL,
		SCROLL_LOOP_BOTH,
	};
	// Tile layers, bottom to top.
	enum Layer {
		LAYER_GROUND,
		LAYER_GROUND_DETAIL,
		LAYER_OBJECTS,
		LAYER_UPPER,
		LAYER_COUNT,
	};

	static constexpr int MAX_SIDE = 512; // Cells per axis.
	static constexpr int DEFAULT_WIDTH = 20; // A 960 x 720 screen of 48 px tiles.
	static constexpr int DEFAULT_HEIGHT = 15;
	static constexpr int CURRENT_SCHEMA_VERSION = 1;
	static constexpr int SCHEMA_UNSTAMPED = -1;

private:
	int schema_version = SCHEMA_UNSTAMPED;
	int map_id = 0;
	int revision = 0;
	String display_name;
	String note;
	int tileset_id = 1;
	Vector2i size = Vector2i(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	PackedInt32Array layers[LAYER_COUNT];
	PackedByteArray shadows;
	PackedByteArray regions;
	ScrollType scroll_type = SCROLL_NONE;
	Ref<JRPGAudioCue> bgm;
	bool autoplay_bgm = false;
	Ref<JRPGAudioCue> bgs;
	bool autoplay_bgs = false;
	Dictionary battleback; // {image1, image2}: res:// or uid:// images; empty for the troop's default.
	Dictionary parallax; // {image, loop_x, loop_y, scroll_x, scroll_y, show_in_editor}.
	Array encounters; // {troop_id, weight, regions: PackedInt32Array} (spec 05, "Random encounters").
	int encounter_steps = 30;
	bool disable_dash = false;
	int next_event_id = 1;
	Array events; // JRPGEventData (spec 03), added by the event tasks.

	int _cell_count() const { return size.x * size.y; }
	void _fit_arrays();
	void _stamp_version();
	template <typename T>
	T _fitted(const T &p_array, const char *p_what) const;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;
	bool _set(const StringName &p_name, const Variant &p_value);

public:
	void set_schema_version(int p_version) { schema_version = p_version; }
	int get_schema_version() const { return schema_version; }
	void set_map_id(int p_id);
	int get_map_id() const { return map_id; }
	void set_revision(int p_revision);
	int get_revision() const { return revision; }
	// One more than before: a JRPGMap whose cells changed calls it as the editor saves its scene (spec 06 compares
	// revisions).
	void bump_revision();
	void set_display_name(const String &p_name);
	String get_display_name() const { return display_name; }
	void set_note(const String &p_note);
	String get_note() const { return note; }
	void set_tileset_id(int p_id);
	int get_tileset_id() const { return tileset_id; }

	// Sets the size (1..MAX_SIDE per axis) keeping the cells the old and new sizes share, from the top left.
	void set_size(const Vector2i &p_size);
	Vector2i get_size() const { return size; }
	void set_layers(const Array &p_layers);
	Array get_layers() const;
	void set_shadows(const PackedByteArray &p_shadows);
	PackedByteArray get_shadows() const { return shadows; }
	void set_regions(const PackedByteArray &p_regions);
	PackedByteArray get_regions() const { return regions; }

	bool has_cell(const Vector2i &p_cell) const { return p_cell.x >= 0 && p_cell.y >= 0 && p_cell.x < size.x && p_cell.y < size.y; }
	int get_cell_index(const Vector2i &p_cell) const { return p_cell.y * size.x + p_cell.x; }
	// The palette index of a cell of p_layer (0 outside the map).
	int get_cell(int p_layer, const Vector2i &p_cell) const;
	PackedInt32Array get_layer(int p_layer) const;
	int get_region(const Vector2i &p_cell) const;
	int get_shadow(const Vector2i &p_cell) const;
	// Writes p_values[i] to the cell at p_cells[i] (cell indexes, row by row) of p_layer: one paint stroke or undo
	// step. Emits cells_changed once, then changed. Values are palette indexes (0 = empty).
	void paint(int p_layer, const PackedInt32Array &p_cells, const PackedInt32Array &p_values);
	void paint_regions(const PackedInt32Array &p_cells, const PackedInt32Array &p_values);
	void paint_shadows(const PackedInt32Array &p_cells, const PackedInt32Array &p_values);

	void set_scroll_type(ScrollType p_type);
	ScrollType get_scroll_type() const { return scroll_type; }
	// The resolver flags of this map drawn with a tileset whose edge mode is p_edge_mode.
	BitField<JRPGAutotileResolver::Flags> get_resolver_flags(JRPGTilesetData::EdgeMode p_edge_mode) const;

	void set_bgm(const Ref<JRPGAudioCue> &p_cue);
	Ref<JRPGAudioCue> get_bgm() const { return bgm; }
	void set_autoplay_bgm(bool p_enabled);
	bool is_autoplay_bgm() const { return autoplay_bgm; }
	void set_bgs(const Ref<JRPGAudioCue> &p_cue);
	Ref<JRPGAudioCue> get_bgs() const { return bgs; }
	void set_autoplay_bgs(bool p_enabled);
	bool is_autoplay_bgs() const { return autoplay_bgs; }
	void set_battleback(const Dictionary &p_battleback);
	Dictionary get_battleback() const { return battleback; }
	void set_parallax(const Dictionary &p_parallax);
	Dictionary get_parallax() const { return parallax; }
	void set_encounters(const Array &p_encounters);
	Array get_encounters() const { return encounters; }
	void set_encounter_steps(int p_steps);
	int get_encounter_steps() const { return encounter_steps; }
	void set_disable_dash(bool p_disable);
	bool is_disable_dash() const { return disable_dash; }
	void set_next_event_id(int p_id);
	int get_next_event_id() const { return next_event_id; }
	void set_events(const Array &p_events);
	Array get_events() const { return events; }
	// The IDs of the map's events, which saved event states are checked against (JRPGGameState::prune_event_states).
	// None until event data exists (task 9.1 defines JRPGEventData and reads its IDs here).
	PackedInt32Array get_event_ids() const;

	JRPGMapData();
};

VARIANT_ENUM_CAST(JRPGMapData::ScrollType);
VARIANT_ENUM_CAST(JRPGMapData::Layer);
