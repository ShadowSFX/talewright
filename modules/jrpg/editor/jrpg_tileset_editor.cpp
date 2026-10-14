/**************************************************************************/
/*  jrpg_tileset_editor.cpp                                               */
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

#include "jrpg_tileset_editor.h"

#include "jrpg_database_editing.h"

#include "core/input/input_event.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/spin_box.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/image_texture.h"

#include "modules/jrpg/core/jrpg_autotile.h"
#include "modules/jrpg/core/jrpg_entry.h"
#include "modules/jrpg/core/jrpg_misc_data.h"

namespace {

// What each mode asks of the user; also the legend of what the overlay shows.
const char *MODE_HINTS[JRPGTileFlags::FIELD_MAX] = {
	TTRC("Click a tile to cycle it through open (ring), blocked (cross) and transparent (broken ring: the tile below decides). Right-click cycles back."),
	TTRC("Click near an edge of a tile to stop characters entering it from that side, or the middle for all four sides."),
	TTRC("Click near an edge of a tile to stop characters leaving it toward that side, or the middle for all four sides."),
	TTRC("Click a tile to draw it above characters. Overhead tiles never block."),
	TTRC("Click a tile to make it a ladder: characters on it climb, facing up."),
	TTRC("Click a tile to make it a bush: it hides the lower part of characters standing in it."),
	TTRC("Click a tile to make it a counter: characters can talk across it."),
	TTRC("Click a tile to make it hurt the party at every step."),
	TTRC("Click a tile to raise its terrain tag, right-click to lower it (0 to 15). Events and scripts can read the tag."),
};

const char *SIDE_NAMES[] = { TTRC("North"), TTRC("East"), TTRC("South"), TTRC("West") };

// A note at the bottom of the editor, or the log without one (tests).
void toast(const String &p_message, EditorToaster::Severity p_severity = EditorToaster::SEVERITY_INFO) {
	if (EditorToaster::get_singleton()) {
		EditorToaster::get_singleton()->popup_str(p_message, p_severity);
	} else if (p_severity == EditorToaster::SEVERITY_ERROR) {
		ERR_PRINT(p_message);
	} else {
		print_line(p_message);
	}
}

String kind_label(const String &p_kind) {
	if (p_kind == "autotile") {
		return TTR("Autotile");
	}
	if (p_kind == "wall") {
		return TTR("Wall");
	}
	return TTR("Plain Tile");
}

} // namespace

// JRPGTileSheetDialog

JRPGAutotileImporter::SheetKind JRPGTileSheetDialog::guess_kind(const Vector2i &p_cells, int &r_frames) {
	r_frames = 1;
	if (p_cells.y == JRPGAutotile::BLOB_ROWS && p_cells.x > 0 && p_cells.x % JRPGAutotile::BLOB_COLUMNS == 0) {
		r_frames = p_cells.x / JRPGAutotile::BLOB_COLUMNS;
		return JRPGAutotileImporter::SHEET_AUTOTILE;
	}
	// 8 x 4 tiles may be a wall with two frames or 32 plain tiles; plain sheets are the likelier.
	if (p_cells == Vector2i(JRPGAutotile::WALL_COLUMNS, JRPGAutotile::WALL_ROWS)) {
		return JRPGAutotileImporter::SHEET_WALL;
	}
	return JRPGAutotileImporter::SHEET_PLAIN;
}

void JRPGTileSheetDialog::open(int p_tileset_id, const String &p_path, const Vector2i &p_image_size, const Vector2i &p_tile_size) {
	tileset_id = p_tileset_id;
	image_path = p_path;
	image_size = p_image_size;
	tile_size = p_tile_size;
	image_label->set_text(vformat(TTR("%s (%d x %d pixels)"), p_path.get_file(), p_image_size.x, p_image_size.y));
	int guessed_frames = 1;
	const Vector2i cells = tile_size.x > 0 && tile_size.y > 0 ? image_size / tile_size : Vector2i();
	kind->select(guess_kind(cells, guessed_frames));
	frames->set_value(guessed_frames);
	name->set_text(p_path.get_file().get_basename().capitalize());
	error->hide();
	_update();
	popup_centered(Size2(420, 0) * EDSCALE);
	name->grab_focus();
	name->select_all();
}

void JRPGTileSheetDialog::_update() {
	const JRPGAutotileImporter::SheetKind sheet = JRPGAutotileImporter::SheetKind(kind->get_selected());
	frames->set_editable(sheet != JRPGAutotileImporter::SHEET_PLAIN);
	if (sheet == JRPGAutotileImporter::SHEET_PLAIN) {
		frames->set_value(1);
	}
	const int count = int(frames->get_value());
	bool fits = tile_size.x > 0 && tile_size.y > 0 && image_size.x % tile_size.x == 0 && image_size.y % tile_size.y == 0;
	String text;
	if (sheet == JRPGAutotileImporter::SHEET_PLAIN) {
		text = vformat(TTR("Each %d x %d tile of the image that shows anything becomes a tile of its own."), tile_size.x, tile_size.y);
	} else {
		const bool blob = sheet == JRPGAutotileImporter::SHEET_AUTOTILE;
		const Vector2i block = blob ? Vector2i(JRPGAutotile::BLOB_COLUMNS, JRPGAutotile::BLOB_ROWS) : Vector2i(JRPGAutotile::WALL_COLUMNS, JRPGAutotile::WALL_ROWS);
		const Vector2i needed(block.x * count * tile_size.x, block.y * tile_size.y);
		fits = fits && image_size == needed;
		text = vformat(blob ? TTR("An autotile sheet is %d x %d tiles for each frame, frames side by side: %d x %d pixels here.") : TTR("A wall sheet is %d x %d tiles for each frame, frames side by side: %d x %d pixels here."), block.x, block.y, needed.x, needed.y);
	}
	if (!fits) {
		text += "\n" + vformat(TTR("This image is %d x %d pixels, so it cannot be read this way."), image_size.x, image_size.y);
	}
	layout->set_text(text);
	layout->add_theme_color_override(SceneStringName(font_color), get_theme_color(fits ? SNAME("font_readonly_color") : SNAME("warning_color"), EditorStringName(Editor)));
	get_ok_button()->set_disabled(!fits || name->get_text().strip_edges().is_empty());
}

void JRPGTileSheetDialog::_confirmed() {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	ERR_FAIL_NULL(editing);
	PackedInt32Array indexes;
	String message;
	const Error err = editing->add_tile_sheet(tileset_id, image_path, JRPGAutotileImporter::SheetKind(kind->get_selected()), int(frames->get_value()), name->get_text(), indexes, &message);
	if (err != OK) {
		error->set_text(message);
		error->show();
		return;
	}
	hide();
}

void JRPGTileSheetDialog::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		error->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
	}
}

JRPGTileSheetDialog::JRPGTileSheetDialog() {
	set_title(TTR("Add Sheet"));
	set_ok_button_text(TTR("Add Sheet"));
	set_hide_on_ok(false);
	connect(SceneStringName(confirmed), callable_mp(this, &JRPGTileSheetDialog::_confirmed));

	VBoxContainer *box = memnew(VBoxContainer);
	add_child(box);
	GridContainer *grid = memnew(GridContainer);
	grid->set_columns(2);
	box->add_child(grid);
	const auto row = [&](const String &p_label, Control *p_field) {
		Label *label = memnew(Label);
		label->set_text(p_label);
		grid->add_child(label);
		p_field->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		grid->add_child(p_field);
		p_field->set_accessibility_name(p_label);
	};

	image_label = memnew(Label);
	image_label->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	image_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	row(TTR("Image"), image_label);
	kind = memnew(OptionButton);
	kind->add_item(TTR("Plain Tiles"), JRPGAutotileImporter::SHEET_PLAIN);
	kind->add_item(TTR("Autotile"), JRPGAutotileImporter::SHEET_AUTOTILE);
	kind->add_item(TTR("Wall"), JRPGAutotileImporter::SHEET_WALL);
	kind->connect(SceneStringName(item_selected), callable_mp(this, &JRPGTileSheetDialog::_update).unbind(1));
	row(TTR("Kind"), kind);
	frames = memnew(SpinBox);
	frames->set_min(1);
	frames->set_max(16);
	frames->set_tooltip_text(TTR("Animated autotiles and walls hold their frames side by side."));
	frames->connect(SceneStringName(value_changed), callable_mp(this, &JRPGTileSheetDialog::_update).unbind(1));
	row(TTR("Frames"), frames);
	name = memnew(LineEdit);
	name->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	name->connect(SceneStringName(text_changed), callable_mp(this, &JRPGTileSheetDialog::_update).unbind(1));
	register_text_enter(name);
	row(TTR("Name"), name);

	layout = memnew(Label);
	layout->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	layout->set_custom_minimum_size(Size2(360, 0) * EDSCALE);
	box->add_child(layout);
	error = memnew(Label);
	error->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	error->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED); // The database's message.
	error->hide();
	box->add_child(error);
}

// JRPGEditorPropertyTiles

int JRPGEditorPropertyTiles::next_value(JRPGTileFlags::Field p_mode, int p_word, int p_side, bool p_back) {
	const JRPGTileFlags::FieldInfo &field = JRPGTileFlags::get_field(p_mode);
	const int value = (p_word & field.mask) >> field.shift;
	if (p_mode == JRPGTileFlags::FIELD_NO_ENTRY || p_mode == JRPGTileFlags::FIELD_NO_EXIT) {
		if (p_side < 0 || p_side >= SIDES) {
			return value == field.max ? 0 : field.max; // The middle: all four sides, or none when all are set.
		}
		return value ^ (1 << p_side);
	}
	return (value + (p_back ? field.max : 1)) % (field.max + 1);
}

int JRPGEditorPropertyTiles::side_at(const Rect2 &p_tile, const Vector2 &p_at) {
	if (p_tile.size.x <= 0 || p_tile.size.y <= 0) {
		return -1;
	}
	const Vector2 rel = (p_at - p_tile.get_center()) / p_tile.size; // -0.5 .. 0.5 inside the tile.
	if (MAX(Math::abs(rel.x), Math::abs(rel.y)) < 0.2f) {
		return -1;
	}
	if (Math::abs(rel.y) >= Math::abs(rel.x)) {
		return rel.y < 0 ? 0 : 2;
	}
	return rel.x > 0 ? 1 : 3;
}

int JRPGEditorPropertyTiles::_tileset_id() {
	const JRPGEntry *entry = Object::cast_to<JRPGEntry>(get_edited_object());
	return entry ? entry->get_id() : 0;
}

bool JRPGEditorPropertyTiles::_is_side_mode() const {
	return _mode() == JRPGTileFlags::FIELD_NO_ENTRY || _mode() == JRPGTileFlags::FIELD_NO_EXIT;
}

void JRPGEditorPropertyTiles::_set_tileset(const Ref<TileSet> &p_tileset) {
	if (tileset == p_tileset) {
		return;
	}
	const Callable changed = callable_mp(this, &JRPGEditorPropertyTiles::_tileset_changed);
	if (tileset.is_valid() && tileset->is_connected(CoreStringName(changed), changed)) {
		tileset->disconnect(CoreStringName(changed), changed);
	}
	tileset = p_tileset;
	if (tileset.is_valid()) {
		tileset->connect(CoreStringName(changed), changed);
	}
}

void JRPGEditorPropertyTiles::_tileset_changed() {
	// Every tile flag written emits it (one click writes nine layers on up to 47 shapes): refresh once, afterwards.
	if (!refresh_queued) {
		refresh_queued = true;
		callable_mp(this, &JRPGEditorPropertyTiles::_refresh).call_deferred();
	}
}

void JRPGEditorPropertyTiles::_refresh() {
	refresh_queued = false;
	list->queue_redraw();
	_update_sides();
}

Dictionary JRPGEditorPropertyTiles::_entry(int p_item) const {
	return p_item >= 0 && p_item < palette.size() ? Dictionary(palette[p_item]) : Dictionary();
}

Ref<Texture2D> JRPGEditorPropertyTiles::_thumbnail(const Dictionary &p_entry) const {
	const int source_id = p_entry.get("source_id", TileSet::INVALID_SOURCE);
	if (tileset.is_null() || !tileset->has_source(source_id) || !JRPGTileFlags::has_entry_tile(tileset, p_entry)) {
		return Ref<Texture2D>();
	}
	const Ref<TileSetAtlasSource> atlas = tileset->get_source(source_id);
	if (atlas.is_null() || atlas->get_texture().is_null()) {
		return Ref<Texture2D>();
	}
	// A kind shows its first shape, the one its flags are read from (an isolated patch for autotiles and walls).
	Ref<AtlasTexture> thumbnail;
	thumbnail.instantiate();
	thumbnail->set_atlas(atlas->get_texture());
	thumbnail->set_region(atlas->get_tile_texture_region(JRPGTileFlags::get_entry_tiles(p_entry)[0], 0));
	return thumbnail;
}

Rect2 JRPGEditorPropertyTiles::_tile_rect(int p_item) {
	// Where ItemList draws the icon of an item in top-icon mode (scene/gui/item_list.cpp:1463-1468,1578-1592).
	Rect2 rect = list->get_item_rect(p_item);
	rect.position.y -= int(list->get_v_scroll_bar()->get_value());
	const Size2 icon = list->get_fixed_icon_size() * list->get_icon_scale();
	Point2 pos = rect.position;
	pos.y += MAX(list->get_theme_constant(SNAME("v_separation")), 0) / 2;
	pos.x += Math::floor((rect.size.width - icon.width) / 2);
	return Rect2(pos, icon);
}

int JRPGEditorPropertyTiles::_selected() const {
	const PackedInt32Array items = list->get_selected_items();
	return items.is_empty() ? -1 : items[0];
}

void JRPGEditorPropertyTiles::_draw_overlays() {
	if (tileset.is_null()) {
		return;
	}
	const JRPGTileFlags::Field mode = _mode();
	const JRPGTileFlags::FieldInfo &field = JRPGTileFlags::get_field(mode);
	// A dark halo under every mark keeps it readable on any tile art, as the TileSet editor outlines its numbers.
	const Color halo(0, 0, 0, 0.7);
	const Color open = get_theme_color(SNAME("success_color"), EditorStringName(Editor));
	const Color blocked = get_theme_color(SNAME("error_color"), EditorStringName(Editor));
	const Color transparent = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
	const Color dim = get_theme_color(SNAME("font_readonly_color"), EditorStringName(Editor));
	const Ref<Texture2D> on_icon = get_editor_theme_icon(SNAME("TileChecked"));
	const Ref<Texture2D> off_icon = get_editor_theme_icon(SNAME("TileUnchecked"));
	const Ref<Texture2D> warning = get_editor_theme_icon(SNAME("StatusWarning"));
	const Ref<Font> font = get_theme_font(SNAME("bold"), EditorStringName(EditorFonts));
	const int font_size = get_theme_font_size(SNAME("bold_size"), EditorStringName(EditorFonts));
	for (int i = 0; i < list->get_item_count(); i++) {
		const Rect2 rect = _tile_rect(i);
		const Vector2 center = rect.get_center();
		const float size = MIN(rect.size.x, rect.size.y);
		const Dictionary entry = _entry(i);
		if (!JRPGTileFlags::has_entry_tile(tileset, entry)) {
			list->draw_texture(warning, (center - warning->get_size() / 2).round());
			continue;
		}
		const int value = (JRPGTileFlags::get_entry_flags(tileset, entry) & field.mask) >> field.shift;
		const float line = MAX(2.0f, Math::round(size * 0.07f));
		switch (mode) {
			case JRPGTileFlags::FIELD_PASS: {
				const float radius = size * 0.24f;
				if (value == JRPGTilesetData::PASS_BLOCKED) {
					const Vector2 a(radius, radius);
					const Vector2 b(radius, -radius);
					list->draw_line(center - a, center + a, halo, line + 2 * EDSCALE, true);
					list->draw_line(center - b, center + b, halo, line + 2 * EDSCALE, true);
					list->draw_line(center - a, center + a, blocked, line, true);
					list->draw_line(center - b, center + b, blocked, line, true);
				} else if (value == JRPGTilesetData::PASS_TRANSPARENT) {
					for (int k = 0; k < 8; k++) {
						const float from = k * Math::TAU / 8;
						list->draw_arc(center, radius, from, from + Math::TAU / 14, 6, halo, line + 2 * EDSCALE, true);
						list->draw_arc(center, radius, from, from + Math::TAU / 14, 6, transparent, line, true);
					}
				} else {
					list->draw_arc(center, radius, 0, Math::TAU, 32, halo, line + 2 * EDSCALE, true);
					list->draw_arc(center, radius, 0, Math::TAU, 32, open, line, true);
				}
			} break;
			case JRPGTileFlags::FIELD_NO_ENTRY:
			case JRPGTileFlags::FIELD_NO_EXIT: {
				const float bar = MAX(3.0f, Math::round(size * 0.1f));
				const Rect2 bars[SIDES] = {
					Rect2(rect.position, Size2(rect.size.x, bar)),
					Rect2(rect.position + Vector2(rect.size.x - bar, 0), Size2(bar, rect.size.y)),
					Rect2(rect.position + Vector2(0, rect.size.y - bar), Size2(rect.size.x, bar)),
					Rect2(rect.position, Size2(bar, rect.size.y)),
				};
				for (int side = 0; side < SIDES; side++) {
					if (value & (1 << side)) {
						list->draw_rect(bars[side].grow(EDSCALE), halo);
						list->draw_rect(bars[side], blocked);
					}
				}
			} break;
			case JRPGTileFlags::FIELD_TERRAIN_TAG: {
				const String text = itos(value);
				const Vector2 number_size = font->get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
				const Vector2 pos = (center + Vector2(-number_size.x / 2, number_size.y / 4)).round();
				list->draw_string_outline(font, pos, text, HORIZONTAL_ALIGNMENT_CENTER, number_size.x, font_size, 3 * EDSCALE, Color(0, 0, 0));
				list->draw_string(font, pos, text, HORIZONTAL_ALIGNMENT_CENTER, number_size.x, font_size, value == 0 ? dim : Color(1, 1, 1));
			} break;
			default: {
				const float mark = Math::round(size / 3);
				list->draw_texture_rect(value ? on_icon : off_icon, Rect2((center - Vector2(mark, mark) / 2).round(), Size2(mark, mark)));
			} break;
		}
	}
}

void JRPGEditorPropertyTiles::_mode_toggled(bool p_on, int p_mode) {
	if (p_on) {
		last_mode = p_mode;
		_update_mode();
	}
}

void JRPGEditorPropertyTiles::_update_mode() {
	if (mode_buttons[last_mode] && !mode_buttons[last_mode]->is_pressed()) {
		mode_buttons[last_mode]->set_pressed_no_signal(true);
	}
	hint->set_text(TTRGET(MODE_HINTS[last_mode]));
	sides_label->set_text(_mode() == JRPGTileFlags::FIELD_NO_EXIT ? TTR("Block Leaving Toward:") : TTR("Block Entering From:"));
	sides_row->set_visible(_is_side_mode());
	_update_sides();
	list->queue_redraw();
}

void JRPGEditorPropertyTiles::_update_sides() {
	if (!_is_side_mode()) {
		return;
	}
	const int item = _selected();
	const Dictionary entry = _entry(item);
	const bool usable = tileset.is_valid() && item >= 0 && JRPGTileFlags::has_entry_tile(tileset, entry) && !is_read_only();
	const JRPGTileFlags::FieldInfo &field = JRPGTileFlags::get_field(_mode());
	const int value = usable ? (JRPGTileFlags::get_entry_flags(tileset, entry) & field.mask) >> field.shift : 0;
	for (int side = 0; side < SIDES; side++) {
		side_buttons[side]->set_disabled(!usable);
		side_buttons[side]->set_pressed_no_signal(value & (1 << side));
	}
}

void JRPGEditorPropertyTiles::_item_clicked(int p_item, const Vector2 &p_at, MouseButton p_button) {
	if (p_button != MouseButton::LEFT && p_button != MouseButton::RIGHT) {
		return;
	}
	_act(p_item, _is_side_mode() ? side_at(_tile_rect(p_item), p_at) : -1, p_button == MouseButton::RIGHT);
}

void JRPGEditorPropertyTiles::_list_input(const Ref<InputEvent> &p_event) {
	// Enter acts on the selected tile instead of ItemList's item_activated (Control emits gui_input before handling the
	// event, so accepting it here stops the list: scene/main/viewport.cpp, Control::_call_gui_input).
	if (p_event->is_action_pressed(SNAME("ui_accept"), false, false) && _selected() >= 0) {
		const Ref<InputEventWithModifiers> modifiers = p_event;
		_act(_selected(), -1, modifiers.is_valid() && modifiers->is_shift_pressed());
		list->accept_event();
	}
}

void JRPGEditorPropertyTiles::_act(int p_item, int p_side, bool p_back) {
	const Dictionary entry = _entry(p_item);
	if (is_read_only() || tileset.is_null() || entry.is_empty() || !JRPGTileFlags::has_entry_tile(tileset, entry)) {
		return;
	}
	const JRPGTileFlags::FieldInfo &field = JRPGTileFlags::get_field(_mode());
	const int value = next_value(_mode(), JRPGTileFlags::get_entry_flags(tileset, entry), p_side, p_back);
	_apply(p_item + 1, field.mask, value << field.shift);
}

void JRPGEditorPropertyTiles::_side_toggled(bool p_on, int p_side) {
	const int item = _selected();
	if (item < 0) {
		return;
	}
	const int bit = (1 << p_side) << JRPGTileFlags::get_field(_mode()).shift;
	_apply(item + 1, bit, p_on ? bit : 0);
}

void JRPGEditorPropertyTiles::_apply(int p_index, int p_mask, int p_value) {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	ERR_FAIL_NULL(editing);
	String message;
	if (editing->set_tile_flags(_tileset_id(), PackedInt32Array({ p_index }), p_mask, p_value, &message) != OK) {
		toast(message, EditorToaster::SEVERITY_ERROR);
	}
	_tileset_changed();
}

void JRPGEditorPropertyTiles::_create_tileset() {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const JRPGEntry *entry = Object::cast_to<JRPGEntry>(get_edited_object());
	ERR_FAIL_COND(!editing || !entry);
	String base = entry->get_entry_name().to_snake_case().validate_filename();
	if (base.is_empty()) {
		base = vformat("tileset_%d", entry->get_id());
	}
	String path = "res://jrpg/tilesets/" + base + ".tres";
	for (int n = 2; FileAccess::exists(path); n++) {
		path = vformat("res://jrpg/tilesets/%s_%d.tres", base, n);
	}
	String message;
	if (editing->create_tileset(entry->get_id(), path, DEFAULT_TILE_SIZE, &message) != OK) {
		toast(message, EditorToaster::SEVERITY_ERROR);
		return;
	}
	toast(vformat(TTR("Created the TileSet %s."), path));
	update_property();
}

void JRPGEditorPropertyTiles::_add_sheet() {
	if (!file_dialog) {
		// Made on first use, as EditorPropertyPath does (editor/inspector/editor_properties.cpp:723-724).
		file_dialog = memnew(EditorFileDialog);
		file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
		file_dialog->set_access(EditorFileDialog::ACCESS_RESOURCES);
		file_dialog->set_title(TTR("Choose a Tile Sheet"));
		List<String> extensions;
		ResourceLoader::get_recognized_extensions_for_type("Texture2D", &extensions);
		for (const String &extension : extensions) {
			file_dialog->add_filter("*." + extension);
		}
		file_dialog->connect("file_selected", callable_mp(this, &JRPGEditorPropertyTiles::_file_selected));
		add_child(file_dialog);
	}
	file_dialog->popup_file_dialog();
}

void JRPGEditorPropertyTiles::_file_selected(const String &p_path) {
	const Ref<Texture2D> texture = ResourceLoader::exists(p_path) ? ResourceLoader::load(p_path) : Ref<Resource>();
	if (texture.is_null() || tileset.is_null()) {
		toast(vformat(TTR("%s is not an image the project has imported."), p_path), EditorToaster::SEVERITY_ERROR);
		return;
	}
	sheet_dialog->open(_tileset_id(), p_path, texture->get_size(), tileset->get_tile_size());
}

void JRPGEditorPropertyTiles::_set_read_only(bool p_read_only) {
	create_button->set_disabled(p_read_only);
	add_button->set_disabled(p_read_only);
	_update_sides();
}

void JRPGEditorPropertyTiles::update_property() {
	const Ref<JRPGTilesetData> tileset_data = Object::cast_to<JRPGTilesetData>(get_edited_object());
	_set_tileset(JRPGTileFlags::load_tileset(tileset_data));
	no_tileset->set_visible(tileset.is_null());
	tiles->set_visible(tileset.is_valid());
	if (tileset.is_null()) {
		palette.clear();
		list->clear();
		const bool named = tileset_data.is_valid() && !tileset_data->get_tileset_uid().is_empty();
		no_tileset_label->set_text(named ? TTR("The TileSet chosen above could not be loaded. Choose another one, or create a new TileSet for this tileset.") : TTR("This tileset has no TileSet yet. Create one, or choose an existing TileSet above."));
		return;
	}
	palette = get_edited_property_value();
	const int was_selected = _selected();
	list->clear();
	const Vector2i tile = tileset->get_tile_size();
	// Pixel art at a whole zoom near 48 editor pixels, unfiltered.
	const int zoom = MAX(1, int(Math::floor(DEFAULT_TILE_SIZE * EDSCALE / MAX(1, tile.y))));
	list->set_fixed_icon_size(tile * zoom);
	if (placeholder.is_null() || placeholder->get_size() != Size2(tile)) {
		Ref<Image> image = Image::create_empty(MAX(1, tile.x), MAX(1, tile.y), false, Image::FORMAT_RGBA8);
		image->fill(Color(0.5, 0.5, 0.5, 0.15));
		placeholder = ImageTexture::create_from_image(image);
	}
	for (int i = 0; i < palette.size(); i++) {
		const Dictionary entry = palette[i];
		const Ref<Texture2D> thumbnail = _thumbnail(entry);
		const int item = list->add_icon_item(thumbnail.is_valid() ? thumbnail : placeholder);
		String tooltip = vformat(TTR("Tile %d: %s (%s)"), i + 1, String(entry.get("name", "")), kind_label(entry.get("kind", "plain")));
		if (thumbnail.is_null()) {
			tooltip += "\n" + TTR("Its tile is missing from the TileSet.");
		}
		list->set_item_tooltip(item, tooltip);
	}
	if (was_selected >= 0 && was_selected < list->get_item_count()) {
		list->select(was_selected);
	}
	list->set_visible(!palette.is_empty());
	empty->set_visible(palette.is_empty());
	_update_mode();
}

void JRPGEditorPropertyTiles::update_cache() {
	EditorProperty::update_cache();
	const JRPGTilesetData *tileset_data = Object::cast_to<JRPGTilesetData>(get_edited_object());
	cached_data = tileset_data ? tileset_data->get_instance_id() : ObjectID();
	cached_uid = tileset_data ? tileset_data->get_tileset_uid() : String();
}

bool JRPGEditorPropertyTiles::is_cache_valid() const {
	// Choosing another TileSet in the field above changes what this shows, though the palette stays the same.
	const JRPGTilesetData *tileset_data = ObjectDB::get_instance<JRPGTilesetData>(cached_data);
	return EditorProperty::is_cache_valid() && (tileset_data ? tileset_data->get_tileset_uid() : String()) == cached_uid;
}

void JRPGEditorPropertyTiles::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			create_button->set_button_icon(get_editor_theme_icon(SNAME("TileSet")));
			add_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
			const Color readonly = get_theme_color(SNAME("font_readonly_color"), EditorStringName(Editor));
			for (Label *text : { no_tileset_label, hint, empty }) {
				text->add_theme_color_override(SceneStringName(font_color), readonly);
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_set_tileset(Ref<TileSet>());
		} break;
	}
}

JRPGEditorPropertyTiles::JRPGEditorPropertyTiles() {
	container = memnew(VBoxContainer);
	add_child(container);
	set_bottom_editor(container);

	// Without a TileSet: say so and offer to make one.
	no_tileset = memnew(VBoxContainer);
	container->add_child(no_tileset);
	no_tileset_label = memnew(Label);
	no_tileset_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	no_tileset_label->set_custom_minimum_size(Size2(120, 0) * EDSCALE);
	no_tileset->add_child(no_tileset_label);
	create_button = memnew(Button);
	create_button->set_text(TTR("Create TileSet"));
	create_button->set_tooltip_text(vformat(TTR("Creates a TileSet of %d x %d pixel tiles in res://jrpg/tilesets/ and uses it here."), DEFAULT_TILE_SIZE, DEFAULT_TILE_SIZE));
	create_button->set_h_size_flags(SIZE_SHRINK_BEGIN);
	create_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyTiles::_create_tileset));
	no_tileset->add_child(create_button);
	add_focusable(create_button);

	tiles = memnew(VBoxContainer);
	container->add_child(tiles);

	HFlowContainer *modes = memnew(HFlowContainer);
	tiles->add_child(modes);
	mode_group.instantiate();
	for (int i = 0; i < JRPGTileFlags::FIELD_MAX; i++) {
		Button *button = memnew(Button);
		button->set_text(TTRGET(JRPGTileFlags::get_field(JRPGTileFlags::Field(i)).label));
		button->set_toggle_mode(true);
		button->set_button_group(mode_group);
		button->set_theme_type_variation(SceneStringName(FlatButton));
		button->connect(SceneStringName(toggled), callable_mp(this, &JRPGEditorPropertyTiles::_mode_toggled).bind(i));
		modes->add_child(button);
		add_focusable(button);
		mode_buttons[i] = button;
	}

	hint = memnew(Label);
	hint->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	hint->set_custom_minimum_size(Size2(120, 0) * EDSCALE);
	tiles->add_child(hint);

	list = memnew(ItemList);
	list->set_icon_mode(ItemList::ICON_MODE_TOP);
	list->set_max_columns(0);
	list->set_same_column_width(true);
	list->set_auto_height(true);
	list->set_allow_rmb_select(true);
	list->set_allow_reselect(true);
	list->set_texture_filter(TEXTURE_FILTER_NEAREST);
	list->set_clip_contents(true);
	list->set_accessibility_name(TTR("Tiles"));
	list->connect(SceneStringName(draw), callable_mp(this, &JRPGEditorPropertyTiles::_draw_overlays));
	list->connect(SNAME("item_clicked"), callable_mp(this, &JRPGEditorPropertyTiles::_item_clicked));
	list->connect(SceneStringName(item_selected), callable_mp(this, &JRPGEditorPropertyTiles::_update_sides).unbind(1));
	list->connect(SceneStringName(gui_input), callable_mp(this, &JRPGEditorPropertyTiles::_list_input));
	tiles->add_child(list);
	add_focusable(list);

	empty = memnew(Label);
	empty->set_text(TTR("No tiles yet. Add a sheet to bring its tiles into this tileset."));
	empty->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	empty->set_custom_minimum_size(Size2(120, 0) * EDSCALE);
	tiles->add_child(empty);

	// Keyboard access to the sides of the selected tile: one check box per bit, as the inspector shows flags
	// (EditorPropertyFlags, editor/inspector/editor_properties.cpp:1066).
	sides_row = memnew(HBoxContainer);
	tiles->add_child(sides_row);
	sides_label = memnew(Label);
	sides_row->add_child(sides_label);
	for (int side = 0; side < SIDES; side++) {
		CheckBox *button = memnew(CheckBox);
		button->set_text(TTRGET(SIDE_NAMES[side]));
		button->connect(SceneStringName(toggled), callable_mp(this, &JRPGEditorPropertyTiles::_side_toggled).bind(side));
		sides_row->add_child(button);
		add_focusable(button);
		side_buttons[side] = button;
	}

	add_button = memnew(Button);
	add_button->set_text(TTR("Add Sheet..."));
	add_button->set_tooltip_text(TTR("Brings the tiles of an image into this tileset: plain tiles, an autotile or a wall."));
	add_button->set_h_size_flags(SIZE_SHRINK_BEGIN);
	add_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyTiles::_add_sheet));
	tiles->add_child(add_button);
	add_focusable(add_button);

	sheet_dialog = memnew(JRPGTileSheetDialog);
	add_child(sheet_dialog);
}
