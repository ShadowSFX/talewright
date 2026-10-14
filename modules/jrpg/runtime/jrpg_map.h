/**************************************************************************/
/*  jrpg_map.h                                                            */
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

#include "core/templates/local_vector.h"
#include "scene/2d/node_2d.h"

#include "modules/jrpg/core/jrpg_map_data.h"

class JRPGDatabase;
class TileMapLayer;
class TileSet;

// The root of a map scene (spec 05, D-006): holds the map's JRPGMapData as a built-in sub-resource, so map edits
// follow the scene's own undo history and save with it, and draws it. For each of the four data layers it builds two
// internal TileMapLayers (INTERNAL_MODE_FRONT, as TileMap builds its layers, scene/2d/tile_map.cpp:272): tiles drawn
// below characters (z 0) and overhead tiles (z 2; characters use z 1). Autotile and wall cells show the shape
// JRPGAutotileResolver picks; a paint re-resolves only the cells around it. The map's quarter shadows draw between the
// ground layers and the objects (an internal node, task 8.4.3.1). Users may add any other node under it.
// Authority (D-007): the map only presents its data; what moves on it belongs to the server online.
class JRPGMap : public Node2D {
	GDCLASS(JRPGMap, Node2D);

public:
	static constexpr int Z_BELOW = 0;
	static constexpr int Z_CHARACTERS = 1;
	static constexpr int Z_ABOVE = 2;
	static constexpr int DEFAULT_CELL_SIZE = 48; // Pixels, until a TileSet says otherwise (D-034).

private:
	Ref<JRPGMapData> map_data;
	Ref<JRPGDatabase> database; // Null: the project's (JRPGDatabase::PROJECT_PATH).
	TileMapLayer *below[JRPGMapData::LAYER_COUNT] = {};
	TileMapLayer *above[JRPGMapData::LAYER_COUNT] = {};
	Node2D *shadows = nullptr; // Quarter shadows, over the ground layers and under the objects.

	static constexpr float SHADOW_ALPHA = 0.5;

	// What the last build used.
	Ref<JRPGTilesetData> tileset_data;
	Ref<TileSet> tileset;
	// The tile of each palette index (index 0: none), read once per build instead of once per cell (epic 8 review A13).
	struct PaletteTile {
		int source_id = -1; // TileSet::INVALID_SOURCE.
		Vector2i coords;
		int alternative = 0;
	};
	LocalVector<PaletteTile> palette_tiles;
	PackedByteArray kinds;
	PackedByteArray overhead; // 1 per palette index whose baked flags say overhead.
	int flags = 0;
	PackedByteArray shapes[JRPGMapData::LAYER_COUNT];
	bool rebuild_queued = false;
	int rebuild_count = 0;
	bool edited = false; // Cells or layout changed since the last save (the editor's pre-save raises the revision).

	void _connect_data(bool p_connect);
	void _queue_rebuild();
	void _rebuild_if_queued();
	void _on_cells_changed(int p_layer, const PackedInt32Array &p_cells);
	void _mark_edited();
	// Draws p_cell of p_layer, which holds palette index p_index.
	void _update_cell(int p_layer, const Vector2i &p_cell, int p_index);
	void _draw_shadows();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_map_data(const Ref<JRPGMapData> &p_data);
	Ref<JRPGMapData> get_map_data() const { return map_data; }
	// The database the tileset comes from (tests, tools); without one, the project's.
	void set_database(const Ref<JRPGDatabase> &p_database);
	Ref<JRPGDatabase> get_database() const;

	// Builds every drawn cell again (the tileset, its palette or its flags changed).
	void rebuild();
	// How many full builds ran (diagnostics: a paint must not cause one).
	int get_rebuild_count() const { return rebuild_count; }
	// The internal layer that draws p_layer's tiles below (or above) characters.
	TileMapLayer *get_tile_layer(int p_layer, bool p_above) const;
	// The size of one cell in the map's local coordinates: its TileSet's tile size (DEFAULT_CELL_SIZE without one).
	Vector2i get_cell_size() const;
	// The database tileset the map's data names (tileset_id), or null; read now, not from the last build.
	Ref<JRPGTilesetData> find_tileset_data() const;
	// The JRPGAutotileResolver::Kind of palette index p_index in the tileset the map draws with (KIND_PLAIN for plain
	// tiles, 0 and indexes the palette does not have, and before the first build).
	int get_palette_kind(int p_index) const;

	JRPGMap();
	~JRPGMap();
};
