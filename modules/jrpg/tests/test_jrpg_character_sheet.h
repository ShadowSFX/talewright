/**************************************************************************/
/*  test_jrpg_character_sheet.h                                           */
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

#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/sprite_frames.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_map_cell_cache.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_mover.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/runtime/jrpg_character_sheet.h"
#include "modules/jrpg/runtime/jrpg_character_sprite.h"

#ifdef TOOLS_ENABLED
#include "modules/jrpg/editor/jrpg_character_sheet_importer.h"
#endif

namespace TestJRPGCharacterSheet {

using Sheet = JRPGCharacterSheet;

// A sheet of p_columns x p_rows frames of 48 pixels.
static Ref<ImageTexture> sheet(int p_columns, int p_rows) {
	Ref<Image> image = Image::create_empty(p_columns * 48, p_rows * 48, false, Image::FORMAT_RGBA8);
	image->fill(Color(0.2, 0.4, 0.8));
	return ImageTexture::create_from_image(image);
}

static Rect2 region(const Ref<SpriteFrames> &p_frames, const char *p_animation, int p_frame) {
	const Ref<AtlasTexture> frame = p_frames->get_frame_texture(p_animation, p_frame);
	return frame.is_valid() ? frame->get_region() : Rect2(-1, -1, 0, 0);
}

TEST_CASE("[JRPG][CharacterSheet] A 4 x 4 sheet gives four directions of four frames, column 0 standing") {
	const Ref<ImageTexture> texture = sheet(4, 4);
	String error;
	const Ref<SpriteFrames> frames = Sheet::make_frames(texture, Sheet::Layout(), &error);
	REQUIRE_MESSAGE(frames.is_valid(), error);
	CHECK(frames->get_animation_names().size() == 4); // No "default" left.
	for (int row = 0; row < 4; row++) {
		const char *animation = Sheet::DIRECTION_NAMES[row]; // Up, right, down, left.
		REQUIRE(frames->has_animation(animation));
		CHECK(frames->get_frame_count(animation) == 4);
		CHECK(frames->get_animation_loop(animation));
		for (int column = 0; column < 4; column++) {
			CHECK_MESSAGE(region(frames, animation, column) == Rect2(column * 48, row * 48, 48, 48), animation);
			CHECK(Ref<AtlasTexture>(frames->get_frame_texture(animation, column))->get_atlas() == texture);
		}
	}
	CHECK((Sheet::get_y_offset(frames) == 0 && !Sheet::is_object(frames)));
}

TEST_CASE("[JRPG][CharacterSheet] Eight directions, a grid of characters, another row order; layouts that do not fit") {
	// 8 rows: the diagonals follow the four sides.
	Ref<SpriteFrames> frames = Sheet::make_frames(sheet(4, 8), Sheet::layout_from(Dictionary({ { "directions", 8 } })));
	REQUIRE(frames.is_valid());
	CHECK(frames->get_animation_names().size() == 8);
	CHECK(region(frames, "up_right", 0) == Rect2(0, 4 * 48, 48, 48));
	CHECK(region(frames, "up_left", 3) == Rect2(3 * 48, 7 * 48, 48, 48));

	// Two characters side by side, three frames each; the second one.
	Sheet::Layout pair;
	pair.frames = 3;
	pair.grid = Vector2i(2, 1);
	pair.index = 1;
	frames = Sheet::make_frames(sheet(6, 4), pair);
	REQUIRE(frames.is_valid());
	CHECK(frames->get_frame_count("down") == 3);
	CHECK(region(frames, "down", 0) == Rect2(3 * 48, 2 * 48, 48, 48));
	CHECK(region(frames, "left", 2) == Rect2(5 * 48, 3 * 48, 48, 48));

	// Rows in another order, with the y offset and object settings carried along.
	frames = Sheet::make_frames(sheet(4, 4), Sheet::layout_from(Dictionary({ { "row_order", "Down, left, right, up" }, { "y_offset", 6 }, { "object", true } })));
	REQUIRE(frames.is_valid());
	CHECK(region(frames, "down", 1) == Rect2(48, 0, 48, 48));
	CHECK(region(frames, "up", 1) == Rect2(48, 3 * 48, 48, 48));
	CHECK((Sheet::get_y_offset(frames) == 6 && Sheet::is_object(frames)));

	// Refused, with the reason.
	String error;
	CHECK(Sheet::make_frames(ImageTexture::create_from_image(Image::create_empty(100, 90, false, Image::FORMAT_RGBA8)), Sheet::Layout(), &error).is_null()); // 90 rows of pixels in 4.
	CHECK(error.contains("does not divide"));
	CHECK(Sheet::make_frames(sheet(4, 4), Sheet::layout_from(Dictionary({ { "row_order", "up,up,down,left" } })), &error).is_null());
	CHECK(error.contains("each of the 4 directions once"));
	CHECK(Sheet::make_frames(sheet(4, 4), Sheet::layout_from(Dictionary({ { "directions", 6 } })), &error).is_null());
	CHECK(Sheet::make_frames(sheet(4, 4), Sheet::layout_from(Dictionary({ { "index", 1 } })), &error).is_null());
	CHECK(error.contains("not in a grid"));
	CHECK(Sheet::make_frames(Ref<Texture2D>(), Sheet::Layout(), &error).is_null());
}

TEST_CASE("[JRPG][CharacterSheet] A character reference loads imported frames as they are, an image with the default layout") {
	const String image_path = TestUtils::get_temp_path("jrpg_character_texture.tres");
	REQUIRE(ResourceSaver::save(sheet(4, 4), image_path) == OK);
	Ref<SpriteFrames> frames = Sheet::load_frames(image_path);
	REQUIRE(frames.is_valid());
	CHECK(region(frames, "right", 3) == Rect2(3 * 48, 48, 48, 48));

	Sheet::Layout tall;
	tall.y_offset = 12;
	const String frames_path = TestUtils::get_temp_path("jrpg_character_frames.res");
	REQUIRE(ResourceSaver::save(Sheet::make_frames(sheet(4, 4), tall), frames_path) == OK);
	frames = Sheet::load_frames(frames_path);
	REQUIRE(frames.is_valid());
	CHECK(Sheet::get_y_offset(frames) == 12);

	String error;
	CHECK(Sheet::load_frames(TestUtils::get_temp_path("jrpg_no_character.res"), &error).is_null());
	CHECK(error.contains("missing"));
}

TEST_CASE("[SceneTree][JRPG][CharacterSprite] The feet stand on the cell; walk frames follow the distance walked at every speed") {
	Ref<JRPGMapData> lane;
	lane.instantiate();
	lane->set_size(Vector2i(8, 1));
	for (int x = 0; x < 8; x++) {
		lane->paint(JRPGMapData::LAYER_GROUND, PackedInt32Array({ x }), PackedInt32Array({ 1 })); // Open without flags.
	}
	const Ref<JRPGMapCellCache> cache = JRPGMapCellCache::compile(lane, Ref<JRPGTilesetData>());

	JRPGCharacterSprite *sprite = memnew(JRPGCharacterSprite);
	SceneTree::get_singleton()->get_root()->add_child(sprite);
	sprite->set_cell_size(Vector2i(48, 48));
	Sheet::Layout raised;
	raised.y_offset = 6;
	sprite->set_character_frames(Sheet::make_frames(sheet(4, 4), raised));
	sprite->show_state(Vector2(), JRPGSystemData::DIRECTION_DOWN, false);
	CHECK(sprite->get_animation() == StringName("down"));
	CHECK(sprite->get_offset() == Vector2(-24, 24 - 48 - 6)); // Bottom edge on the cell's, 6 pixels higher.
	sprite->set_character_frames(Sheet::make_frames(sheet(4, 4), Sheet::Layout()));
	CHECK(sprite->get_offset() == Vector2(-24, -24));

	// The same frame at the same distance, whatever the speed: 1 on the first half cell, then 2, 3, 0.
	for (int speed : { 1, 4, 6 }) {
		Ref<JRPGMover> mover;
		mover.instantiate();
		mover->set_speed(speed);
		mover->set_cell(Vector2i());
		sprite->reset_walk();
		sprite->show_state(mover->get_position(), mover->get_direction(), false);
		const real_t marks[4] = { 0.25, 0.75, 1.25, 1.75 };
		const int expected[4] = { 1, 2, 3, 0 };
		int mark = 0;
		while (mark < 4) {
			if (!mover->is_moving()) {
				REQUIRE(mover->move(JRPGSystemData::DIRECTION_RIGHT, cache));
			}
			mover->advance();
			sprite->show_state(mover->get_position(), mover->get_direction(), mover->is_moving() || mover->get_position().x < 2);
			if (mover->get_position().x >= marks[mark]) {
				CHECK_MESSAGE(sprite->get_frame() == expected[mark], vformat("speed %d at %.2f cells", speed, marks[mark]));
				mark++;
			}
		}
		CHECK(sprite->get_animation() == StringName("right"));
	}
	sprite->show_state(Vector2(2, 0), JRPGSystemData::DIRECTION_UP, false); // Standing still, facing up.
	CHECK((sprite->get_animation() == StringName("up") && sprite->get_frame() == 0 && sprite->get_walked() == 0));
	memdelete(sprite);
}

#ifdef TOOLS_ENABLED
TEST_CASE("[JRPG][CharacterSheet] The Character Sheet importer saves the SpriteFrames of the layout in its options") {
	Ref<JRPGCharacterSheetImporter> importer;
	importer.instantiate();
	CHECK(importer->get_priority() < 1.0); // Images stay textures unless switched to it.
	CHECK(importer->get_resource_type() == "SpriteFrames");
	List<ResourceImporter::ImportOption> options;
	importer->get_import_options(String(), &options);
	HashMap<StringName, Variant> values;
	for (const ResourceImporter::ImportOption &option : options) {
		values[option.option.name] = option.default_value;
	}
	CHECK((int(values["frames"]) == 4 && int(values["directions"]) == 4 && Vector2i(values["grid"]) == Vector2i(1, 1)));

	const String source = TestUtils::get_temp_path("jrpg_character_import.png");
	Ref<Image> image = Image::create_empty(4 * 32, 4 * 40, false, Image::FORMAT_RGBA8);
	image->fill(Color(0.9, 0.5, 0.1));
	REQUIRE(image->save_png(source) == OK);
	values["y_offset"] = 4;
	const String saved = TestUtils::get_temp_path("jrpg_character_import");
	REQUIRE(importer->import(ResourceUID::INVALID_ID, source, saved, values, nullptr) == OK);
	const Ref<SpriteFrames> frames = ResourceLoader::load(saved + ".res", "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(frames.is_valid());
	CHECK(frames->get_animation_names().size() == 4);
	CHECK(region(frames, "left", 2) == Rect2(64, 120, 32, 40));
	CHECK(Sheet::get_y_offset(frames) == 4);

	values["frames"] = 5; // 128 pixels do not divide into 5 frames.
	ERR_PRINT_OFF;
	CHECK(importer->import(ResourceUID::INVALID_ID, source, saved, values, nullptr) == ERR_INVALID_DATA);
	ERR_PRINT_ON;
}
#endif // TOOLS_ENABLED

} // namespace TestJRPGCharacterSheet
