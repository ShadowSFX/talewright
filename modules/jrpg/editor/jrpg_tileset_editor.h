/**************************************************************************/
/*  jrpg_tileset_editor.h                                                 */
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

#include "jrpg_autotile_importer.h"
#include "jrpg_tile_flags.h"

#include "editor/inspector/editor_inspector.h"
#include "scene/gui/dialogs.h"

class Button;
class ButtonGroup;
class EditorFileDialog;
class HBoxContainer;
class ItemList;
class Label;
class LineEdit;
class OptionButton;
class SpinBox;
class Texture2D;
class TileSet;
class VBoxContainer;

// Add Sheet: how to read an image chosen for a tileset (the JRPGAutotileImporter layouts), with the size each layout
// needs at the TileSet's tile size. A ConfirmationDialog with a form grid, like the editor's resource dialogs; it stays
// open with the reason when the import is refused.
class JRPGTileSheetDialog : public ConfirmationDialog {
	GDCLASS(JRPGTileSheetDialog, ConfirmationDialog);

	int tileset_id = 0;
	String image_path;
	Vector2i image_size;
	Vector2i tile_size;
	Label *image_label = nullptr;
	OptionButton *kind = nullptr;
	SpinBox *frames = nullptr;
	LineEdit *name = nullptr;
	Label *layout = nullptr;
	Label *error = nullptr;

	void _update();
	void _confirmed();

protected:
	void _notification(int p_what);

public:
	// Opens the dialog for the image at p_path (p_image_size pixels), guessing the layout from its size
	// (JRPGAutotileImporter::guess_kind).
	void open(int p_tileset_id, const String &p_path, const Vector2i &p_image_size, const Vector2i &p_tile_size);

	JRPGTileSheetDialog();
};

// The tileset form's tiles (JRPGTilesetData.palette, spec 05; task 7.3.2.2): the palette as a grid of tile thumbnails
// with one tile flag drawn over each, edited by clicking. Follows the editor's own patterns: an EditorProperty with a
// full-width bottom editor (editor/inspector/editor_properties_array_dict.cpp), flat toggle buttons in one group for
// the mode (editor/scene/2d/tiles/tile_map_layer_editor.cpp:2207-2251), an icon-mode ItemList as the palette
// (tile_map_layer_editor.cpp:474,2477-2478) and values drawn over tiles as the TileSet editor paints custom data
// (editor/scene/2d/tiles/tile_data_editors.cpp:1272-1329). Every click is one undoable action through
// JRPGDatabaseEditing::set_tile_flags; Enter acts on the selected tile and Shift+Enter cycles back.
class JRPGEditorPropertyTiles : public EditorProperty {
	GDCLASS(JRPGEditorPropertyTiles, EditorProperty);

	static constexpr int SIDES = 4; // North, east, south, west: bits 1, 2, 4, 8 of a side mask.
	static inline int last_mode = JRPGTileFlags::FIELD_PASS; // A new form starts in the mode last chosen in any.
	int mode = last_mode; // Each form has its own (the workspace and the Inspector dock may show one each).

	VBoxContainer *container = nullptr;
	VBoxContainer *no_tileset = nullptr;
	Label *no_tileset_label = nullptr;
	Button *create_button = nullptr;
	VBoxContainer *tiles = nullptr;
	Button *mode_buttons[JRPGTileFlags::FIELD_MAX] = {};
	Ref<ButtonGroup> mode_group;
	Label *hint = nullptr;
	ItemList *list = nullptr;
	Label *empty = nullptr;
	HBoxContainer *sides_row = nullptr;
	Label *sides_label = nullptr;
	Button *side_buttons[SIDES] = {};
	Button *add_button = nullptr;
	EditorFileDialog *file_dialog = nullptr;
	JRPGTileSheetDialog *sheet_dialog = nullptr;

	Ref<TileSet> tileset;
	Array palette;
	String cached_uid;
	ObjectID cached_data; // The edited tileset, for is_cache_valid (get_edited_object is not const).
	Ref<Texture2D> placeholder;
	bool refresh_queued = false;

	int _tileset_id();
	JRPGTileFlags::Field _mode() const { return JRPGTileFlags::Field(mode); }
	bool _is_side_mode() const;
	void _set_tileset(const Ref<TileSet> &p_tileset);
	void _tileset_changed();
	void _refresh();
	Dictionary _entry(int p_item) const;
	Rect2 _tile_rect(int p_item);
	int _selected() const;
	void _draw_overlays();
	void _mode_toggled(bool p_on, int p_mode);
	void _update_mode();
	void _update_sides();
	void _item_clicked(int p_item, const Vector2 &p_at, MouseButton p_button);
	void _list_input(const Ref<InputEvent> &p_event);
	void _act(int p_item, int p_side, bool p_back);
	void _side_toggled(bool p_on, int p_side);
	void _apply(int p_index, int p_mask, int p_value);
	void _create_tileset();
	void _add_sheet();
	void _file_selected(const String &p_path);

protected:
	void _notification(int p_what);
	void _set_read_only(bool p_read_only) override;

public:
	void update_property() override;
	void update_cache() override;
	bool is_cache_valid() const override;

	// What a click does in mode p_mode to a tile whose flag word is p_word: p_side picks a side in the side modes (-1:
	// the middle, all four), p_back cycles backward. Returns the field's new value (not shifted).
	static int next_value(JRPGTileFlags::Field p_mode, int p_word, int p_side, bool p_back);
	// Which part of a tile a click at p_at falls on: 0..3 for the north, east, south and west edges, -1 for the middle.
	static int side_at(const Rect2 &p_tile, const Vector2 &p_at);
	// Where ItemList draws an icon of p_icon_size in an item of top-icon mode whose rectangle (get_item_rect without
	// expansion) is p_item_rect, scrolled by p_scroll, in a list p_width wide laid out left to right or right to left.
	static Rect2 icon_rect(const Rect2 &p_item_rect, const Size2 &p_icon_size, int p_v_separation, float p_scroll, float p_width, bool p_rtl);
	// Whether a palette entry's shapes carry different flags (painted one by one in Godot's TileSet editor).
	static bool is_mixed(const Ref<TileSet> &p_tileset, const Dictionary &p_entry);
	// The image of palette entry p_entry in p_tileset: a kind shows its first shape (an isolated patch for autotiles and
	// walls); null when the TileSet does not have the tile.
	static Ref<Texture2D> make_thumbnail(const Ref<TileSet> &p_tileset, const Dictionary &p_entry);

	JRPGEditorPropertyTiles();
};
