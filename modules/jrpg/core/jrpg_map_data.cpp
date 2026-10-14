/**************************************************************************/
/*  jrpg_map_data.cpp                                                     */
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

#include "jrpg_map_data.h"

#include "jrpg_schema.h"

#include "core/object/class_db.h"

template <typename T>
T JRPGMapData::_fitted(const T &p_array, const char *p_what) const {
	const int count = _cell_count();
	if (p_array.size() == count) {
		return p_array;
	}
	if (!p_array.is_empty()) {
		WARN_PRINT(vformat("JRPGMapData \"%s\": %s holds %d cells for a %d x %d map; it is fitted to %d.", get_path(), p_what, p_array.size(), size.x, size.y, count));
	}
	T fitted = p_array;
	const int old_size = fitted.size();
	fitted.resize(count);
	for (int i = old_size; i < count; i++) {
		fitted.set(i, 0);
	}
	return fitted;
}

void JRPGMapData::_fit_arrays() {
	static const char *names[LAYER_COUNT] = { "layer 0", "layer 1", "layer 2", "layer 3" };
	for (int i = 0; i < LAYER_COUNT; i++) {
		layers[i] = _fitted(layers[i], names[i]);
	}
	shadows = _fitted(shadows, "shadows");
	regions = _fitted(regions, "regions");
}

void JRPGMapData::_stamp_version() {
	if (schema_version == SCHEMA_UNSTAMPED) {
		schema_version = CURRENT_SCHEMA_VERSION;
	}
}

void JRPGMapData::set_map_id(int p_id) {
	ERR_FAIL_COND(p_id < 0);
	map_id = p_id;
	emit_changed();
}

void JRPGMapData::set_revision(int p_revision) {
	revision = MAX(0, p_revision);
	emit_changed();
}

void JRPGMapData::bump_revision() {
	set_revision(revision + 1);
}

void JRPGMapData::set_display_name(const String &p_name) {
	display_name = p_name;
	emit_changed();
}

void JRPGMapData::set_note(const String &p_note) {
	note = p_note;
	emit_changed();
}

void JRPGMapData::set_tileset_id(int p_id) {
	tileset_id = MAX(0, p_id);
	emit_signal(SNAME("layout_changed"));
	emit_changed();
}

void JRPGMapData::set_size(const Vector2i &p_size) {
	const Vector2i new_size = p_size.clamp(Vector2i(1, 1), Vector2i(MAX_SIDE, MAX_SIDE));
	ERR_FAIL_COND_MSG(new_size != p_size, vformat("A map is 1 to %d cells on each axis.", MAX_SIDE));
	if (new_size == size && layers[0].size() == _cell_count()) {
		return;
	}
	// Keep what both sizes share, from the top left (row by row data: copy row by row).
	const Vector2i old_size = size;
	const auto remap = [&](auto &r_array) {
		auto copy = r_array;
		r_array.resize(new_size.x * new_size.y);
		for (int i = 0; i < r_array.size(); i++) {
			r_array.set(i, 0);
		}
		if (copy.size() == old_size.x * old_size.y) {
			for (int y = 0; y < MIN(old_size.y, new_size.y); y++) {
				for (int x = 0; x < MIN(old_size.x, new_size.x); x++) {
					r_array.set(y * new_size.x + x, copy[y * old_size.x + x]);
				}
			}
		}
	};
	for (int i = 0; i < LAYER_COUNT; i++) {
		remap(layers[i]);
	}
	remap(shadows);
	remap(regions);
	size = new_size;
	_stamp_version();
	emit_signal(SNAME("layout_changed"));
	emit_changed();
}

void JRPGMapData::set_layers(const Array &p_layers) {
	if (p_layers.size() != LAYER_COUNT) {
		WARN_PRINT(vformat("JRPGMapData \"%s\": %d tile layers given, %d expected; missing ones are empty.", get_path(), p_layers.size(), LAYER_COUNT));
	}
	for (int i = 0; i < LAYER_COUNT; i++) {
		layers[i] = i < p_layers.size() ? PackedInt32Array(p_layers[i]) : PackedInt32Array();
	}
	_fit_arrays();
	_stamp_version();
	emit_signal(SNAME("layout_changed"));
	emit_changed();
}

Array JRPGMapData::get_layers() const {
	Array result;
	for (int i = 0; i < LAYER_COUNT; i++) {
		result.push_back(layers[i]);
	}
	return result;
}

void JRPGMapData::set_shadows(const PackedByteArray &p_shadows) {
	shadows = _fitted(p_shadows, "shadows");
	emit_signal(SNAME("layout_changed"));
	emit_changed();
}

void JRPGMapData::set_regions(const PackedByteArray &p_regions) {
	regions = _fitted(p_regions, "regions");
	emit_signal(SNAME("layout_changed"));
	emit_changed();
}

int JRPGMapData::get_cell(int p_layer, const Vector2i &p_cell) const {
	ERR_FAIL_INDEX_V(p_layer, LAYER_COUNT, 0);
	return has_cell(p_cell) ? layers[p_layer][get_cell_index(p_cell)] : 0;
}

PackedInt32Array JRPGMapData::get_layer(int p_layer) const {
	ERR_FAIL_INDEX_V(p_layer, LAYER_COUNT, PackedInt32Array());
	return layers[p_layer];
}

int JRPGMapData::get_region(const Vector2i &p_cell) const {
	return has_cell(p_cell) ? regions[get_cell_index(p_cell)] : 0;
}

int JRPGMapData::get_shadow(const Vector2i &p_cell) const {
	return has_cell(p_cell) ? shadows[get_cell_index(p_cell)] : 0;
}

void JRPGMapData::paint(int p_layer, const PackedInt32Array &p_cells, const PackedInt32Array &p_values) {
	ERR_FAIL_INDEX(p_layer, LAYER_COUNT);
	ERR_FAIL_COND_MSG(p_cells.size() != p_values.size(), "paint: one value per cell.");
	const int count = _cell_count();
	int32_t *data = layers[p_layer].ptrw();
	for (int i = 0; i < p_cells.size(); i++) {
		ERR_CONTINUE_MSG(p_cells[i] < 0 || p_cells[i] >= count, vformat("paint: cell %d is outside the %d x %d map.", p_cells[i], size.x, size.y));
		ERR_CONTINUE_MSG(p_values[i] < 0, "paint: palette indexes are 0 or more.");
		data[p_cells[i]] = p_values[i];
	}
	emit_signal(SNAME("cells_changed"), p_layer, p_cells);
	emit_changed();
}

void JRPGMapData::paint_regions(const PackedInt32Array &p_cells, const PackedInt32Array &p_values) {
	ERR_FAIL_COND_MSG(p_cells.size() != p_values.size(), "paint_regions: one value per cell.");
	const int count = _cell_count();
	uint8_t *data = regions.ptrw();
	for (int i = 0; i < p_cells.size(); i++) {
		ERR_CONTINUE(p_cells[i] < 0 || p_cells[i] >= count);
		data[p_cells[i]] = uint8_t(CLAMP(p_values[i], 0, 255));
	}
	emit_signal(SNAME("cells_changed"), -1, p_cells); // Layer -1: regions or shadows.
	emit_changed();
}

void JRPGMapData::paint_shadows(const PackedInt32Array &p_cells, const PackedInt32Array &p_values) {
	ERR_FAIL_COND_MSG(p_cells.size() != p_values.size(), "paint_shadows: one value per cell.");
	const int count = _cell_count();
	uint8_t *data = shadows.ptrw();
	for (int i = 0; i < p_cells.size(); i++) {
		ERR_CONTINUE(p_cells[i] < 0 || p_cells[i] >= count);
		data[p_cells[i]] = uint8_t(p_values[i] & 0xF);
	}
	emit_signal(SNAME("cells_changed"), -1, p_cells);
	emit_changed();
}

void JRPGMapData::set_scroll_type(ScrollType p_type) {
	ERR_FAIL_INDEX(int(p_type), SCROLL_LOOP_BOTH + 1);
	scroll_type = p_type;
	emit_signal(SNAME("layout_changed"));
	emit_changed();
}

BitField<JRPGAutotileResolver::Flags> JRPGMapData::get_resolver_flags(JRPGTilesetData::EdgeMode p_edge_mode) const {
	return JRPGAutotileResolver::get_flags(scroll_type & 2, scroll_type & 1, p_edge_mode);
}

void JRPGMapData::set_bgm(const Ref<JRPGAudioCue> &p_cue) {
	bgm = p_cue;
	emit_changed();
}

void JRPGMapData::set_autoplay_bgm(bool p_enabled) {
	autoplay_bgm = p_enabled;
	emit_changed();
}

void JRPGMapData::set_bgs(const Ref<JRPGAudioCue> &p_cue) {
	bgs = p_cue;
	emit_changed();
}

void JRPGMapData::set_autoplay_bgs(bool p_enabled) {
	autoplay_bgs = p_enabled;
	emit_changed();
}

void JRPGMapData::set_battleback(const Dictionary &p_battleback) {
	battleback = p_battleback;
	emit_changed();
}

void JRPGMapData::set_parallax(const Dictionary &p_parallax) {
	parallax = p_parallax;
	emit_changed();
}

void JRPGMapData::set_encounters(const Array &p_encounters) {
	encounters = p_encounters;
	emit_changed();
}

void JRPGMapData::set_encounter_steps(int p_steps) {
	encounter_steps = CLAMP(p_steps, 1, 999);
	emit_changed();
}

void JRPGMapData::set_disable_dash(bool p_disable) {
	disable_dash = p_disable;
	emit_changed();
}

void JRPGMapData::set_next_event_id(int p_id) {
	next_event_id = MAX(1, p_id);
	emit_changed();
}

void JRPGMapData::set_events(const Array &p_events) {
	events = p_events;
	emit_changed();
}

PackedInt32Array JRPGMapData::get_event_ids() const {
	return PackedInt32Array();
}

void JRPGMapData::_validate_property(PropertyInfo &p_property) const {
	JRPGSchema::hide_engine_property(p_property);
}

bool JRPGMapData::_set(const StringName &p_name, const Variant &p_value) {
	return JRPGSchema::set_renamed(this, p_name, p_value);
}

void JRPGMapData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_schema_version", "version"), &JRPGMapData::set_schema_version);
	ClassDB::bind_method(D_METHOD("get_schema_version"), &JRPGMapData::get_schema_version);
	ClassDB::bind_method(D_METHOD("set_map_id", "id"), &JRPGMapData::set_map_id);
	ClassDB::bind_method(D_METHOD("get_map_id"), &JRPGMapData::get_map_id);
	ClassDB::bind_method(D_METHOD("set_revision", "revision"), &JRPGMapData::set_revision);
	ClassDB::bind_method(D_METHOD("get_revision"), &JRPGMapData::get_revision);
	ClassDB::bind_method(D_METHOD("bump_revision"), &JRPGMapData::bump_revision);
	ClassDB::bind_method(D_METHOD("set_display_name", "name"), &JRPGMapData::set_display_name);
	ClassDB::bind_method(D_METHOD("get_display_name"), &JRPGMapData::get_display_name);
	ClassDB::bind_method(D_METHOD("set_note", "note"), &JRPGMapData::set_note);
	ClassDB::bind_method(D_METHOD("get_note"), &JRPGMapData::get_note);
	ClassDB::bind_method(D_METHOD("set_tileset_id", "id"), &JRPGMapData::set_tileset_id);
	ClassDB::bind_method(D_METHOD("get_tileset_id"), &JRPGMapData::get_tileset_id);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &JRPGMapData::set_size);
	ClassDB::bind_method(D_METHOD("get_size"), &JRPGMapData::get_size);
	ClassDB::bind_method(D_METHOD("set_layers", "layers"), &JRPGMapData::set_layers);
	ClassDB::bind_method(D_METHOD("get_layers"), &JRPGMapData::get_layers);
	ClassDB::bind_method(D_METHOD("set_shadows", "shadows"), &JRPGMapData::set_shadows);
	ClassDB::bind_method(D_METHOD("get_shadows"), &JRPGMapData::get_shadows);
	ClassDB::bind_method(D_METHOD("set_regions", "regions"), &JRPGMapData::set_regions);
	ClassDB::bind_method(D_METHOD("get_regions"), &JRPGMapData::get_regions);
	ClassDB::bind_method(D_METHOD("has_cell", "cell"), &JRPGMapData::has_cell);
	ClassDB::bind_method(D_METHOD("get_cell_index", "cell"), &JRPGMapData::get_cell_index);
	ClassDB::bind_method(D_METHOD("get_cell", "layer", "cell"), &JRPGMapData::get_cell);
	ClassDB::bind_method(D_METHOD("get_layer", "layer"), &JRPGMapData::get_layer);
	ClassDB::bind_method(D_METHOD("get_region", "cell"), &JRPGMapData::get_region);
	ClassDB::bind_method(D_METHOD("get_shadow", "cell"), &JRPGMapData::get_shadow);
	ClassDB::bind_method(D_METHOD("paint", "layer", "cells", "values"), &JRPGMapData::paint);
	ClassDB::bind_method(D_METHOD("paint_regions", "cells", "values"), &JRPGMapData::paint_regions);
	ClassDB::bind_method(D_METHOD("paint_shadows", "cells", "values"), &JRPGMapData::paint_shadows);
	ClassDB::bind_method(D_METHOD("set_scroll_type", "type"), &JRPGMapData::set_scroll_type);
	ClassDB::bind_method(D_METHOD("get_scroll_type"), &JRPGMapData::get_scroll_type);
	ClassDB::bind_method(D_METHOD("get_resolver_flags", "edge_mode"), &JRPGMapData::get_resolver_flags);
	ClassDB::bind_method(D_METHOD("set_bgm", "cue"), &JRPGMapData::set_bgm);
	ClassDB::bind_method(D_METHOD("get_bgm"), &JRPGMapData::get_bgm);
	ClassDB::bind_method(D_METHOD("set_autoplay_bgm", "enabled"), &JRPGMapData::set_autoplay_bgm);
	ClassDB::bind_method(D_METHOD("is_autoplay_bgm"), &JRPGMapData::is_autoplay_bgm);
	ClassDB::bind_method(D_METHOD("set_bgs", "cue"), &JRPGMapData::set_bgs);
	ClassDB::bind_method(D_METHOD("get_bgs"), &JRPGMapData::get_bgs);
	ClassDB::bind_method(D_METHOD("set_autoplay_bgs", "enabled"), &JRPGMapData::set_autoplay_bgs);
	ClassDB::bind_method(D_METHOD("is_autoplay_bgs"), &JRPGMapData::is_autoplay_bgs);
	ClassDB::bind_method(D_METHOD("set_battleback", "battleback"), &JRPGMapData::set_battleback);
	ClassDB::bind_method(D_METHOD("get_battleback"), &JRPGMapData::get_battleback);
	ClassDB::bind_method(D_METHOD("set_parallax", "parallax"), &JRPGMapData::set_parallax);
	ClassDB::bind_method(D_METHOD("get_parallax"), &JRPGMapData::get_parallax);
	ClassDB::bind_method(D_METHOD("set_encounters", "encounters"), &JRPGMapData::set_encounters);
	ClassDB::bind_method(D_METHOD("get_encounters"), &JRPGMapData::get_encounters);
	ClassDB::bind_method(D_METHOD("set_encounter_steps", "steps"), &JRPGMapData::set_encounter_steps);
	ClassDB::bind_method(D_METHOD("get_encounter_steps"), &JRPGMapData::get_encounter_steps);
	ClassDB::bind_method(D_METHOD("set_disable_dash", "disable"), &JRPGMapData::set_disable_dash);
	ClassDB::bind_method(D_METHOD("is_disable_dash"), &JRPGMapData::is_disable_dash);
	ClassDB::bind_method(D_METHOD("set_next_event_id", "id"), &JRPGMapData::set_next_event_id);
	ClassDB::bind_method(D_METHOD("get_next_event_id"), &JRPGMapData::get_next_event_id);
	ClassDB::bind_method(D_METHOD("set_events", "events"), &JRPGMapData::set_events);
	ClassDB::bind_method(D_METHOD("get_events"), &JRPGMapData::get_events);

	// Property order is load order: the size comes before the arrays it shapes.
	ADD_PROPERTY(PropertyInfo(Variant::INT, "schema_version", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_schema_version", "get_schema_version");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "map_id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY), "set_map_id", "get_map_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "revision", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_revision", "get_revision");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "note", PROPERTY_HINT_MULTILINE_TEXT), "set_note", "get_note");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tileset_id", PROPERTY_HINT_RANGE, "1,9999,1"), "set_tileset_id", "get_tileset_id");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "size"), "set_size", "get_size");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "layers", PROPERTY_HINT_ARRAY_TYPE, "PackedInt32Array", PROPERTY_USAGE_NO_EDITOR), "set_layers", "get_layers");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "shadows", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_shadows", "get_shadows");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "regions", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_regions", "get_regions");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "scroll_type", PROPERTY_HINT_ENUM, "None,Loop Vertically,Loop Horizontally,Loop Both"), "set_scroll_type", "get_scroll_type");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "bgm", PROPERTY_HINT_RESOURCE_TYPE, "JRPGAudioCue"), "set_bgm", "get_bgm");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay_bgm"), "set_autoplay_bgm", "is_autoplay_bgm");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "bgs", PROPERTY_HINT_RESOURCE_TYPE, "JRPGAudioCue"), "set_bgs", "get_bgs");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay_bgs"), "set_autoplay_bgs", "is_autoplay_bgs");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "battleback"), "set_battleback", "get_battleback");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "parallax"), "set_parallax", "get_parallax");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "encounters", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_encounters", "get_encounters");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "encounter_steps", PROPERTY_HINT_RANGE, "1,999,1"), "set_encounter_steps", "get_encounter_steps");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "disable_dash"), "set_disable_dash", "is_disable_dash");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "next_event_id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_next_event_id", "get_next_event_id");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "events", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_events", "get_events");

	// A cell edit (layer -1: regions or shadows), for views that update only the cells around it.
	ADD_SIGNAL(MethodInfo("cells_changed", PropertyInfo(Variant::INT, "layer"), PropertyInfo(Variant::PACKED_INT32_ARRAY, "cells")));
	// The size, tileset, scroll type or whole arrays changed: views build everything again.
	ADD_SIGNAL(MethodInfo("layout_changed"));

	BIND_ENUM_CONSTANT(SCROLL_NONE);
	BIND_ENUM_CONSTANT(SCROLL_LOOP_VERTICAL);
	BIND_ENUM_CONSTANT(SCROLL_LOOP_HORIZONTAL);
	BIND_ENUM_CONSTANT(SCROLL_LOOP_BOTH);
	BIND_ENUM_CONSTANT(LAYER_GROUND);
	BIND_ENUM_CONSTANT(LAYER_GROUND_DETAIL);
	BIND_ENUM_CONSTANT(LAYER_OBJECTS);
	BIND_ENUM_CONSTANT(LAYER_UPPER);
	BIND_ENUM_CONSTANT(LAYER_COUNT);
	BIND_CONSTANT(MAX_SIDE);
}

JRPGMapData::JRPGMapData() {
	_fit_arrays();
}
