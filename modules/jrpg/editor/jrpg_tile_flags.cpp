/**************************************************************************/
/*  jrpg_tile_flags.cpp                                                   */
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

#include "jrpg_tile_flags.h"

#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "scene/resources/2d/tile_set.h"

#include "modules/jrpg/core/jrpg_autotile.h"
#include "modules/jrpg/core/jrpg_misc_data.h"

namespace {

struct Layer {
	const char *name;
	Variant::Type type;
};

const Layer LAYERS[] = {
	{ JRPGTileFlags::LAYER_PASS, Variant::INT },
	{ JRPGTileFlags::LAYER_NO_ENTRY, Variant::INT },
	{ JRPGTileFlags::LAYER_NO_EXIT, Variant::INT },
	{ JRPGTileFlags::LAYER_OVERHEAD, Variant::BOOL },
	{ JRPGTileFlags::LAYER_LADDER, Variant::BOOL },
	{ JRPGTileFlags::LAYER_BUSH, Variant::BOOL },
	{ JRPGTileFlags::LAYER_COUNTER, Variant::BOOL },
	{ JRPGTileFlags::LAYER_DAMAGE, Variant::BOOL },
	{ JRPGTileFlags::LAYER_TERRAIN_TAG, Variant::INT },
};

// Boolean layers and their bits.
struct Bit {
	const char *layer;
	uint32_t bit;
};
const Bit BITS[] = {
	{ JRPGTileFlags::LAYER_OVERHEAD, JRPGTilesetData::FLAG_OVERHEAD },
	{ JRPGTileFlags::LAYER_LADDER, JRPGTilesetData::FLAG_LADDER },
	{ JRPGTileFlags::LAYER_BUSH, JRPGTilesetData::FLAG_BUSH },
	{ JRPGTileFlags::LAYER_COUNTER, JRPGTilesetData::FLAG_COUNTER },
	{ JRPGTileFlags::LAYER_DAMAGE, JRPGTilesetData::FLAG_DAMAGE },
};

Variant layer_value(const Ref<TileSet> &p_tileset, const TileData *p_tile, const char *p_layer) {
	const int index = p_tileset->get_custom_data_layer_by_name(p_layer);
	return index >= 0 ? p_tile->get_custom_data_by_layer_id(index) : Variant();
}

TileData *entry_tile(const Ref<TileSet> &p_tileset, const Dictionary &p_entry, const Vector2i &p_coords) {
	const int source_id = p_entry.get("source_id", -1);
	if (!p_tileset->has_source(source_id)) {
		return nullptr;
	}
	const Ref<TileSetAtlasSource> atlas = p_tileset->get_source(source_id);
	const int alternative = p_entry.get("alternative", 0);
	if (atlas.is_null() || !atlas->has_tile(p_coords) || !atlas->has_alternative_tile(p_coords, alternative)) {
		return nullptr;
	}
	return atlas->get_tile_data(p_coords, alternative);
}

} // namespace

int JRPGTileFlags::ensure_layers(const Ref<TileSet> &p_tileset) {
	ERR_FAIL_COND_V(p_tileset.is_null(), 0);
	int added = 0;
	for (const Layer &layer : LAYERS) {
		if (p_tileset->get_custom_data_layer_by_name(layer.name) >= 0) {
			continue;
		}
		const int index = p_tileset->get_custom_data_layers_count();
		p_tileset->add_custom_data_layer();
		p_tileset->set_custom_data_layer_name(index, layer.name);
		p_tileset->set_custom_data_layer_type(index, layer.type);
		added++;
	}
	return added;
}

int JRPGTileFlags::read_tile(const Ref<TileSet> &p_tileset, const TileData *p_tile) {
	ERR_FAIL_COND_V(p_tileset.is_null() || !p_tile, 0);
	int bits = 0;
	for (const Bit &b : BITS) {
		if (bool(layer_value(p_tileset, p_tile, b.layer))) {
			bits |= b.bit;
		}
	}
	const int pass = CLAMP(int(layer_value(p_tileset, p_tile, LAYER_PASS)), 0, int(JRPGTilesetData::PASS_TRANSPARENT));
	return JRPGTilesetData::pack_flags(JRPGTilesetData::PassMode(pass), layer_value(p_tileset, p_tile, LAYER_NO_ENTRY), layer_value(p_tileset, p_tile, LAYER_NO_EXIT), bits, CLAMP(int(layer_value(p_tileset, p_tile, LAYER_TERRAIN_TAG)), 0, 15));
}

void JRPGTileFlags::write_tile(const Ref<TileSet> &p_tileset, TileData *p_tile, int p_flags) {
	ERR_FAIL_COND(p_tileset.is_null() || !p_tile);
	p_tile->set_custom_data(LAYER_PASS, int(JRPGTilesetData::get_pass_mode(p_flags)));
	p_tile->set_custom_data(LAYER_NO_ENTRY, JRPGTilesetData::get_no_entry(p_flags));
	p_tile->set_custom_data(LAYER_NO_EXIT, JRPGTilesetData::get_no_exit(p_flags));
	for (const Bit &b : BITS) {
		p_tile->set_custom_data(b.layer, bool(uint32_t(p_flags) & b.bit));
	}
	p_tile->set_custom_data(LAYER_TERRAIN_TAG, JRPGTilesetData::get_terrain_tag(p_flags));
}

Vector<Vector2i> JRPGTileFlags::get_entry_tiles(const Dictionary &p_entry) {
	Vector<Vector2i> tiles;
	const Vector2i origin = p_entry.get("atlas_coords", Vector2i());
	const String kind = p_entry.get("kind", "plain");
	if (kind == "autotile") {
		for (int i = 0; i < JRPGAutotile::BLOB_SHAPES; i++) {
			tiles.push_back(origin + JRPGAutotile::get_blob_cell(i));
		}
	} else if (kind == "wall") {
		for (int i = 0; i < JRPGAutotile::WALL_SHAPES; i++) {
			tiles.push_back(origin + JRPGAutotile::get_wall_cell(i));
		}
	} else {
		tiles.push_back(origin);
	}
	return tiles;
}

int JRPGTileFlags::get_entry_flags(const Ref<TileSet> &p_tileset, const Dictionary &p_entry) {
	ERR_FAIL_COND_V(p_tileset.is_null(), 0);
	const TileData *tile = entry_tile(p_tileset, p_entry, get_entry_tiles(p_entry)[0]);
	return tile ? read_tile(p_tileset, tile) : 0;
}

Error JRPGTileFlags::set_entry_flags(const Ref<TileSet> &p_tileset, const Dictionary &p_entry, int p_flags) {
	ERR_FAIL_COND_V(p_tileset.is_null(), ERR_INVALID_PARAMETER);
	ensure_layers(p_tileset);
	bool any = false;
	for (const Vector2i &coords : get_entry_tiles(p_entry)) {
		TileData *tile = entry_tile(p_tileset, p_entry, coords);
		if (tile) {
			write_tile(p_tileset, tile, p_flags);
			any = true;
		}
	}
	return any ? OK : ERR_DOES_NOT_EXIST;
}

PackedInt32Array JRPGTileFlags::bake(const Ref<TileSet> &p_tileset, const Ref<JRPGTilesetData> &p_data, String &r_hash) {
	PackedInt32Array words;
	ERR_FAIL_COND_V(p_tileset.is_null() || p_data.is_null(), words);
	const Array palette = p_data->get_palette();
	words.push_back(0); // Palette index 0: the empty cell.
	String source = "v1";
	for (int i = 0; i < palette.size(); i++) {
		const Dictionary entry = palette[i];
		const int word = get_entry_flags(p_tileset, entry);
		words.push_back(word);
		// What each word was read from (the entry's tile) and its value: a changed tile, palette or flag changes it.
		source += vformat("|%d:%d:%s:%d:%d", i + 1, int(entry.get("source_id", -1)), Vector2i(entry.get("atlas_coords", Vector2i())), int(entry.get("alternative", 0)), word);
	}
	r_hash = source.sha256_text();
	return words;
}

bool JRPGTileFlags::is_stale(const Ref<TileSet> &p_tileset, const Ref<JRPGTilesetData> &p_data) {
	ERR_FAIL_COND_V(p_tileset.is_null() || p_data.is_null(), false);
	String hash;
	const PackedInt32Array words = bake(p_tileset, p_data, hash);
	return hash != p_data->get_flags_hash() || words != p_data->get_flags();
}

bool JRPGTileFlags::rebake(const Ref<TileSet> &p_tileset, const Ref<JRPGTilesetData> &p_data) {
	ERR_FAIL_COND_V(p_tileset.is_null() || p_data.is_null(), false);
	String hash;
	const PackedInt32Array words = bake(p_tileset, p_data, hash);
	if (hash == p_data->get_flags_hash() && words == p_data->get_flags()) {
		return false;
	}
	p_data->set_flags(words);
	p_data->set_flags_hash(hash);
	return true;
}

Ref<TileSet> JRPGTileFlags::load_tileset(const Ref<JRPGTilesetData> &p_data) {
	if (p_data.is_null() || p_data->get_tileset_uid().is_empty() || !ResourceLoader::exists(p_data->get_tileset_uid())) {
		return Ref<TileSet>();
	}
	return ResourceLoader::load(p_data->get_tileset_uid());
}

void JRPGTileFlags::_bind_methods() {
	ClassDB::bind_static_method("JRPGTileFlags", D_METHOD("ensure_layers", "tileset"), &JRPGTileFlags::ensure_layers);
	ClassDB::bind_static_method("JRPGTileFlags", D_METHOD("get_entry_flags", "tileset", "entry"), &JRPGTileFlags::get_entry_flags);
	ClassDB::bind_static_method("JRPGTileFlags", D_METHOD("set_entry_flags", "tileset", "entry", "flags"), &JRPGTileFlags::set_entry_flags);
	ClassDB::bind_static_method("JRPGTileFlags", D_METHOD("is_stale", "tileset", "data"), &JRPGTileFlags::is_stale);
	ClassDB::bind_static_method("JRPGTileFlags", D_METHOD("rebake", "tileset", "data"), &JRPGTileFlags::rebake);
}
