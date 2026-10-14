/**************************************************************************/
/*  test_jrpg_tileset_editor.h                                            */
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

#include "core/input/input_event.h"
#include "core/io/dir_access.h"
#include "core/io/image.h"
#include "core/io/resource_saver.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/spin_box.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/image_texture.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/editor/jrpg_database_editing.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"
#include "modules/jrpg/editor/jrpg_tile_flags.h"
#include "modules/jrpg/editor/jrpg_tileset_editor.h"

namespace TestJRPGTilesetEditor {

using Field = JRPGTileFlags::Field;

TEST_CASE("[JRPG][TilesetEditor] A click cycles, toggles or steps the flag of its mode") {
	const int blocked = JRPGTilesetData::PASS_BLOCKED;
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_PASS, 0, -1, false) == blocked);
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_PASS, blocked, -1, false) == JRPGTilesetData::PASS_TRANSPARENT);
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_PASS, JRPGTilesetData::PASS_TRANSPARENT, -1, false) == 0);
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_PASS, 0, -1, true) == JRPGTilesetData::PASS_TRANSPARENT); // Back.
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_BUSH, 0, -1, false) == 1);
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_BUSH, JRPGTilesetData::FLAG_BUSH, -1, true) == 0);
	// Other bits of the word do not leak into the value.
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_LADDER, JRPGTilesetData::FLAG_BUSH | blocked, -1, false) == 1);
	const int tag_15 = 15 << JRPGTilesetData::FLAG_TERRAIN_TAG_SHIFT;
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_TERRAIN_TAG, tag_15, -1, false) == 0); // Wraps.
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_TERRAIN_TAG, 0, -1, true) == 15);
	// Sides: an edge toggles its side; the middle sets all four, or clears them when all are set.
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_NO_ENTRY, 0, 0, false) == 1);
	const int north_west = (1 | 8) << JRPGTilesetData::FLAG_NO_EXIT_SHIFT;
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_NO_EXIT, north_west, 3, false) == 1);
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_NO_EXIT, north_west, -1, false) == 15);
	CHECK(JRPGEditorPropertyTiles::next_value(JRPGTileFlags::FIELD_NO_ENTRY, 15 << JRPGTilesetData::FLAG_NO_ENTRY_SHIFT, -1, false) == 0);

	const Rect2 tile(10, 10, 48, 48);
	CHECK(JRPGEditorPropertyTiles::side_at(tile, Vector2(34, 12)) == 0); // North.
	CHECK(JRPGEditorPropertyTiles::side_at(tile, Vector2(56, 34)) == 1); // East.
	CHECK(JRPGEditorPropertyTiles::side_at(tile, Vector2(30, 57)) == 2); // South.
	CHECK(JRPGEditorPropertyTiles::side_at(tile, Vector2(11, 40)) == 3); // West.
	CHECK(JRPGEditorPropertyTiles::side_at(tile, Vector2(36, 32)) == -1); // The middle.
	CHECK(JRPGEditorPropertyTiles::side_at(tile, tile.position) == 0); // A corner goes to north or south first.
	CHECK(JRPGEditorPropertyTiles::side_at(Rect2(), Vector2()) == -1);

	int frames = 0;
	CHECK(JRPGAutotileImporter::guess_kind(Vector2i(8, 6), frames) == JRPGAutotileImporter::SHEET_AUTOTILE);
	CHECK(frames == 1);
	CHECK(JRPGAutotileImporter::guess_kind(Vector2i(24, 6), frames) == JRPGAutotileImporter::SHEET_AUTOTILE);
	CHECK(frames == 3);
	CHECK(JRPGAutotileImporter::guess_kind(Vector2i(4, 4), frames) == JRPGAutotileImporter::SHEET_WALL);
	CHECK(JRPGAutotileImporter::guess_kind(Vector2i(8, 4), frames) == JRPGAutotileImporter::SHEET_PLAIN); // Not a wall.
	CHECK(JRPGAutotileImporter::guess_kind(Vector2i(8, 8), frames) == JRPGAutotileImporter::SHEET_PLAIN);
	CHECK(frames == 1);
	CHECK(JRPGAutotileImporter::guess_kind(Vector2i(0, 6), frames) == JRPGAutotileImporter::SHEET_PLAIN);
}

String save_sheet(const String &p_name, const Vector2i &p_tiles) {
	Ref<Image> image = Image::create_empty(p_tiles.x * 48, p_tiles.y * 48, false, Image::FORMAT_RGBA8);
	image->fill(Color(0.3, 0.5, 0.7));
	const String path = TestUtils::get_temp_path(p_name + ".tres");
	REQUIRE(ResourceSaver::save(ImageTexture::create_from_image(image), path) == OK);
	return path;
}

Button *find_button(Node *p_root, const String &p_text) {
	for (const Variant &v : p_root->find_children("*", "Button", true, false)) {
		Button *button = Object::cast_to<Button>(v);
		if (button && button->get_text() == p_text) {
			return button;
		}
	}
	return nullptr;
}

Ref<InputEventKey> enter(bool p_shift) {
	Ref<InputEventKey> key;
	key.instantiate();
	key->set_keycode(Key::ENTER);
	key->set_physical_keycode(Key::ENTER);
	key->set_shift_pressed(p_shift);
	key->set_pressed(true);
	return key;
}

TEST_CASE("[SceneTree][JRPG][TilesetEditor] The tiles form shows the palette and edits flags by click, Enter and side buttons") {
	JRPGDatabaseEditing *editing = memnew(JRPGDatabaseEditing);
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	editing->set_database(db);
	const Ref<JRPGTable> table = db->get_table(JRPGDatabase::TABLE_TILESETS);
	const Ref<JRPGTilesetData> data = table->get_entry(1);
	const Ref<JRPGTilesetData> bare = table->get_entry(2);
	const String folder = TestUtils::get_temp_path("jrpg_tileset_editor");
	if (DirAccess::exists(folder)) {
		Ref<DirAccess> da = DirAccess::open(folder);
		REQUIRE(da->erase_contents_recursive() == OK);
	}
	REQUIRE(editing->create_tileset(1, folder.path_join("field.tres"), 48) == OK);
	PackedInt32Array indexes;
	REQUIRE(editing->add_tile_sheet(1, save_sheet("jrpg_tileset_editor_grass", Vector2i(8, 6)), JRPGAutotileImporter::SHEET_AUTOTILE, 1, "Grass", indexes) == OK);
	REQUIRE(editing->add_tile_sheet(1, save_sheet("jrpg_tileset_editor_props", Vector2i(2, 1)), JRPGAutotileImporter::SHEET_PLAIN, 1, "Props", indexes) == OK);
	const Ref<TileSet> tileset = editing->get_tileset_of(1);
	REQUIRE(tileset.is_valid());
	const auto word = [&](int p_index) {
		return JRPGTileFlags::get_entry_flags(tileset, data->get_palette()[p_index - 1]);
	};

	ERR_PRINT_OFF; // The editor theme reads editor settings that tests do not have.
	JRPGEditorPropertyTiles *editor = memnew(JRPGEditorPropertyTiles);
	SceneTree::get_singleton()->get_root()->add_child(editor);
	ERR_PRINT_ON;
	editor->set_object_and_property(data.ptr(), "palette");
	editor->update_property();
	editor->update_cache();
	ItemList *list = Object::cast_to<ItemList>(editor->find_children("*", "ItemList", true, false)[0]);
	REQUIRE(list != nullptr);
	CHECK(list->get_item_count() == 3); // The grass kind and two props.
	CHECK(list->get_fixed_icon_size() == Size2i(48, 48));
	CHECK(list->get_item_tooltip(0).contains("Grass"));
	CHECK(list->get_item_tooltip(0).contains("Autotile"));
	CHECK(list->get_item_icon(0).is_valid());

	SUBCASE("Passage cycles on a click; the whole kind changes, as one action") {
		find_button(editor, "Passage")->set_pressed(true);
		list->emit_signal(SNAME("item_clicked"), 0, Vector2(), MouseButton::LEFT);
		CHECK(JRPGTilesetData::get_pass_mode(word(1)) == JRPGTilesetData::PASS_BLOCKED);
		const Dictionary grass = data->get_palette()[0];
		for (int words : JRPGTileFlags::get_tile_words(tileset, grass)) {
			CHECK(JRPGTilesetData::get_pass_mode(words) == JRPGTilesetData::PASS_BLOCKED);
		}
		list->emit_signal(SNAME("item_clicked"), 0, Vector2(), MouseButton::RIGHT);
		CHECK(JRPGTilesetData::get_pass_mode(word(1)) == JRPGTilesetData::PASS_OPEN);
		list->emit_signal(SNAME("item_clicked"), 2, Vector2(), MouseButton::MIDDLE); // Not an edit.
		CHECK(word(3) == 0);
	}

	SUBCASE("A flag toggles; Enter acts on the selected tile and Shift+Enter steps back") {
		find_button(editor, "Bush")->set_pressed(true);
		list->emit_signal(SNAME("item_clicked"), 1, Vector2(), MouseButton::LEFT);
		CHECK(word(2) == int(JRPGTilesetData::FLAG_BUSH));
		CHECK(word(3) == 0);
		find_button(editor, "Terrain Tag")->set_pressed(true);
		list->select(2);
		list->emit_signal(SceneStringName(gui_input), enter(false));
		list->emit_signal(SceneStringName(gui_input), enter(false));
		CHECK(JRPGTilesetData::get_terrain_tag(word(3)) == 2);
		list->emit_signal(SceneStringName(gui_input), enter(true));
		CHECK(JRPGTilesetData::get_terrain_tag(word(3)) == 1);
	}

	SUBCASE("Sides: a click on an edge blocks that side; the side buttons follow the selected tile") {
		find_button(editor, "Entry Sides")->set_pressed(true);
		Button *north = find_button(editor, "North");
		Button *west = find_button(editor, "West");
		REQUIRE(north != nullptr);
		REQUIRE(north->is_visible());
		list->select(1);
		list->emit_signal(SceneStringName(item_selected), 1);
		CHECK_FALSE(north->is_disabled());
		const Rect2 rect = list->get_item_rect(1);
		list->emit_signal(SNAME("item_clicked"), 1, rect.position + Vector2(rect.size.x / 2, 2), MouseButton::LEFT);
		CHECK(JRPGTilesetData::get_no_entry(word(2)) == 1);
		west->set_pressed(true); // Pressing it, as the keyboard does.
		CHECK(JRPGTilesetData::get_no_entry(word(2)) == (1 | 8));
		CHECK(JRPGTilesetData::get_no_exit(word(2)) == 0);
		SceneTree::get_singleton()->process(0); // The deferred refresh.
		CHECK(north->is_pressed());
		find_button(editor, "Exit Sides")->set_pressed(true);
		CHECK_FALSE(north->is_pressed()); // Exit sides of the same tile.
		find_button(editor, "Overhead")->set_pressed(true);
		CHECK_FALSE(north->is_visible_in_tree());
	}

	SUBCASE("Without a TileSet the form offers to create one; with an empty palette it asks for a sheet") {
		editor->set_object_and_property(bare.ptr(), "palette");
		editor->update_property();
		Button *create = find_button(editor, "Create TileSet");
		REQUIRE(create != nullptr);
		CHECK(create->is_visible_in_tree());
		CHECK_FALSE(list->is_visible_in_tree());
		// Choosing a TileSet above (tileset_uid) shows it, though the palette did not change.
		editor->update_cache();
		CHECK(editor->is_cache_valid());
		REQUIRE(editing->create_tileset(2, folder.path_join("cave.tres"), 48) == OK);
		CHECK_FALSE(editor->is_cache_valid());
		editor->update_property();
		CHECK_FALSE(create->is_visible_in_tree());
		CHECK(list->get_item_count() == 0);
		Button *add = find_button(editor, "Add Sheet...");
		REQUIRE(add != nullptr);
		CHECK(add->is_visible_in_tree());
	}

	SUBCASE("The sheet dialog guesses the layout, refuses a size that does not fit and adds the sheet") {
		JRPGTileSheetDialog *dialog = Object::cast_to<JRPGTileSheetDialog>(editor->find_children("*", "JRPGTileSheetDialog", true, false)[0]);
		REQUIRE(dialog != nullptr);
		const String wall = save_sheet("jrpg_tileset_editor_wall", Vector2i(4, 4));
		dialog->open(1, wall, Vector2i(192, 192), Vector2i(48, 48));
		OptionButton *kind = Object::cast_to<OptionButton>(dialog->find_children("*", "OptionButton", true, false)[0]);
		LineEdit *name = nullptr;
		for (const Variant &v : dialog->find_children("*", "LineEdit", true, false)) {
			LineEdit *line = Object::cast_to<LineEdit>(v);
			if (!Object::cast_to<SpinBox>(line->get_parent())) {
				name = line; // Not the frame count's own field.
			}
		}
		REQUIRE(name != nullptr);
		CHECK(kind->get_selected() == JRPGAutotileImporter::SHEET_WALL);
		CHECK(name->get_text() == "Jrpg Tileset Editor Wall");
		CHECK_FALSE(dialog->get_ok_button()->is_disabled());
		kind->select(JRPGAutotileImporter::SHEET_AUTOTILE);
		kind->emit_signal(SceneStringName(item_selected), JRPGAutotileImporter::SHEET_AUTOTILE);
		CHECK(dialog->get_ok_button()->is_disabled()); // 4 x 4 tiles are no autotile sheet.
		kind->select(JRPGAutotileImporter::SHEET_WALL);
		kind->emit_signal(SceneStringName(item_selected), JRPGAutotileImporter::SHEET_WALL);
		name->set_text("Stone Wall");
		name->emit_signal(SceneStringName(text_changed), "Stone Wall");
		dialog->emit_signal(SceneStringName(confirmed));
		CHECK_FALSE(dialog->is_visible());
		REQUIRE(data->get_palette().size() == 4);
		CHECK(String(Dictionary(data->get_palette()[3])["kind"]) == "wall");
		CHECK(String(Dictionary(data->get_palette()[3])["name"]) == "Stone Wall");
	}

	memdelete(editor);
	memdelete(editing);
}

} // namespace TestJRPGTilesetEditor

#endif // TOOLS_ENABLED
