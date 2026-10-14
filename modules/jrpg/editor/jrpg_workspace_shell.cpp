/**************************************************************************/
/*  jrpg_workspace_shell.cpp                                              */
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

#include "jrpg_workspace_shell.h"

#include "jrpg_database_editing.h"
#include "jrpg_database_view.h"
#include "jrpg_editor_file_system.h"
#include "jrpg_managed_addon.h"
#include "jrpg_map_canvas.h"
#include "jrpg_map_editing.h"
#include "jrpg_map_tree.h"
#include "jrpg_tile_palette.h"
#include "jrpg_workspace_view.h"

#include "core/io/image.h"
#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/center_container.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_bar.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/runtime/jrpg_map.h"

JRPGWorkspaceShell *JRPGWorkspaceShell::singleton = nullptr;

namespace {

const char *const SECTION_NAMES[JRPGWorkspaceShell::SECTION_MAX] = { TTRC("Map"), TTRC("Database") }; // Translated when shown.
const char *const SECTION_ICONS[JRPGWorkspaceShell::SECTION_MAX] = { "TileMapLayer", "FileList" };
const char *const MODE_ICONS[JRPGWorkspaceShell::MODE_MAX] = { "TileMapLayer", "Object", "Grid" };
// Editor icons; the ellipse and the shadow pen have none of their own (the round icons are class or widget icons), so
// they are drawn here.
const char *const TOOL_ICONS[JRPGMapEditSession::TOOL_MAX] = { "Edit", "Rectangle", nullptr, "Bucket", nullptr, "Eraser", "ColorPick" };

// A 16 x 16 icon of p_tool (the ellipse, or a cell with one shadowed quarter) in the editor's icon size and text color
// (light and dark themes alike).
Ref<Texture2D> make_tool_icon(int p_tool, const Color &p_color) {
	const String color = p_color.to_html(false);
	const String shape = p_tool == JRPGMapEditSession::TOOL_SHADOW
			? vformat("<rect x=\"2\" y=\"2\" width=\"12\" height=\"12\" fill=\"none\" stroke=\"#%s\" stroke-width=\"2\"/><rect x=\"2\" y=\"8\" width=\"6\" height=\"6\" fill=\"#%s\"/>", color, color)
			: vformat("<ellipse cx=\"8\" cy=\"8\" rx=\"6.5\" ry=\"4.5\" fill=\"none\" stroke=\"#%s\" stroke-width=\"2\"/>", color);
	const String svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\">" + shape + "</svg>";
	Ref<Image> image;
	image.instantiate();
	if (image->load_svg_from_string(svg, EDSCALE) != OK) {
		return Ref<Texture2D>();
	}
	return ImageTexture::create_from_image(image);
}

} // namespace

String JRPGWorkspaceShell::get_section_name(Section p_section) {
	ERR_FAIL_INDEX_V(int(p_section), SECTION_MAX, String());
	return SECTION_NAMES[p_section];
}

JRPGWorkspaceShell::Section JRPGWorkspaceShell::find_section(const String &p_name) {
	for (int i = 0; i < SECTION_MAX; i++) {
		if (p_name.nocasecmp_to(SECTION_NAMES[i]) == 0) {
			return Section(i);
		}
	}
	return SECTION_MAX;
}

void JRPGWorkspaceShell::select_section(Section p_section) {
	ERR_FAIL_INDEX(int(p_section), SECTION_MAX);
	if (tabs->get_current_tab() == p_section) {
		_on_tab_changed(p_section); // Shown again: refreshed once.
	} else {
		tabs->set_current_tab(p_section); // Emits tab_changed, which refreshes it (review finding 33).
	}
}

JRPGWorkspaceShell::Section JRPGWorkspaceShell::get_section() const {
	return Section(tabs->get_current_tab());
}

JRPGWorkspaceShell::MapMode JRPGWorkspaceShell::get_map_mode() const {
	for (int i = 0; i < MODE_MAX; i++) {
		if (mode_buttons[i]->is_pressed()) {
			return MapMode(i);
		}
	}
	return MODE_TILES;
}

void JRPGWorkspaceShell::_on_tab_changed(int p_tab) {
	for (int i = 0; i < SECTION_MAX; i++) {
		sections[i]->set_visible(i == p_tab);
	}
	if (p_tab == SECTION_DATABASE) {
		database_view->refresh();
	}
}

Array JRPGWorkspaceShell::get_database_rows() const {
	Array rows;
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const Ref<JRPGDatabase> db = editing ? editing->get_database() : Ref<JRPGDatabase>();
	if (db.is_null()) {
		return rows;
	}
	for (int kind = 0; kind < JRPGDatabase::TABLE_MAX; kind++) {
		const Ref<JRPGTable> table = db->get_table(JRPGDatabase::TableKind(kind));
		Dictionary row;
		row["name"] = JRPGDatabase::get_table_name(JRPGDatabase::TableKind(kind));
		row["entries"] = table.is_valid() ? table->get_maximum() : 0;
		row["path"] = table.is_valid() ? table->get_path() : String();
		row["unsaved"] = table.is_valid() && editing->is_unsaved(table);
		rows.push_back(row);
	}
	const Ref<Resource> singles[] = { db->get_system(), db->get_types(), db->get_terms() };
	const char *const single_names[] = { "system", "types", "terms" };
	for (int i = 0; i < 3; i++) {
		Dictionary row;
		row["name"] = single_names[i];
		row["entries"] = -1;
		row["path"] = singles[i].is_valid() ? singles[i]->get_path() : String();
		row["unsaved"] = singles[i].is_valid() && editing->is_unsaved(singles[i]);
		rows.push_back(row);
	}
	return rows;
}

void JRPGWorkspaceShell::refresh() {
	if (get_section() == SECTION_DATABASE) {
		database_view->refresh();
	}
	_update_canvas();
	_update_play_button();
}

String JRPGWorkspaceShell::play_blocker(const Ref<JRPGDatabase> &p_database, const Ref<JRPGMapList> &p_map_list, bool p_has_game_scene) {
	const Ref<JRPGSystemData> system = p_database.is_valid() ? p_database->get_system() : Ref<JRPGSystemData>();
	if (system.is_null() || !system->has_start_position()) {
		return TTR("Set the start position first: in the map's Events mode, right-click a cell and choose Set Start Position Here.");
	}
	if (p_map_list.is_null() || !p_map_list->has_map(system->get_start_map_id())) {
		return TTR("The start position is on a map that no longer exists. Set it again on another map.");
	}
	if (!p_has_game_scene) {
		return TTR("The managed addon has no game scene yet. Run Project > Tools > Set Up JRPG Project... to update it.");
	}
	return String();
}

String JRPGWorkspaceShell::get_play_blocker() const {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	return play_blocker(editing ? editing->get_database() : Ref<JRPGDatabase>(), JRPGMapEditing::get_map_list(), !JRPGManagedAddon::find_game_scene().is_empty());
}

Error JRPGWorkspaceShell::play_from_start(String *r_message) {
	String blocker = get_play_blocker();
	if (blocker.is_empty()) {
		// The whole start position now, as the game will check it: the map's scene readable, the cell inside the map
		// (a map can shrink after the start was set; epic 8 review B3). The button's state keeps to the cheap checks.
		JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
		const Ref<JRPGSystemData> system = editing && editing->get_database().is_valid() ? editing->get_database()->get_system() : Ref<JRPGSystemData>();
		String reason;
		if (system.is_valid() && JRPGMapEditing::check_start_position(system->get_start_map_id(), system->get_start_cell(), &reason) != OK) {
			blocker = vformat(TTR("The start position does not work: %s Set it again (in the map's Events mode, Set Start Position Here)."), reason);
		}
	}
	if (!blocker.is_empty()) {
		if (r_message) {
			*r_message = blocker;
		}
		_update_play_button();
		return ERR_UNCONFIGURED;
	}
	EditorInterface::get_singleton()->play_custom_scene(JRPGManagedAddon::find_game_scene());
	if (!EditorInterface::get_singleton()->is_playing_scene()) {
		// The editor refused to run (Recovery Mode, Movie Maker without a movie file: editor/run/editor_run_bar.cpp:258-264,
		// 450-459) and said why itself.
		if (r_message) {
			*r_message = TTR("The editor did not start the game; its own message says why.");
		}
		return ERR_CANT_CREATE;
	}
	return OK;
}

void JRPGWorkspaceShell::_play_pressed() {
	play_from_start();
}

void JRPGWorkspaceShell::_update_play_button() {
	const String blocker = get_play_blocker();
	play_button->set_disabled(!blocker.is_empty());
	play_button->set_tooltip_text(blocker.is_empty() ? TTR("Runs a new game from the start position. The editor saves first, as when running the project.") : blocker);
}

void JRPGWorkspaceShell::_update_canvas() {
	// The edited map in the canvas, or the message while the edited scene is not a map.
	const bool has_map = map_canvas->has_map();
	map_canvas->set_visible(has_map);
	canvas_center->set_visible(!has_map);
	_sync_brush();
}

void JRPGWorkspaceShell::_sync_brush() {
	const Ref<JRPGMapEditSession> session = map_canvas->get_session();
	const JRPGMap *map = session->get_map();
	const JRPGMapEditSession::Mode mode = session->get_mode();
	const bool tiles = map && mode == JRPGMapEditSession::MODE_TILES;
	for (int i = 0; i < JRPGMapEditSession::TOOL_MAX; i++) {
		tool_buttons[i]->set_disabled(!map || !JRPGMapEditSession::is_tool_available(JRPGMapEditSession::Tool(i), mode));
		tool_buttons[i]->set_pressed_no_signal(i == session->get_tool());
	}
	layer_picker->set_visible(mode != JRPGMapEditSession::MODE_REGIONS);
	layer_picker->set_disabled(!tiles);
	layer_picker->select(session->get_layer() + 1); // Auto first.
	region_picker->set_visible(mode == JRPGMapEditSession::MODE_REGIONS);
	region_picker->set_editable(map != nullptr);
	region_picker->set_value_no_signal(session->get_region());
}

void JRPGWorkspaceShell::_mode_toggled(bool p_on, int p_mode) {
	if (p_on && map_canvas) { // The constructor presses Tiles before the canvas exists.
		const Ref<JRPGMapEditSession> session = map_canvas->get_session();
		if (!JRPGMapEditSession::is_tool_available(session->get_tool(), JRPGMapEditSession::Mode(p_mode))) {
			session->set_tool(JRPGMapEditSession::TOOL_PENCIL); // The shadow pen draws on tiles only.
		}
		session->set_mode(JRPGMapEditSession::Mode(p_mode));
	}
}

void JRPGWorkspaceShell::_region_changed(double p_value) {
	map_canvas->get_session()->set_region(int(p_value));
}

void JRPGWorkspaceShell::_tool_toggled(bool p_on, int p_tool) {
	if (p_on && map_canvas) {
		map_canvas->get_session()->set_tool(JRPGMapEditSession::Tool(p_tool));
	}
}

void JRPGWorkspaceShell::_layer_selected(int p_index) {
	map_canvas->get_session()->set_layer(p_index - 1); // Item 0 is Auto.
}

void JRPGWorkspaceShell::_update_theme() {
	for (int i = 0; i < SECTION_MAX; i++) {
		tabs->set_tab_icon(i, get_editor_theme_icon(SECTION_ICONS[i]));
	}
	play_button->set_button_icon(get_editor_theme_icon(SNAME("PlayStart")));
	battle_button->set_button_icon(get_editor_theme_icon(SNAME("PlayScene")));
	for (int i = 0; i < MODE_MAX; i++) {
		mode_buttons[i]->set_button_icon(get_editor_theme_icon(MODE_ICONS[i]));
	}
	for (int i = 0; i < JRPGMapEditSession::TOOL_MAX; i++) {
		tool_buttons[i]->set_button_icon(TOOL_ICONS[i] ? get_editor_theme_icon(TOOL_ICONS[i]) : make_tool_icon(i, get_theme_color(SNAME("font_color"), EditorStringName(Editor))));
	}

	// The canvas is the darker editor surface; the message frame matches the workspace's empty state (the
	// message-window motif of the JRPG tab).
	Ref<StyleBoxFlat> surface;
	surface.instantiate();
	surface->set_bg_color(get_theme_color(SNAME("dark_color_2"), EditorStringName(Editor)));
	canvas->add_theme_style_override(SceneStringName(panel), surface);
	Ref<StyleBoxFlat> frame;
	frame.instantiate();
	frame->set_bg_color(get_theme_color(SNAME("base_color"), EditorStringName(Editor)));
	frame->set_border_color(get_theme_color(SNAME("extra_border_color_1"), EditorStringName(Editor)));
	frame->set_border_width_all(MAX(1, int(EDSCALE)));
	frame->set_corner_radius_all(int(EDITOR_GET("interface/theme/corner_radius")) * EDSCALE);
	frame->set_content_margin_individual(16 * EDSCALE, 12 * EDSCALE, 16 * EDSCALE, 12 * EDSCALE);
	canvas_message->add_theme_style_override(SceneStringName(panel), frame);
	canvas_title->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
}

void JRPGWorkspaceShell::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
		} break;
		case NOTIFICATION_RESIZED: {
			// The message keeps a readable width on wide canvases and fits narrow ones.
			const real_t width = CLAMP(canvas->get_size().x - 32 * EDSCALE, 160 * EDSCALE, 420 * EDSCALE);
			canvas_message->set_custom_minimum_size(Size2(width, 0));
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible_in_tree()) {
				refresh();
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			// Play From Start follows the start position, the map list and the managed addon's files.
			if (JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton()) {
				editing->connect(SNAME("database_changed"), callable_mp(this, &JRPGWorkspaceShell::_update_play_button));
			}
			if (EditorNode::get_singleton()) {
				EditorNode::get_singleton()->connect(SNAME("scene_changed"), callable_mp(this, &JRPGWorkspaceShell::_update_play_button));
			}
			if (jrpg_editor_file_system()) {
				jrpg_editor_file_system()->connect(SNAME("filesystem_changed"), callable_mp(this, &JRPGWorkspaceShell::_update_play_button));
			}
			_update_play_button();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			const Callable update = callable_mp(this, &JRPGWorkspaceShell::_update_play_button);
			JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
			if (editing && editing->is_connected(SNAME("database_changed"), update)) {
				editing->disconnect(SNAME("database_changed"), update);
			}
			if (EditorNode::get_singleton() && EditorNode::get_singleton()->is_connected(SNAME("scene_changed"), update)) {
				EditorNode::get_singleton()->disconnect(SNAME("scene_changed"), update);
			}
			if (jrpg_editor_file_system() && jrpg_editor_file_system()->is_connected(SNAME("filesystem_changed"), update)) {
				jrpg_editor_file_system()->disconnect(SNAME("filesystem_changed"), update);
			}
		} break;
	}
}

JRPGWorkspaceShell::JRPGWorkspaceShell() {
	singleton = this;
	set_name("JRPGWorkspaceShell");
	add_theme_constant_override("separation", 0);

	// Top row: section tabs on the left, playtest on the right (like the editor's own run buttons).
	HBoxContainer *top = memnew(HBoxContainer);
	add_child(top);
	tabs = memnew(TabBar);
	for (int i = 0; i < SECTION_MAX; i++) {
		tabs->add_tab(TTRGET(SECTION_NAMES[i]));
	}
	tabs->set_h_size_flags(SIZE_EXPAND_FILL);
	tabs->set_clip_tabs(false);
	tabs->connect("tab_changed", callable_mp(this, &JRPGWorkspaceShell::_on_tab_changed));
	top->add_child(tabs);
	play_button = memnew(Button);
	play_button->set_text(TTR("Play From Start"));
	play_button->set_theme_type_variation("FlatButton");
	play_button->set_disabled(true); // Until _update_play_button says it can run.
	play_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGWorkspaceShell::_play_pressed));
	top->add_child(play_button);
	battle_button = memnew(Button);
	battle_button->set_text(TTR("Battle Test"));
	battle_button->set_theme_type_variation("FlatButton");
	battle_button->set_disabled(true);
	battle_button->set_tooltip_text(TTR("Runs a test battle against a troop. Available once battles are in place."));
	top->add_child(battle_button);

	// Map section.
	HSplitContainer *map_section = memnew(HSplitContainer);
	map_section->set_v_size_flags(SIZE_EXPAND_FILL);
	map_section->set_split_offset(220 * EDSCALE);
	add_child(map_section);
	sections[SECTION_MAP] = map_section;

	VSplitContainer *side = memnew(VSplitContainer);
	side->set_custom_minimum_size(Size2(160, 0) * EDSCALE);
	map_section->add_child(side);
	map_tree = memnew(JRPGMapTree);
	map_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	side->add_child(map_tree);
	palette = memnew(JRPGTilePalette);
	palette->set_v_size_flags(SIZE_EXPAND_FILL);
	side->add_child(palette);

	VBoxContainer *work = memnew(VBoxContainer);
	work->set_h_size_flags(SIZE_EXPAND_FILL);
	work->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
	map_section->add_child(work);
	HFlowContainer *toolbar = memnew(HFlowContainer); // Wraps on narrow screens instead of clipping.
	work->add_child(toolbar);
	mode_group.instantiate();
	const String mode_names[MODE_MAX] = { TTR("Tiles"), TTR("Events"), TTR("Regions") };
	const String mode_tips[MODE_MAX] = { TTR("Draw the map's tile layers."), TTR("Place and edit events."), TTR("Paint region numbers used by encounters and map rules.") };
	for (int i = 0; i < MODE_MAX; i++) {
		mode_buttons[i] = memnew(Button);
		mode_buttons[i]->set_text(mode_names[i]);
		mode_buttons[i]->set_tooltip_text(mode_tips[i]);
		mode_buttons[i]->set_toggle_mode(true);
		mode_buttons[i]->set_button_group(mode_group);
		mode_buttons[i]->set_theme_type_variation("FlatButton");
		mode_buttons[i]->connect(SceneStringName(toggled), callable_mp(this, &JRPGWorkspaceShell::_mode_toggled).bind(i));
		toolbar->add_child(mode_buttons[i]);
	}
	mode_buttons[MODE_TILES]->set_pressed(true);
	toolbar->add_child(memnew(VSeparator));
	// The drawing tools, one at a time, as the TileMap editor's (editor/scene/2d/tiles/tile_map_layer_editor.cpp:2207-2251).
	tool_group.instantiate();
	const String tool_names[JRPGMapEditSession::TOOL_MAX] = { TTR("Pencil"), TTR("Rectangle"), TTR("Ellipse"), TTR("Fill"), TTR("Shadow Pen"), TTR("Eraser"), TTR("Eyedropper") };
	const String tool_tips[JRPGMapEditSession::TOOL_MAX] = { TTR("Draw where you drag."), TTR("Draw a filled rectangle: drag from one corner to the other."), TTR("Draw a filled ellipse inside the rectangle you drag."), TTR("Fill the area of matching cells you click."), TTR("Darken a quarter of a cell, or clear it if it is dark; drag to go on. Tiles only."), TTR("Erase where you drag."), TTR("Pick what the cell you click holds, to draw with it (right-click does the same with any tool).") };
	for (int i = 0; i < JRPGMapEditSession::TOOL_MAX; i++) {
		tool_buttons[i] = memnew(Button);
		tool_buttons[i]->set_tooltip_text(tool_names[i] + "\n" + tool_tips[i]);
		tool_buttons[i]->set_accessibility_name(tool_names[i]);
		tool_buttons[i]->set_theme_type_variation("FlatButton");
		tool_buttons[i]->set_toggle_mode(true);
		tool_buttons[i]->set_button_group(tool_group);
		tool_buttons[i]->set_disabled(true);
		tool_buttons[i]->connect(SceneStringName(toggled), callable_mp(this, &JRPGWorkspaceShell::_tool_toggled).bind(i));
		toolbar->add_child(tool_buttons[i]);
	}
	toolbar->add_child(memnew(VSeparator));
	layer_picker = memnew(OptionButton);
	layer_picker->set_flat(true);
	const String layer_names[JRPGMapData::LAYER_COUNT + 1] = { TTR("Auto"), TTR("Ground"), TTR("Ground Detail"), TTR("Objects"), TTR("Upper") };
	for (const String &name : layer_names) {
		layer_picker->add_item(name);
	}
	layer_picker->set_tooltip_text(TTR("The tile layer the tools draw on, from the ground up. Auto puts an autotile or wall on the ground, or over it on Ground Detail where the ground is another tile, and stacks other tiles on the two upper layers, at most two per cell."));
	layer_picker->set_accessibility_name(TTR("Layer"));
	layer_picker->set_disabled(true);
	layer_picker->connect(SceneStringName(item_selected), callable_mp(this, &JRPGWorkspaceShell::_layer_selected));
	toolbar->add_child(layer_picker);
	region_picker = memnew(SpinBox);
	region_picker->set_min(1);
	region_picker->set_max(255);
	region_picker->set_prefix(TTR("Region"));
	region_picker->set_custom_minimum_size(Size2(120, 0) * EDSCALE); // The prefix and three digits.
	region_picker->set_tooltip_text(TTR("The region ID the tools paint, 1-255. Regions limit encounters and map rules to areas."));
	region_picker->set_accessibility_name(TTR("Region"));
	region_picker->set_visible(false);
	region_picker->connect(SceneStringName(value_changed), callable_mp(this, &JRPGWorkspaceShell::_region_changed));
	toolbar->add_child(region_picker);

	PanelContainer *canvas_panel = memnew(PanelContainer);
	canvas_panel->set_v_size_flags(SIZE_EXPAND_FILL);
	canvas = canvas_panel;
	work->add_child(canvas_panel);
	map_canvas = memnew(JRPGMapCanvas);
	map_canvas->set_visible(false);
	map_canvas->connect(SNAME("map_changed"), callable_mp(this, &JRPGWorkspaceShell::_update_canvas));
	map_canvas->get_session()->connect(SNAME("brush_changed"), callable_mp(this, &JRPGWorkspaceShell::_sync_brush));
	palette->set_session(map_canvas->get_session());
	canvas_panel->add_child(map_canvas);
	canvas_center = memnew(CenterContainer);
	CenterContainer *center = canvas_center;
	canvas_panel->add_child(center);
	canvas_message = memnew(PanelContainer);
	canvas_message->set_custom_minimum_size(Size2(280, 0) * EDSCALE); // Updated on resize.
	center->add_child(canvas_message);
	VBoxContainer *message_box = memnew(VBoxContainer);
	canvas_message->add_child(message_box);
	canvas_title = memnew(Label);
	canvas_title->set_text(TTR("No map is open"));
	message_box->add_child(canvas_title);
	canvas_body = memnew(Label);
	canvas_body->set_text(TTR("Double-click a map in the Maps list to open it, or create one with New Map."));
	canvas_body->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	message_box->add_child(canvas_body);
	canvas_mark = memnew(JRPGMessageMark);
	message_box->add_child(canvas_mark);

	database_view = memnew(JRPGDatabaseView);
	database_view->set_v_size_flags(SIZE_EXPAND_FILL);
	database_view->set_visible(false);
	add_child(database_view);
	sections[SECTION_DATABASE] = database_view;
	_update_canvas();
}

JRPGWorkspaceShell::~JRPGWorkspaceShell() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
