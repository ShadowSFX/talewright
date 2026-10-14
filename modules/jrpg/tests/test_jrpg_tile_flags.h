/**************************************************************************/
/*  test_jrpg_tile_flags.h                                                */
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

#ifdef TOOLS_ENABLED

#include "core/io/dir_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/image_texture.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_autotile.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/editor/jrpg_autotile_importer.h"
#include "modules/jrpg/editor/jrpg_database_editing.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"
#include "modules/jrpg/editor/jrpg_tile_flags.h"

namespace TestJRPGTileFlags {

// A 48-pixel TileSet with a blank blob sheet (shapes 0..46) and a plain sheet of 4 tiles, listed in a palette.
struct Fixture {
	Ref<TileSet> tileset;
	Ref<JRPGTilesetData> data;

	Fixture() {
		tileset.instantiate();
		tileset->set_tile_size(Vector2i(48, 48));
		data.instantiate();
		PackedInt32Array indexes;
		String error;
		Ref<Image> blob = Image::create_empty(8 * 48, 6 * 48, false, Image::FORMAT_RGBA8);
		JRPGAutotileImporter::import_sheet(tileset, ImageTexture::create_from_image(blob), JRPGAutotileImporter::SHEET_AUTOTILE, 1, "Grass", data, indexes, error);
		Ref<Image> plain = Image::create_empty(4 * 48, 48, false, Image::FORMAT_RGBA8);
		plain->fill(Color(0.5, 0.5, 0.5));
		JRPGAutotileImporter::import_sheet(tileset, ImageTexture::create_from_image(plain), JRPGAutotileImporter::SHEET_PLAIN, 1, "Props", data, indexes, error);
	}
	Dictionary entry(int p_palette_index) const {
		return data->get_palette()[p_palette_index - 1];
	}
};

TEST_CASE("[SceneTree][JRPG][TileFlags] Every flag round-trips through the TileSet's custom data in the spec 05 layout") {
	Fixture f;
	CHECK(JRPGTileFlags::ensure_layers(f.tileset) == 9);
	CHECK(JRPGTileFlags::ensure_layers(f.tileset) == 0); // Already there.
	CHECK(f.tileset->get_custom_data_layer_type(f.tileset->get_custom_data_layer_by_name(JRPGTileFlags::LAYER_OVERHEAD)) == Variant::BOOL);
	// The bit layout: pass in bits 0-1, no-entry N/E/S/W in 2-5, no-exit in 6-9, overhead 10 ... damage 14, tag 16-19.
	CHECK(JRPGTilesetData::pack_flags(JRPGTilesetData::PASS_BLOCKED, 1 | 4, 2, JRPGTilesetData::FLAG_LADDER, 7) == (1 | (5 << 2) | (2 << 6) | (1 << 11) | (7 << 16)));
	TileData *tile = Object::cast_to<TileSetAtlasSource>(f.tileset->get_source(f.entry(2)["source_id"]).ptr())->get_tile_data(Vector2i(0, 0), 0);
	const uint32_t bits[] = { JRPGTilesetData::FLAG_OVERHEAD, JRPGTilesetData::FLAG_LADDER, JRPGTilesetData::FLAG_BUSH, JRPGTilesetData::FLAG_COUNTER, JRPGTilesetData::FLAG_DAMAGE };
	for (int pass = 0; pass <= JRPGTilesetData::PASS_TRANSPARENT; pass++) {
		for (int sides = 0; sides < 16; sides++) {
			for (uint32_t bit : bits) {
				const int tag = (sides * 3 + pass) % 16;
				const int word = JRPGTilesetData::pack_flags(JRPGTilesetData::PassMode(pass), sides, 15 - sides, bit, tag);
				JRPGTileFlags::write_tile(f.tileset, tile, word);
				const int read = JRPGTileFlags::read_tile(f.tileset, tile);
				CHECK(read == word);
				CHECK(JRPGTilesetData::get_pass_mode(read) == pass);
				CHECK(JRPGTilesetData::get_no_entry(read) == sides);
				CHECK(JRPGTilesetData::get_no_exit(read) == 15 - sides);
				CHECK((uint32_t(read) & bit) == bit);
				CHECK(JRPGTilesetData::get_terrain_tag(read) == tag);
			}
		}
	}
	// The custom data itself, as Godot's TileSet editor shows it.
	JRPGTileFlags::write_tile(f.tileset, tile, JRPGTilesetData::pack_flags(JRPGTilesetData::PASS_TRANSPARENT, 8, 0, JRPGTilesetData::FLAG_OVERHEAD, 3));
	CHECK(int(tile->get_custom_data(JRPGTileFlags::LAYER_PASS)) == 2);
	CHECK(int(tile->get_custom_data(JRPGTileFlags::LAYER_NO_ENTRY)) == 8);
	CHECK(bool(tile->get_custom_data(JRPGTileFlags::LAYER_OVERHEAD)));
	CHECK_FALSE(bool(tile->get_custom_data(JRPGTileFlags::LAYER_BUSH)));
	CHECK(int(tile->get_custom_data(JRPGTileFlags::LAYER_TERRAIN_TAG)) == 3);
}

TEST_CASE("[SceneTree][JRPG][TileFlags] An autotile kind's flags are set on all its shapes and baked once per palette index") {
	Fixture f;
	const int blocked = JRPGTilesetData::pack_flags(JRPGTilesetData::PASS_BLOCKED, 0, 0, 0, 0);
	REQUIRE(JRPGTileFlags::set_entry_flags(f.tileset, f.entry(1), blocked) == OK);
	const Ref<TileSetAtlasSource> blob = f.tileset->get_source(f.entry(1)["source_id"]);
	for (int i = 0; i < JRPGAutotile::BLOB_SHAPES; i++) {
		CHECK(JRPGTileFlags::read_tile(f.tileset, blob->get_tile_data(JRPGAutotile::get_blob_cell(i), 0)) == blocked);
	}
	CHECK(JRPGTileFlags::get_entry_flags(f.tileset, f.entry(1)) == blocked);
	const int bush = JRPGTilesetData::pack_flags(JRPGTilesetData::PASS_OPEN, 0, 0, JRPGTilesetData::FLAG_BUSH, 0);
	REQUIRE(JRPGTileFlags::set_entry_flags(f.tileset, f.entry(3), bush) == OK);

	String hash;
	const PackedInt32Array words = JRPGTileFlags::bake(f.tileset, f.data, hash);
	REQUIRE(words.size() == f.data->get_palette().size() + 1);
	CHECK(words[0] == 0); // The empty cell.
	CHECK(words[1] == blocked);
	CHECK(words[2] == 0);
	CHECK(words[3] == bush);
	CHECK(JRPGTileFlags::is_stale(f.tileset, f.data)); // Never baked.
	CHECK(JRPGTileFlags::rebake(f.tileset, f.data));
	CHECK(f.data->get_flags() == words);
	CHECK(f.data->get_flags_hash() == hash);
	CHECK(f.data->get_tile_flags(3) == bush);
	CHECK_FALSE(JRPGTileFlags::is_stale(f.tileset, f.data));
	CHECK_FALSE(JRPGTileFlags::rebake(f.tileset, f.data)); // Nothing to do.

	// Editing a flag makes the bake stale; baking again catches up.
	JRPGTileFlags::set_entry_flags(f.tileset, f.entry(2), JRPGTilesetData::pack_flags(JRPGTilesetData::PASS_OPEN, 0, 0, JRPGTilesetData::FLAG_COUNTER, 0));
	CHECK(JRPGTileFlags::is_stale(f.tileset, f.data));
	CHECK(JRPGTileFlags::rebake(f.tileset, f.data));
	CHECK(f.data->get_tile_flags(2) == JRPGTilesetData::FLAG_COUNTER);
	// So does a change of the palette.
	Array palette = f.data->get_palette();
	palette.remove_at(1);
	f.data->set_palette(palette);
	CHECK(JRPGTileFlags::is_stale(f.tileset, f.data));
}

TEST_CASE("[SceneTree][JRPG][TileFlags] Saving the database bakes stale tilesets again and saves the TileSets it edited") {
	Fixture f;
	const String path = TestUtils::get_temp_path("jrpg_tile_flags_tileset.tres");
	REQUIRE(ResourceSaver::save(f.tileset, path) == OK);
	f.tileset->set_path(path, true);
	JRPGDatabaseEditing *editing = memnew(JRPGDatabaseEditing);
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	editing->set_database(db);
	const Ref<JRPGTable> table = db->get_table(JRPGDatabase::TABLE_TILESETS);
	const String table_path = TestUtils::get_temp_path("jrpg_tile_flags_tilesets.tres");
	table->set_path(table_path, true);
	Ref<JRPGTilesetData> data = table->get_entry(1);
	data->set_palette(f.data->get_palette());
	data->set_tileset_uid(path);
	JRPGTileFlags::set_entry_flags(f.tileset, f.entry(1), JRPGTilesetData::pack_flags(JRPGTilesetData::PASS_BLOCKED, 0, 0, 0, 0));
	editing->mark_unsaved(f.tileset);
	CHECK(editing->get_unsaved_paths().has(path));
	ERR_PRINT_OFF; // The other starter tables have no files to save to.
	editing->save_all();
	ERR_PRINT_ON;
	CHECK(data->get_tile_flags(1) == JRPGTilesetData::PASS_BLOCKED);
	CHECK_FALSE(JRPGTileFlags::is_stale(f.tileset, data));
	CHECK_FALSE(editing->get_unsaved_paths().has(path)); // The TileSet was saved.
	CHECK_FALSE(editing->is_unsaved(table));

	// A flag edited later: the table itself has no change until the save bakes it, and then it is saved too.
	JRPGTileFlags::set_entry_flags(f.tileset, f.entry(2), JRPGTilesetData::pack_flags(JRPGTilesetData::PASS_OPEN, 0, 0, JRPGTilesetData::FLAG_COUNTER, 0));
	editing->mark_unsaved(f.tileset);
	CHECK_FALSE(editing->is_unsaved(table));
	ERR_PRINT_OFF;
	editing->save_all();
	ERR_PRINT_ON;
	CHECK_FALSE(editing->is_unsaved(table));
	const Ref<JRPGTable> saved = ResourceLoader::load(table_path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(saved.is_valid());
	const Ref<JRPGTilesetData> saved_data = saved->get_entry(1);
	REQUIRE(saved_data.is_valid());
	CHECK(saved_data->get_tile_flags(1) == JRPGTilesetData::PASS_BLOCKED);
	CHECK(saved_data->get_tile_flags(2) == JRPGTilesetData::FLAG_COUNTER);
	CHECK(saved_data->get_flags_hash() == data->get_flags_hash());
	memdelete(editing);
}

// A texture file the importer can load: an ImageTexture saved as a text resource (tests have no import pipeline).
String save_sheet(const String &p_name, const Vector2i &p_tiles, bool p_visible) {
	Ref<Image> image = Image::create_empty(p_tiles.x * 48, p_tiles.y * 48, false, Image::FORMAT_RGBA8);
	if (p_visible) {
		image->fill(Color(0.2, 0.6, 0.3));
	}
	const String path = TestUtils::get_temp_path(p_name + ".tres");
	REQUIRE(ResourceSaver::save(ImageTexture::create_from_image(image), path) == OK);
	return path;
}

TEST_CASE("[SceneTree][JRPG][TileFlags] The tileset authoring API creates a TileSet, adds sheets and sets flags") {
	JRPGDatabaseEditing *editing = memnew(JRPGDatabaseEditing);
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	editing->set_database(db);
	const Ref<JRPGTable> table = db->get_table(JRPGDatabase::TABLE_TILESETS);
	const Ref<JRPGTilesetData> data = table->get_entry(1);
	String message;

	// create_tileset.
	const String folder = TestUtils::get_temp_path("jrpg_authoring_tilesets");
	if (DirAccess::exists(folder)) {
		Ref<DirAccess> da = DirAccess::open(folder);
		REQUIRE(da->erase_contents_recursive() == OK);
	}
	const String path = folder.path_join("field.tres");
	CHECK(editing->create_tileset(1, folder.path_join("field.png"), 48, &message) == ERR_INVALID_PARAMETER);
	CHECK(message.contains(".tres or .res"));
	CHECK(editing->create_tileset(1, path, 4, &message) == ERR_PARAMETER_RANGE_ERROR);
	CHECK(editing->create_tileset(99, path, 48, &message) == ERR_DOES_NOT_EXIST);
	CHECK(message == "there is no tileset 99.");
	CHECK(editing->get_tileset_of(1).is_null());
	REQUIRE(editing->create_tileset(1, path, 48, &message) == OK);
	CHECK(data->get_tileset_uid() == path); // Outside res:// a path, not a UID.
	const Ref<TileSet> tileset = editing->get_tileset_of(1);
	REQUIRE(tileset.is_valid());
	CHECK(tileset->get_tile_size() == Vector2i(48, 48));
	CHECK(tileset->get_custom_data_layer_by_name(JRPGTileFlags::LAYER_TERRAIN_TAG) >= 0);
	CHECK(editing->create_tileset(2, path, 48, &message) == ERR_ALREADY_EXISTS); // Never replaced.
	CHECK(message.contains("already exists"));

	// add_tile_sheet.
	const String grass = save_sheet("jrpg_authoring_grass", Vector2i(8, 6), true);
	const String clear = save_sheet("jrpg_authoring_clear", Vector2i(2, 1), false);
	PackedInt32Array indexes;
	CHECK(editing->add_tile_sheet(2, grass, JRPGAutotileImporter::SHEET_AUTOTILE, 1, "Grass", indexes, &message) == ERR_UNCONFIGURED);
	CHECK(message.contains("has no TileSet"));
	CHECK(editing->add_tile_sheet(1, folder.path_join("nothing.tres"), JRPGAutotileImporter::SHEET_AUTOTILE, 1, "Grass", indexes, &message) == ERR_FILE_NOT_FOUND);
	CHECK(editing->add_tile_sheet(1, grass, JRPGAutotileImporter::SHEET_AUTOTILE, 1, "  ", indexes, &message) == ERR_INVALID_PARAMETER);
	CHECK(editing->add_tile_sheet(1, grass, JRPGAutotileImporter::SHEET_WALL, 1, "Grass", indexes, &message) == ERR_INVALID_PARAMETER);
	CHECK(message.contains("wall sheet"));
	CHECK(editing->add_tile_sheet(1, clear, JRPGAutotileImporter::SHEET_PLAIN, 1, "Clear", indexes, &message) == ERR_INVALID_PARAMETER);
	CHECK(message.contains("no visible tile"));
	CHECK(data->get_palette().is_empty()); // Refusals change nothing.
	CHECK(tileset->get_source_count() == 0);
	CHECK(tileset->get_terrain_sets_count() == 0);
	REQUIRE(editing->add_tile_sheet(1, grass, JRPGAutotileImporter::SHEET_AUTOTILE, 1, " Grass ", indexes, &message) == OK);
	CHECK(indexes == PackedInt32Array({ 1 }));
	CHECK(String(Dictionary(data->get_palette()[0])["name"]) == "Grass");
	CHECK(tileset->get_source_count() == 1);
	CHECK(tileset->get_terrains_count(0) == 1);
	CHECK(editing->get_unsaved_paths().has(path)); // Saved with the database.
	const String props = save_sheet("jrpg_authoring_props", Vector2i(3, 1), true);
	REQUIRE(editing->add_tile_sheet(1, props, JRPGAutotileImporter::SHEET_PLAIN, 1, "Props", indexes, &message) == OK);
	CHECK(indexes == PackedInt32Array({ 2, 3, 4 }));

	// set_tile_flags.
	const int bush = JRPGTilesetData::FLAG_BUSH;
	const int pass = JRPGTilesetData::FLAG_PASS_MASK;
	REQUIRE(editing->set_tile_flags(1, PackedInt32Array({ 1, 3 }), bush, bush, &message) == OK);
	const Dictionary grass_entry = data->get_palette()[0];
	const Ref<TileSetAtlasSource> grass_source = tileset->get_source(grass_entry["source_id"]);
	// One shape gets a terrain tag of its own: setting the passage later keeps it.
	TileData *odd = grass_source->get_tile_data(JRPGAutotile::get_blob_cell(5), 0);
	JRPGTileFlags::write_tile(tileset, odd, JRPGTileFlags::read_tile(tileset, odd) | (7 << JRPGTilesetData::FLAG_TERRAIN_TAG_SHIFT));
	REQUIRE(editing->set_tile_flags(1, PackedInt32Array({ 1 }), pass, JRPGTilesetData::PASS_BLOCKED, &message) == OK);
	const PackedInt32Array words = JRPGTileFlags::get_tile_words(tileset, grass_entry);
	REQUIRE(words.size() == JRPGAutotile::BLOB_SHAPES);
	for (int i = 0; i < words.size(); i++) {
		CHECK(JRPGTilesetData::get_pass_mode(words[i]) == JRPGTilesetData::PASS_BLOCKED);
		CHECK((words[i] & bush) == bush);
		CHECK(JRPGTilesetData::get_terrain_tag(words[i]) == (i == 5 ? 7 : 0));
	}
	CHECK(JRPGTileFlags::get_entry_flags(tileset, data->get_palette()[2]) == bush); // Index 3, a plain tile.
	CHECK(JRPGTileFlags::get_entry_flags(tileset, data->get_palette()[1]) == 0); // Index 2 untouched.
	// Already so: nothing to do, no error.
	CHECK(editing->set_tile_flags(1, PackedInt32Array({ 1 }), pass, JRPGTilesetData::PASS_BLOCKED, &message) == OK);
	// Refusals.
	CHECK(editing->set_tile_flags(1, PackedInt32Array({ 0 }), bush, bush, &message) == ERR_PARAMETER_RANGE_ERROR);
	CHECK(message.contains("no palette index 0 (1 to 4)"));
	CHECK(editing->set_tile_flags(1, PackedInt32Array({ 5 }), bush, bush, &message) == ERR_PARAMETER_RANGE_ERROR);
	CHECK(editing->set_tile_flags(1, PackedInt32Array(), bush, bush, &message) == ERR_INVALID_PARAMETER);
	CHECK(editing->set_tile_flags(1, PackedInt32Array({ 1 }), 0, 0, &message) == ERR_INVALID_PARAMETER);
	CHECK(editing->set_tile_flags(1, PackedInt32Array({ 1 }), 1 << 15, 1 << 15, &message) == ERR_INVALID_PARAMETER);
	CHECK(editing->set_tile_flags(1, PackedInt32Array({ 1 }), 1, 1, &message) == ERR_INVALID_PARAMETER); // Half the passage.
	CHECK(editing->set_tile_flags(1, PackedInt32Array({ 1 }), pass, 3, &message) == ERR_INVALID_PARAMETER);
	CHECK(message.contains("2 (transparent)"));
	CHECK(editing->set_tile_flags(2, PackedInt32Array({ 1 }), bush, bush, &message) == ERR_UNCONFIGURED);
	Array palette = data->get_palette();
	Dictionary ghost = Dictionary(palette[1]).duplicate();
	ghost["source_id"] = 77;
	palette.push_back(ghost);
	data->set_palette(palette);
	CHECK(editing->set_tile_flags(1, PackedInt32Array({ 5 }), bush, bush, &message) == ERR_DOES_NOT_EXIST);
	CHECK(message.contains("does not have"));
	// The field table names what the form and the bridge show.
	CHECK(JRPGTileFlags::find_field(pass) == JRPGTileFlags::FIELD_PASS);
	CHECK(JRPGTileFlags::find_field(0xF << JRPGTilesetData::FLAG_NO_EXIT_SHIFT) == JRPGTileFlags::FIELD_NO_EXIT);
	CHECK(JRPGTileFlags::find_field(bush | pass) == JRPGTileFlags::FIELD_MAX);
	for (int i = 0; i < JRPGTileFlags::FIELD_MAX; i++) {
		const JRPGTileFlags::FieldInfo &field = JRPGTileFlags::get_field(JRPGTileFlags::Field(i));
		CHECK((field.mask >> field.shift) == (field.max == 2 ? 3 : field.max)); // The bits hold the values.
	}
	memdelete(editing);
}

} // namespace TestJRPGTileFlags

#endif // TOOLS_ENABLED
