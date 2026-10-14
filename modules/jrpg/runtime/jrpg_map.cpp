/**************************************************************************/
/*  jrpg_map.cpp                                                          */
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

#include "jrpg_map.h"

#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/templates/hash_set.h"
#include "scene/2d/tile_map_layer.h"
#include "scene/resources/2d/tile_set.h"

#include "modules/jrpg/core/jrpg_autotile.h"
#include "modules/jrpg/core/jrpg_autotile_resolver.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_table.h"

void JRPGMap::set_map_data(const Ref<JRPGMapData> &p_data) {
	if (map_data == p_data) {
		return;
	}
	_connect_data(false);
	map_data = p_data;
	if (map_data.is_valid()) {
		map_data->upgrade(); // D-029: older data is migrated here, newer data warned about.
	}
	_connect_data(true);
	_queue_rebuild();
}

void JRPGMap::set_database(const Ref<JRPGDatabase> &p_database) {
	database = p_database;
	_queue_rebuild();
}

Ref<JRPGDatabase> JRPGMap::get_database() const {
	if (database.is_valid()) {
		return database;
	}
	// The project's, from the resource cache: the editor's database tools edit the same object.
	return ResourceLoader::exists(JRPGDatabase::PROJECT_PATH) ? JRPGDatabase::load_database(JRPGDatabase::PROJECT_PATH) : Ref<JRPGDatabase>();
}

void JRPGMap::_connect_data(bool p_connect) {
	if (map_data.is_null()) {
		return;
	}
	const Callable layout = callable_mp(this, &JRPGMap::_queue_rebuild);
	const Callable cells = callable_mp(this, &JRPGMap::_on_cells_changed);
	const Callable edited_layout = callable_mp(this, &JRPGMap::_mark_edited);
	const Callable edited_cells = callable_mp(this, &JRPGMap::_mark_edited).unbind(2);
	if (p_connect) {
		map_data->connect(SNAME("layout_changed"), layout);
		map_data->connect(SNAME("cells_changed"), cells);
		map_data->connect(SNAME("layout_changed"), edited_layout);
		map_data->connect(SNAME("cells_changed"), edited_cells);
	} else {
		map_data->disconnect(SNAME("layout_changed"), layout);
		map_data->disconnect(SNAME("cells_changed"), cells);
		map_data->disconnect(SNAME("layout_changed"), edited_layout);
		map_data->disconnect(SNAME("cells_changed"), edited_cells);
	}
}

void JRPGMap::_queue_rebuild() {
	if (!rebuild_queued && is_inside_tree()) {
		rebuild_queued = true;
		callable_mp(this, &JRPGMap::_rebuild_if_queued).call_deferred();
	}
}

void JRPGMap::_rebuild_if_queued() {
	if (rebuild_queued) {
		rebuild(); // Not when a rebuild ran since it was asked for (the scene manager builds at once).
	}
}

Ref<JRPGTilesetData> JRPGMap::find_tileset_data() const {
	if (map_data.is_null()) {
		return Ref<JRPGTilesetData>();
	}
	const Ref<JRPGDatabase> db = get_database();
	const Ref<JRPGTable> tilesets = db.is_valid() ? db->get_table(JRPGDatabase::TABLE_TILESETS) : Ref<JRPGTable>();
	const int id = map_data->get_tileset_id();
	if (tilesets.is_null() || id < 1 || id > tilesets->get_maximum()) {
		return Ref<JRPGTilesetData>();
	}
	return tilesets->get_entry(id);
}

void JRPGMap::rebuild() {
	rebuild_queued = false;
	rebuild_count++;
	shadows->queue_redraw(); // A new size or cell size moves them.
	for (int i = 0; i < JRPGMapData::LAYER_COUNT; i++) {
		below[i]->clear();
		above[i]->clear();
		shapes[i].clear();
	}
	if (tileset_data.is_valid() && tileset_data->is_connected(CoreStringName(changed), callable_mp(this, &JRPGMap::_queue_rebuild))) {
		tileset_data->disconnect(CoreStringName(changed), callable_mp(this, &JRPGMap::_queue_rebuild));
	}
	tileset_data.unref();
	tileset.unref();
	palette_tiles.clear();
	if (map_data.is_null()) {
		return;
	}
	tileset_data = find_tileset_data();
	if (tileset_data.is_valid()) {
		// A new palette, flags baked again or another TileSet: drawn again.
		tileset_data->connect(CoreStringName(changed), callable_mp(this, &JRPGMap::_queue_rebuild));
		const String uid = tileset_data->get_tileset_uid();
		tileset = !uid.is_empty() && ResourceLoader::exists(uid) ? ResourceLoader::load(uid) : Ref<Resource>();
	}
	for (int i = 0; i < JRPGMapData::LAYER_COUNT; i++) {
		below[i]->set_tile_set(tileset);
		above[i]->set_tile_set(tileset);
	}
	if (tileset.is_null()) {
		return; // Nothing to draw with (a map can be painted once its tileset has a TileSet).
	}
	const Array palette = tileset_data->get_palette();
	kinds = JRPGAutotileResolver::get_palette_kinds(palette);
	overhead.resize(palette.size() + 1);
	palette_tiles.resize(palette.size() + 1);
	for (int index = 0; index < overhead.size(); index++) {
		overhead.set(index, (tileset_data->get_tile_flags(index) & JRPGTilesetData::FLAG_OVERHEAD) ? 1 : 0);
		if (index > 0) {
			const Dictionary entry = palette[index - 1];
			palette_tiles[index].source_id = entry.get("source_id", TileSet::INVALID_SOURCE);
			palette_tiles[index].coords = entry.get("atlas_coords", Vector2i());
			palette_tiles[index].alternative = entry.get("alternative", 0);
		}
	}
	flags = int(map_data->get_resolver_flags(tileset_data->get_edge_mode()));
	const Vector2i size = map_data->get_size();
	for (int layer = 0; layer < JRPGMapData::LAYER_COUNT; layer++) {
		const PackedInt32Array cells = map_data->get_layer(layer); // Always size.x * size.y long.
		shapes[layer] = JRPGAutotileResolver::resolve_layer(cells, size, kinds, flags);
		for (int y = 0; y < size.y; y++) {
			for (int x = 0; x < size.x; x++) {
				_update_cell(layer, Vector2i(x, y), cells[y * size.x + x]);
			}
		}
	}
}

void JRPGMap::_update_cell(int p_layer, const Vector2i &p_cell, int p_index) {
	TileMapLayer *target = (p_index > 0 && p_index < overhead.size() && overhead[p_index]) ? above[p_layer] : below[p_layer];
	TileMapLayer *other = target == below[p_layer] ? above[p_layer] : below[p_layer];
	other->erase_cell(p_cell);
	if (p_index <= 0 || p_index >= int(palette_tiles.size())) {
		target->erase_cell(p_cell); // Empty, or an index the palette does not have.
		return;
	}
	const PaletteTile &tile = palette_tiles[p_index];
	Vector2i coords = tile.coords;
	const int at = map_data->get_cell_index(p_cell);
	const int shape = shapes[p_layer].size() > at ? shapes[p_layer][at] : 0;
	if (kinds[p_index] == JRPGAutotileResolver::KIND_BLOB) {
		coords += JRPGAutotile::get_blob_cell(shape);
	} else if (kinds[p_index] == JRPGAutotileResolver::KIND_WALL) {
		coords += JRPGAutotile::get_wall_cell(shape);
	}
	target->set_cell(p_cell, tile.source_id, coords, tile.alternative);
}

int JRPGMap::get_palette_kind(int p_index) const {
	return p_index > 0 && p_index < kinds.size() ? int(kinds[p_index]) : int(JRPGAutotileResolver::KIND_PLAIN);
}

void JRPGMap::_draw_shadows() {
	// Each shadowed quarter darkened: bit 1 top left, 2 top right, 4 bottom left, 8 bottom right (spec 05).
	if (map_data.is_null()) {
		return;
	}
	const Vector2i size = map_data->get_size();
	const Vector2 quarter = Vector2(get_cell_size()) / 2;
	const PackedByteArray bits = map_data->get_shadows();
	const Color shade(0, 0, 0, SHADOW_ALPHA);
	for (int y = 0; y < size.y; y++) {
		for (int x = 0; x < size.x; x++) {
			const int mask = bits[y * size.x + x];
			for (int q = 0; mask && q < 4; q++) {
				if (mask & (1 << q)) {
					shadows->draw_rect(Rect2((Vector2(x * 2 + (q & 1), y * 2 + (q >> 1))) * quarter, quarter), shade);
				}
			}
		}
	}
}

void JRPGMap::_on_cells_changed(int p_layer, const PackedInt32Array &p_cells) {
	if (p_layer == JRPGMapData::CELLS_SHADOWS) {
		shadows->queue_redraw();
	}
	if (p_layer < 0) {
		return; // Regions are not drawn by the map.
	}
	if (p_layer >= JRPGMapData::LAYER_COUNT || tileset.is_null() || rebuild_queued) {
		return; // A full build is coming anyway.
	}
	const Vector2i size = map_data->get_size();
	if (shapes[p_layer].size() != size.x * size.y) {
		_queue_rebuild();
		return;
	}
	const PackedInt32Array cells = map_data->get_layer(p_layer);
	HashSet<Vector2i> done;
	for (const int at : p_cells) {
		const Vector2i cell(at % size.x, at / size.x);
		// The painted cell and the neighbors whose shape changed (wrapping on looping axes).
		for (const Vector2i &changed : JRPGAutotileResolver::update_around(cells, size, kinds, cell, flags, shapes[p_layer])) {
			if (!done.has(changed)) {
				done.insert(changed);
				_update_cell(p_layer, changed, cells[map_data->get_cell_index(changed)]);
			}
		}
	}
}

TileMapLayer *JRPGMap::get_tile_layer(int p_layer, bool p_above) const {
	ERR_FAIL_INDEX_V(p_layer, JRPGMapData::LAYER_COUNT, nullptr);
	return p_above ? above[p_layer] : below[p_layer];
}

Vector2i JRPGMap::get_cell_size() const {
	const Ref<TileSet> tiles = below[0] ? below[0]->get_tile_set() : Ref<TileSet>();
	return tiles.is_valid() ? tiles->get_tile_size() : Vector2i(DEFAULT_CELL_SIZE, DEFAULT_CELL_SIZE);
}

void JRPGMap::_mark_edited() {
	edited = true;
}

void JRPGMap::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		_queue_rebuild();
	} else if (p_what == NOTIFICATION_EDITOR_PRE_SAVE && edited && map_data.is_valid()) {
		// A save that changes the map's cells raises its revision, so a game saved before notices (spec 06; epic 8
		// review A3). Sent by the editor before it saves the scene (editor/editor_node.cpp:2481).
		map_data->bump_revision();
		edited = false;
	}
}

void JRPGMap::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_map_data", "data"), &JRPGMap::set_map_data);
	ClassDB::bind_method(D_METHOD("get_map_data"), &JRPGMap::get_map_data);
	ClassDB::bind_method(D_METHOD("set_database", "database"), &JRPGMap::set_database);
	ClassDB::bind_method(D_METHOD("get_database"), &JRPGMap::get_database);
	ClassDB::bind_method(D_METHOD("rebuild"), &JRPGMap::rebuild);
	ClassDB::bind_method(D_METHOD("get_rebuild_count"), &JRPGMap::get_rebuild_count);
	ClassDB::bind_method(D_METHOD("get_tile_layer", "layer", "above"), &JRPGMap::get_tile_layer);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &JRPGMap::get_cell_size);
	ClassDB::bind_method(D_METHOD("get_palette_kind", "index"), &JRPGMap::get_palette_kind);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "map_data", PROPERTY_HINT_RESOURCE_TYPE, "JRPGMapData"), "set_map_data", "get_map_data");

	BIND_CONSTANT(Z_BELOW);
	BIND_CONSTANT(Z_CHARACTERS);
	BIND_CONSTANT(Z_ABOVE);
	BIND_CONSTANT(DEFAULT_CELL_SIZE);
}

JRPGMap::JRPGMap() {
	static const char *names[JRPGMapData::LAYER_COUNT] = { "Ground", "GroundDetail", "Objects", "Upper" };
	for (int i = 0; i < JRPGMapData::LAYER_COUNT; i++) {
		if (i == JRPGMapData::LAYER_OBJECTS) {
			// Children of one z index draw in order: the shadows go over the ground layers, under the objects.
			shadows = memnew(Node2D);
			shadows->set_name("Shadows");
			shadows->set_z_index(Z_BELOW);
			shadows->connect(SceneStringName(draw), callable_mp(this, &JRPGMap::_draw_shadows));
			add_child(shadows, false, INTERNAL_MODE_FRONT);
		}
		below[i] = memnew(TileMapLayer);
		below[i]->set_name(names[i]);
		below[i]->set_z_index(Z_BELOW);
		add_child(below[i], false, INTERNAL_MODE_FRONT);
	}
	for (int i = 0; i < JRPGMapData::LAYER_COUNT; i++) {
		above[i] = memnew(TileMapLayer);
		above[i]->set_name(String(names[i]) + "Above");
		above[i]->set_z_index(Z_ABOVE);
		add_child(above[i], false, INTERNAL_MODE_FRONT);
	}
}

JRPGMap::~JRPGMap() {
	// Connections to this map (map data, tileset data) are dropped by Object's destructor.
}
