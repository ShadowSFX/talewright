/**************************************************************************/
/*  jrpg_character_sheet_importer.cpp                                     */
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

#include "jrpg_character_sheet_importer.h"

#include "core/io/image.h"
#include "core/io/image_loader.h"
#include "core/io/resource_saver.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/sprite_frames.h"

#include "modules/jrpg/runtime/jrpg_character_sheet.h"

String JRPGCharacterSheetImporter::get_importer_name() const {
	return IMPORTER_NAME;
}

String JRPGCharacterSheetImporter::get_visible_name() const {
	return "Character Sheet";
}

void JRPGCharacterSheetImporter::get_recognized_extensions(List<String> *p_extensions) const {
	ImageLoader::get_recognized_extensions(p_extensions);
}

String JRPGCharacterSheetImporter::get_save_extension() const {
	return "res";
}

String JRPGCharacterSheetImporter::get_resource_type() const {
	return "SpriteFrames";
}

float JRPGCharacterSheetImporter::get_priority() const {
	return 0.1; // Below the texture importer's 1.0: chosen per file, never by default.
}

void JRPGCharacterSheetImporter::get_import_options(const String &p_path, List<ImportOption> *r_options, int p_preset) const {
	const JRPGCharacterSheet::Layout layout;
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "frames", PROPERTY_HINT_RANGE, "1,64"), layout.frames));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "directions", PROPERTY_HINT_ENUM, "4:4,8:8"), layout.directions));
	r_options->push_back(ImportOption(PropertyInfo(Variant::VECTOR2I, "grid"), layout.grid));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "index", PROPERTY_HINT_RANGE, "0,63"), layout.index));
	r_options->push_back(ImportOption(PropertyInfo(Variant::STRING, "row_order", PROPERTY_HINT_PLACEHOLDER_TEXT, "up,right,down,left"), layout.row_order));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "y_offset", PROPERTY_HINT_RANGE, "-256,256,1,suffix:px"), layout.y_offset));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "object"), layout.object));
}

bool JRPGCharacterSheetImporter::get_option_visibility(const String &p_path, const String &p_option, const HashMap<StringName, Variant> &p_options) const {
	return true;
}

Error JRPGCharacterSheetImporter::import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	Ref<Image> image;
	image.instantiate();
	const Error loaded = ImageLoader::load_image(p_source_file, image);
	ERR_FAIL_COND_V_MSG(loaded != OK, loaded, vformat("Character sheet %s: the image cannot be read.", p_source_file));
	Dictionary options;
	for (const KeyValue<StringName, Variant> &option : p_options) {
		options[option.key] = option.value;
	}
	String error;
	const Ref<SpriteFrames> frames = JRPGCharacterSheet::make_frames(ImageTexture::create_from_image(image), JRPGCharacterSheet::layout_from(options), &error);
	ERR_FAIL_COND_V_MSG(frames.is_null(), ERR_INVALID_DATA, vformat("Character sheet %s: %s Change the layout in the Import dock.", p_source_file, error));
	return ResourceSaver::save(frames, p_save_path + "." + get_save_extension());
}
