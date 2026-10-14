/**************************************************************************/
/*  jrpg_autotile_resolver.h                                              */
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

#include "jrpg_misc_data.h"

#include "core/math/vector2i.h"
#include "core/object/ref_counted.h"
#include "core/templates/bit_field.h"
#include "core/templates/vector.h"
#include "core/variant/binder_common.h" // IWYU pragma: keep (VARIANT_ENUM_CAST, VARIANT_BITFIELD_CAST).

// The autotile resolver (spec 05, "Autotile resolver"; task 7.4): which shape every autotile cell of a map layer
// shows. A cell's neighbors count when they hold the same palette entry (the same kind) on the same layer; outside
// the map, a looping axis wraps around and otherwise the tileset's edge mode decides (extend: the kind runs on, border:
// it ends). Blob kinds use the 8-neighbor mask (JRPGAutotile::get_blob_index), wall kinds the 4-neighbor one. Shapes
// are never stored: they are resolved on load and around every edit, so the map data stays the only source of truth.
// Pure logic on palette indexes (no TileSet, no nodes), deterministic, shared by the editor, the game and the servers.
class JRPGAutotileResolver : public RefCounted {
	GDCLASS(JRPGAutotileResolver, RefCounted);

protected:
	static void _bind_methods();

public:
	// What a palette entry is, for the resolver.
	enum Kind {
		KIND_PLAIN, // A plain tile, the empty cell, or an index the palette does not have: no shape.
		KIND_BLOB,
		KIND_WALL,
	};
	// Flags of a layer's resolve: the map's looping axes (its scroll type) and the tileset's edge mode.
	enum Flags {
		LOOP_HORIZONTAL = 1,
		LOOP_VERTICAL = 2,
		EDGE_BORDER = 4, // Outside the map is another kind (without it, the same kind).
	};

	// The kind of every palette index of a JRPGTilesetData palette (index 0, the empty cell, is plain).
	static PackedByteArray get_palette_kinds(const Array &p_palette);
	// The flags of a map whose axes loop as given, drawn with a tileset whose edge mode is p_edge_mode. A map's scroll
	// type (spec 05: none, loop vertical, loop horizontal, loop both = 0..3) gives p_loop_horizontal = type & 2 and
	// p_loop_vertical = type & 1.
	static BitField<Flags> get_flags(bool p_loop_horizontal, bool p_loop_vertical, JRPGTilesetData::EdgeMode p_edge_mode);
	// The neighbor mask of cell p_cell (8 bits, JRPGAutotile::Neighbor order) for its own kind, before any reduction.
	static int get_neighbor_mask(const PackedInt32Array &p_cells, const Vector2i &p_size, const Vector2i &p_cell, BitField<Flags> p_flags);
	// The shape of cell p_cell: 0..46 for a blob kind, 0..15 for a wall kind, -1 for a plain or empty cell.
	static int resolve_cell(const PackedInt32Array &p_cells, const Vector2i &p_size, const PackedByteArray &p_kinds, const Vector2i &p_cell, BitField<Flags> p_flags);
	// The shapes of a whole layer (p_size.x * p_size.y cells, row by row): one byte per cell, 0 for plain cells.
	static PackedByteArray resolve_layer(const PackedInt32Array &p_cells, const Vector2i &p_size, const PackedByteArray &p_kinds, BitField<Flags> p_flags);
	// After cell p_cell of the layer changed, resolves its 3 x 3 neighborhood again (wrapping on looping axes) into
	// r_shapes, a resolve_layer result, and returns the cells whose shape changed (the edited cell always).
	static Vector<Vector2i> update_around(const PackedInt32Array &p_cells, const Vector2i &p_size, const PackedByteArray &p_kinds, const Vector2i &p_cell, BitField<Flags> p_flags, PackedByteArray &r_shapes);

	// Script-facing form of update_around: {shapes, changed}.
	static Dictionary update_around_bind(const PackedInt32Array &p_cells, const Vector2i &p_size, const PackedByteArray &p_kinds, const Vector2i &p_cell, BitField<Flags> p_flags, const PackedByteArray &p_shapes);
};

VARIANT_ENUM_CAST(JRPGAutotileResolver::Kind);
VARIANT_BITFIELD_CAST(JRPGAutotileResolver::Flags);
