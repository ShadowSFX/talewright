/**************************************************************************/
/*  jrpg_autotile_importer.h                                              */
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
#include "core/variant/binder_common.h" // IWYU pragma: keep (VARIANT_ENUM_CAST).

class JRPGTilesetData;
class Texture2D;
class TileSet;

// Adds tile sheets to a project TileSet and lists their tiles in a JRPG tileset's palette (task 7.2, spec 05).
// Autotile sheets follow the Talewright templates (JRPGAutotile): a blob sheet of 8 x 6 tiles per frame with animation
// frames side by side, and a wall sheet of 4 x 4 tiles. The importer makes the sheet an atlas source, creates its
// tiles, wires animation frames through the atlas animation columns, sets the terrain peering bits (so Godot's own
// terrain painter also works) and appends palette entries: one per kind for autotiles and walls, one per tile for a
// plain sheet. Editor-only; not undoable by itself (the callers wrap it).
class JRPGAutotileImporter : public RefCounted {
	GDCLASS(JRPGAutotileImporter, RefCounted);

protected:
	static void _bind_methods();

public:
	enum SheetKind {
		SHEET_PLAIN,
		SHEET_AUTOTILE,
		SHEET_WALL,
	};

	// Imports p_texture into p_tileset as a new atlas source laid out as p_kind with p_frames animation frames (1 for
	// still sheets; autotile and wall sheets only) and appends the palette entries to p_data. r_indexes: the palette
	// indexes added. Fails with r_error, changing nothing, when the image size does not fit the layout at the
	// tileset's tile size or a plain sheet shows no tile.
	static Error import_sheet(const Ref<TileSet> &p_tileset, const Ref<Texture2D> &p_texture, SheetKind p_kind, int p_frames, const String &p_name, const Ref<JRPGTilesetData> &p_data, PackedInt32Array &r_indexes, String &r_error);
	// Script-facing form: {indexes} on success, {error} otherwise.
	static Dictionary import_sheet_bind(const Ref<TileSet> &p_tileset, const Ref<Texture2D> &p_texture, SheetKind p_kind, int p_frames, const String &p_name, const Ref<JRPGTilesetData> &p_data);
	// "plain", "autotile" or "wall": the palette entry's kind.
	static String get_kind_name(SheetKind p_kind);
	// The kind named p_name (get_kind_name), false when there is none.
	static bool kind_from_name(const String &p_name, SheetKind &r_kind);
	// The layout an image of p_cells tiles most likely has: an autotile sheet when its size fits one (r_frames side by
	// side), a wall sheet at exactly 4 x 4 tiles, a sheet of plain tiles otherwise.
	static SheetKind guess_kind(const Vector2i &p_cells, int &r_frames);
};

VARIANT_ENUM_CAST(JRPGAutotileImporter::SheetKind);
