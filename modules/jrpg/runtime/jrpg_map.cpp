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
	if (p_connect) {
		map_data->connect(SNAME("layout_changed"), layout);
		map_data->connect(SNAME("cells_changed"), cells);
	} else {
		map_data->disconnect(SNAME("layout_changed"), layout);
		map_data->disconnect(SNAME("cells_changed"), cells);
	}
}

void JRPGMap::_queue_rebuild() {
	if (!rebuild_queued && is_inside_tree()) {
		rebuild_queued = true;
		callable_mp(this, &JRPGMap::rebuild).call_deferred();
	}
}

Ref<JRPGTilesetData> JRPGMap::_find_tileset_data() const {
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
	if (map_data.is_null()) {
		return;
	}
	tileset_data = _find_tileset_data();
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
	palette = tileset_data->get_palette();
	kinds = JRPGAutotileResolver::get_palette_kinds(palette);
	overhead.resize(palette.size() + 1);
	for (int index = 0; index < overhead.size(); index++) {
		overhead.set(index, (tileset_data->get_tile_flags(index) & JRPGTilesetData::FLAG_OVERHEAD) ? 1 : 0);
	}
	flags = int(map_data->get_resolver_flags(tileset_data->get_edge_mode()));
	const Vector2i size = map_data->get_size();
	for (int layer = 0; layer < JRPGMapData::LAYER_COUNT; layer++) {
		shapes[layer] = JRPGAutotileResolver::resolve_layer(map_data->get_layer(layer), size, kinds, flags);
		for (int y = 0; y < size.y; y++) {
			for (int x = 0; x < size.x; x++) {
				_update_cell(layer, Vector2i(x, y));
			}
		}
	}
}

void JRPGMap::_update_cell(int p_layer, const Vector2i &p_cell) {
	const int at = map_data->get_cell_index(p_cell);
	const int index = map_data->get_layer(p_layer)[at];
	TileMapLayer *target = (index > 0 && index < overhead.size() && overhead[index]) ? above[p_layer] : below[p_layer];
	TileMapLayer *other = target == below[p_layer] ? above[p_layer] : below[p_layer];
	other->erase_cell(p_cell);
	if (index <= 0 || index > palette.size()) {
		target->erase_cell(p_cell); // Empty, or an index the palette does not have.
		return;
	}
	const Dictionary entry = palette[index - 1];
	Vector2i coords = entry.get("atlas_coords", Vector2i());
	const int shape = shapes[p_layer].size() > at ? shapes[p_layer][at] : 0;
	if (kinds[index] == JRPGAutotileResolver::KIND_BLOB) {
		coords += JRPGAutotile::get_blob_cell(shape);
	} else if (kinds[index] == JRPGAutotileResolver::KIND_WALL) {
		coords += JRPGAutotile::get_wall_cell(shape);
	}
	target->set_cell(p_cell, entry.get("source_id", TileSet::INVALID_SOURCE), coords, entry.get("alternative", 0));
}

void JRPGMap::_on_cells_changed(int p_layer, const PackedInt32Array &p_cells) {
	if (p_layer < 0 || p_layer >= JRPGMapData::LAYER_COUNT || tileset.is_null() || rebuild_queued) {
		return; // Regions and shadows are not drawn here; a full build is coming anyway.
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
				_update_cell(p_layer, changed);
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

void JRPGMap::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		_queue_rebuild();
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

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "map_data", PROPERTY_HINT_RESOURCE_TYPE, "JRPGMapData"), "set_map_data", "get_map_data");

	BIND_CONSTANT(Z_BELOW);
	BIND_CONSTANT(Z_CHARACTERS);
	BIND_CONSTANT(Z_ABOVE);
	BIND_CONSTANT(DEFAULT_CELL_SIZE);
}

JRPGMap::JRPGMap() {
	static const char *names[JRPGMapData::LAYER_COUNT] = { "Ground", "GroundDetail", "Objects", "Upper" };
	for (int i = 0; i < JRPGMapData::LAYER_COUNT; i++) {
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
