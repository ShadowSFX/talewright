/**************************************************************************/
/*  jrpg_tile_flags.h                                                     */
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

#include "core/object/ref_counted.h"
#include "core/templates/vector.h"

class JRPGTilesetData;
class TileData;
class TileSet;

// Tile flags (spec 05, "Tile flags"): authored as TileSet custom data layers, so Godot's own TileSet editor can paint
// them too (editor/scene/2d/tiles/tile_set_atlas_source_editor.cpp:825-857), and baked into
// JRPGTilesetData.flags, one word per palette index, so core code and servers never read a TileSet. The bake keeps a
// hash of what it was made from; a bake whose hash no longer matches is stale and the database save rebuilds it.
class JRPGTileFlags : public RefCounted {
	GDCLASS(JRPGTileFlags, RefCounted);

protected:
	static void _bind_methods();

public:
	static constexpr const char *LAYER_PASS = "jrpg_pass"; // int: 0 open, 1 blocked, 2 transparent.
	static constexpr const char *LAYER_NO_ENTRY = "jrpg_no_entry"; // int: sides N 1, E 2, S 4, W 8 not to enter by.
	static constexpr const char *LAYER_NO_EXIT = "jrpg_no_exit"; // int: sides not to leave by.
	static constexpr const char *LAYER_OVERHEAD = "jrpg_overhead"; // bool
	static constexpr const char *LAYER_LADDER = "jrpg_ladder"; // bool
	static constexpr const char *LAYER_BUSH = "jrpg_bush"; // bool
	static constexpr const char *LAYER_COUNTER = "jrpg_counter"; // bool
	static constexpr const char *LAYER_DAMAGE = "jrpg_damage"; // bool
	static constexpr const char *LAYER_TERRAIN_TAG = "jrpg_terrain_tag"; // int 0..15

	// The parts of a flag word, in the order the tileset form lists them.
	enum Field {
		FIELD_PASS,
		FIELD_NO_ENTRY,
		FIELD_NO_EXIT,
		FIELD_OVERHEAD,
		FIELD_LADDER,
		FIELD_BUSH,
		FIELD_COUNTER,
		FIELD_DAMAGE,
		FIELD_TERRAIN_TAG,
		FIELD_MAX,
	};
	struct FieldInfo {
		const char *key; // "pass", "no_entry", ...: the bridge tools' names.
		const char *label; // "Passage", "Entry Sides", ...: marked for translation (TTRC), shown through TTRGET.
		int mask; // The field's bits in the word.
		int shift; // Value = (word & mask) >> shift.
		int max; // Largest value: 2 for passage, 15 for side masks and the terrain tag, 1 for the others.
	};
	static const FieldInfo &get_field(Field p_field);
	// The field whose bits are exactly p_mask, or FIELD_MAX.
	static Field find_field(int p_mask);

	// Adds the jrpg_* custom data layers p_tileset lacks; returns how many were added.
	static int ensure_layers(const Ref<TileSet> &p_tileset);
	// The flag word of a tile from its custom data (JRPGTilesetData::pack_flags layout); 0 without the layers.
	static int read_tile(const Ref<TileSet> &p_tileset, const TileData *p_tile);
	// Writes a flag word into a tile's custom data (the layers must exist).
	static void write_tile(const Ref<TileSet> &p_tileset, TileData *p_tile, int p_flags);
	// The atlas coordinates of every tile a palette entry stands for: one for a plain tile, every shape of an
	// autotile or wall kind.
	static Vector<Vector2i> get_entry_tiles(const Dictionary &p_entry);
	// Whether the TileSet has the tile a palette entry names (its first tile, for a kind).
	static bool has_entry_tile(const Ref<TileSet> &p_tileset, const Dictionary &p_entry);
	// The flag word of a palette entry (its first tile); 0 when the tile is missing.
	static int get_entry_flags(const Ref<TileSet> &p_tileset, const Dictionary &p_entry);
	// Writes p_flags to every tile of a palette entry, adding the layers when needed.
	static Error set_entry_flags(const Ref<TileSet> &p_tileset, const Dictionary &p_entry, int p_flags);
	// One word per tile of get_entry_tiles (0 for a tile the TileSet does not have).
	static PackedInt32Array get_tile_words(const Ref<TileSet> &p_tileset, const Dictionary &p_entry);
	// Writes one word per tile of get_entry_tiles, adding the layers when needed; tiles the TileSet does not have are
	// skipped. Returns how many tiles were written.
	static int set_tile_words(const Ref<TileSet> &p_tileset, const Dictionary &p_entry, const PackedInt32Array &p_words);
	// The baked table of p_data's palette (index 0, the empty cell, is 0) and the hash of what it was made from.
	static PackedInt32Array bake(const Ref<TileSet> &p_tileset, const Ref<JRPGTilesetData> &p_data, String &r_hash);
	static bool is_stale(const Ref<TileSet> &p_tileset, const Ref<JRPGTilesetData> &p_data);
	// Bakes p_data again when it is stale; true when that changed it.
	static bool rebake(const Ref<TileSet> &p_tileset, const Ref<JRPGTilesetData> &p_data);
	// The TileSet a JRPG tileset uses (its tileset_uid), or null.
	static Ref<TileSet> load_tileset(const Ref<JRPGTilesetData> &p_data);
};
