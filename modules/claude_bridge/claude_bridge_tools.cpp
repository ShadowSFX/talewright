/**************************************************************************/
/*  claude_bridge_tools.cpp                                               */
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

#include "claude_bridge_tools.h"

#include "claude_bridge_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/variant/variant_utility.h"
#include "core/version.h"
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "scene/main/node.h"

namespace ClaudeBridgeTools {

static constexpr int MAX_DEPTH = 6;
static constexpr int MAX_ITEMS = 256;

Variant variant_to_json(const Variant &p_value, int p_depth) {
	switch (p_value.get_type()) {
		case Variant::NIL:
		case Variant::BOOL:
		case Variant::INT:
		case Variant::STRING:
			return p_value;
		case Variant::FLOAT: {
			const double d = p_value;
			return Math::is_finite(d) ? p_value : Variant(VariantUtilityFunctions::var_to_str(p_value));
		}
		case Variant::STRING_NAME:
		case Variant::NODE_PATH:
			return String(p_value);
		case Variant::ARRAY: {
			const Array src = p_value;
			Array out;
			if (p_depth >= MAX_DEPTH) {
				return vformat("<Array of %d>", src.size());
			}
			for (int i = 0; i < MIN(src.size(), MAX_ITEMS); i++) {
				out.push_back(variant_to_json(src[i], p_depth + 1));
			}
			if (src.size() > MAX_ITEMS) {
				out.push_back(vformat("<%d more>", src.size() - MAX_ITEMS));
			}
			return out;
		}
		case Variant::DICTIONARY: {
			const Dictionary src = p_value;
			if (p_depth >= MAX_DEPTH) {
				return vformat("<Dictionary of %d>", src.size());
			}
			Dictionary out;
			int n = 0;
			for (const KeyValue<Variant, Variant> &kv : src) {
				if (n++ >= MAX_ITEMS) {
					out["<truncated>"] = src.size() - MAX_ITEMS;
					break;
				}
				out[String(kv.key)] = variant_to_json(kv.value, p_depth + 1);
			}
			return out;
		}
		case Variant::OBJECT: {
			Object *obj = p_value.get_validated_object();
			if (!obj) {
				return Variant();
			}
			Dictionary d;
			d["class"] = obj->get_class();
			if (const Resource *res = Object::cast_to<Resource>(obj)) {
				d["resource_path"] = res->get_path();
			} else if (const Node *node = Object::cast_to<Node>(obj)) {
				d["node_name"] = node->get_name();
			}
			return d;
		}
		default: {
			// Packed arrays and math types: Godot literal text, truncated when large.
			String text = VariantUtilityFunctions::var_to_str(p_value);
			if (text.length() > 4096) {
				text = text.substr(0, 4096) + "...<truncated>";
			}
			return text;
		}
	}
}

Variant json_to_value(const Variant &p_json, Variant::Type p_target, bool &r_ok) {
	r_ok = true;
	if (p_target == Variant::NIL || p_json.get_type() == p_target) {
		return p_json;
	}
	if (p_json.get_type() == Variant::STRING && p_target != Variant::STRING && p_target != Variant::STRING_NAME && p_target != Variant::NODE_PATH) {
		const Variant parsed = VariantUtilityFunctions::str_to_var(p_json);
		if (parsed.get_type() == p_target) {
			return parsed;
		}
		if (parsed.get_type() != Variant::NIL && Variant::can_convert_strict(parsed.get_type(), p_target)) {
			return VariantUtilityFunctions::type_convert(parsed, p_target);
		}
	}
	if (p_target == Variant::INT && p_json.get_type() == Variant::FLOAT) {
		return (int64_t)Math::round((double)p_json);
	}
	if (Variant::can_convert_strict(p_json.get_type(), p_target)) {
		return VariantUtilityFunctions::type_convert(p_json, p_target);
	}
	r_ok = false;
	return Variant();
}

String validate_res_path(const String &p_path) {
	String path = p_path.strip_edges();
	if (path.is_empty()) {
		return String();
	}
	if (!path.begins_with("res://")) {
		path = "res://" + path.trim_prefix("/");
	}
	const String rel = path.substr(6).simplify_path();
	if (rel.begins_with("..") || rel.contains("/../") || rel.contains(":")) {
		return String();
	}
	return "res://" + rel;
}

// --- helpers -------------------------------------------------------------------------------------------

static Node *_edited_root() {
	return EditorInterface::get_singleton()->get_edited_scene_root();
}

static Node *_resolve_node(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call) {
	Node *root = _edited_root();
	if (!root) {
		p_call->fail("No scene is open in the editor. Use editor_open_scene first.");
		return nullptr;
	}
	const String path = String(p_args.get("path", "")).strip_edges();
	if (path.is_empty() || path == "." || path == "/") {
		return root;
	}
	Node *node = root->get_node_or_null(NodePath(path));
	if (!node) {
		p_call->fail("Node not found (paths are relative to the edited scene root): " + path);
	}
	return node;
}

static Dictionary _node_summary(Node *p_node, Node *p_root, int p_depth, int p_max_depth, int &r_count, int p_max_count) {
	Dictionary d;
	d["name"] = p_node->get_name();
	d["type"] = p_node->get_class();
	d["path"] = p_node == p_root ? String(".") : String(p_root->get_path_to(p_node));
	Ref<Resource> script = p_node->get_script();
	if (script.is_valid()) {
		d["script"] = script->get_path();
	}
	if (!p_node->get_scene_file_path().is_empty() && p_node != p_root) {
		d["instance_of"] = p_node->get_scene_file_path();
	}
	const int child_count = p_node->get_child_count();
	if (child_count > 0) {
		if (p_depth >= p_max_depth || r_count >= p_max_count) {
			d["children_omitted"] = child_count;
		} else {
			Array children;
			for (int i = 0; i < child_count; i++) {
				if (r_count >= p_max_count) {
					d["children_omitted"] = child_count - i;
					break;
				}
				r_count++;
				children.push_back(_node_summary(p_node->get_child(i), p_root, p_depth + 1, p_max_depth, r_count, p_max_count));
			}
			d["children"] = children;
		}
	}
	return d;
}

static bool _is_editor_property(const PropertyInfo &p_info) {
	return (p_info.usage & PROPERTY_USAGE_EDITOR) && !(p_info.usage & (PROPERTY_USAGE_CATEGORY | PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP));
}

// --- tools ---------------------------------------------------------------------------------------------

static void _tool_scene_tree(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	Node *root = _edited_root();
	Dictionary out;
	PackedStringArray open;
	for (const String &s : EditorInterface::get_singleton()->get_open_scenes()) {
		open.push_back(s);
	}
	out["open_scenes"] = open;
	if (!root) {
		out["edited_scene"] = Variant();
		p_call->complete_data(out);
		return;
	}
	Node *start = _resolve_node(p_args, p_call);
	if (!start) {
		return;
	}
	int count = 1;
	const int max_depth = CLAMP(int(p_args.get("depth", 8)), 0, 64);
	const int max_nodes = CLAMP(int(p_args.get("max_nodes", 2000)), 1, 20000);
	out["edited_scene"] = root->get_scene_file_path();
	out["tree"] = _node_summary(start, root, 0, max_depth, count, max_nodes);
	out["node_count"] = count;
	p_call->complete_data(out);
}

static void _tool_node_get(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	Node *node = _resolve_node(p_args, p_call);
	if (!node) {
		return;
	}
	const Array wanted = p_args.get("properties", Array());
	Dictionary props;
	List<PropertyInfo> list;
	node->get_property_list(&list);
	for (const PropertyInfo &pi : list) {
		if (!_is_editor_property(pi)) {
			continue;
		}
		if (!wanted.is_empty() && !wanted.has(String(pi.name))) {
			continue;
		}
		props[pi.name] = variant_to_json(node->get(pi.name));
	}
	Dictionary out;
	out["path"] = String(p_args.get("path", "."));
	out["class"] = node->get_class();
	out["properties"] = props;
	p_call->complete_data(out);
}

static void _tool_node_set(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	Node *node = _resolve_node(p_args, p_call);
	if (!node) {
		return;
	}
	const StringName property = String(p_args.get("property", ""));
	List<PropertyInfo> list;
	node->get_property_list(&list);
	const PropertyInfo *info = nullptr;
	for (const PropertyInfo &pi : list) {
		if (pi.name == property && _is_editor_property(pi)) {
			info = &pi;
			break;
		}
	}
	if (!info) {
		p_call->fail("Unknown or non-editable property: " + String(property));
		return;
	}
	bool ok = false;
	const Variant value = json_to_value(p_args.get("value", Variant()), info->type, ok);
	if (!ok) {
		p_call->fail(vformat("Cannot convert the value to %s; use a Godot literal string such as \"Vector2(1, 2)\".", Variant::get_type_name(info->type)));
		return;
	}
	const Variant old = node->get(property);
	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action(vformat("Claude bridge: set %s.%s", node->get_name(), property));
	ur->add_do_property(node, property, value);
	ur->add_undo_property(node, property, old);
	ur->commit_action();
	Dictionary out;
	out["property"] = String(property);
	out["old_value"] = variant_to_json(old);
	out["new_value"] = variant_to_json(node->get(property));
	out["undoable"] = true;
	p_call->complete_data(out);
}

static void _tool_open_scene(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	const String path = validate_res_path(p_args.get("path", ""));
	if (path.is_empty() || !ResourceLoader::exists(path)) {
		p_call->fail("Scene not found: " + String(p_args.get("path", "")));
		return;
	}
	EditorInterface::get_singleton()->open_scene_from_path(path);
	Node *root = _edited_root();
	Dictionary out;
	out["requested"] = path;
	out["edited_scene"] = root ? root->get_scene_file_path() : String();
	p_call->complete_data(out);
}

static void _tool_save_scene(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	Node *root = _edited_root();
	if (!root) {
		p_call->fail("No scene is open.");
		return;
	}
	const Error err = EditorInterface::get_singleton()->save_scene();
	if (err != OK) {
		p_call->fail(vformat("Saving failed (error %d). New scenes need a path: save them once from the editor.", err));
		return;
	}
	Dictionary out;
	out["saved"] = root->get_scene_file_path();
	p_call->complete_data(out);
}

static void _tool_select(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	Node *root = _edited_root();
	if (!root) {
		p_call->fail("No scene is open.");
		return;
	}
	EditorSelection *selection = EditorInterface::get_singleton()->get_selection();
	selection->clear();
	const Array paths = p_args.get("paths", Array());
	Array selected;
	Node *first = nullptr;
	for (int i = 0; i < paths.size(); i++) {
		const String p = String(paths[i]).strip_edges();
		Node *n = (p.is_empty() || p == ".") ? root : root->get_node_or_null(NodePath(p));
		if (!n) {
			p_call->fail("Node not found: " + p);
			return;
		}
		selection->add_node(n);
		selected.push_back(p.is_empty() ? String(".") : p);
		first = first ? first : n;
	}
	if (first) {
		EditorInterface::get_singleton()->edit_node(first);
	}
	Dictionary out;
	out["selected"] = selected;
	p_call->complete_data(out);
}

static void _tool_project_info(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	Dictionary out;
	out["name"] = GLOBAL_GET("application/config/name");
	out["main_scene"] = GLOBAL_GET("application/run/main_scene");
	out["features"] = variant_to_json(GLOBAL_GET("application/config/features"));
	out["rendering_method"] = GLOBAL_GET("rendering/renderer/rendering_method");
	out["project_path"] = ps->globalize_path("res://");
	out["engine"] = GODOT_VERSION_FULL_CONFIG;
	Node *root = _edited_root();
	out["edited_scene"] = root ? root->get_scene_file_path() : String();
	out["is_playing"] = EditorInterface::get_singleton()->is_playing_scene();
	p_call->complete_data(out);
}

static void _tool_project_settings(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	const String prefix = p_args.get("prefix", "");
	const int limit = CLAMP(int(p_args.get("limit", 200)), 1, 2000);
	List<PropertyInfo> list;
	ProjectSettings::get_singleton()->get_property_list(&list);
	Dictionary values;
	int n = 0;
	for (const PropertyInfo &pi : list) {
		if (pi.usage & (PROPERTY_USAGE_CATEGORY | PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP)) {
			continue;
		}
		if (!prefix.is_empty() && !pi.name.begins_with(prefix)) {
			continue;
		}
		if (n++ >= limit) {
			break;
		}
		values[pi.name] = variant_to_json(ProjectSettings::get_singleton()->get(pi.name));
	}
	Dictionary out;
	out["prefix"] = prefix;
	out["count"] = values.size();
	out["settings"] = values;
	p_call->complete_data(out);
}

static void _list_dir(const String &p_dir, bool p_recursive, const PackedStringArray &p_ext, PackedStringArray &r_out, int p_limit) {
	Ref<DirAccess> da = DirAccess::open(p_dir);
	if (da.is_null()) {
		return;
	}
	da->list_dir_begin();
	for (String f = da->get_next(); !f.is_empty() && r_out.size() < p_limit; f = da->get_next()) {
		if (f.begins_with(".")) {
			continue; // Skips ".godot", ".git", "." and "..".
		}
		const String full = p_dir.path_join(f);
		if (da->current_is_dir()) {
			if (p_recursive) {
				_list_dir(full, true, p_ext, r_out, p_limit);
			}
		} else if (p_ext.is_empty() || p_ext.has(f.get_extension().to_lower())) {
			r_out.push_back(full);
		}
	}
	da->list_dir_end();
}

static void _tool_list_files(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	const String dir = validate_res_path(p_args.get("dir", "res://"));
	if (dir.is_empty()) {
		p_call->fail("Invalid directory (must be inside res://).");
		return;
	}
	PackedStringArray ext;
	const Array exts = p_args.get("extensions", Array());
	for (int i = 0; i < exts.size(); i++) {
		ext.push_back(String(exts[i]).trim_prefix(".").to_lower());
	}
	const int limit = CLAMP(int(p_args.get("limit", 500)), 1, 10000);
	PackedStringArray files;
	_list_dir(dir, bool(p_args.get("recursive", true)), ext, files, limit);
	Dictionary out;
	out["dir"] = dir;
	out["files"] = files;
	out["truncated"] = files.size() >= limit;
	p_call->complete_data(out);
}

static void _tool_read_file(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	const String path = validate_res_path(p_args.get("path", ""));
	if (path.is_empty() || !FileAccess::exists(path)) {
		p_call->fail("File not found (must be inside res://): " + String(p_args.get("path", "")));
		return;
	}
	Ref<FileAccess> f = FileAccess::open(path, FileAccess::READ);
	if (f.is_null()) {
		p_call->fail("Cannot open " + path);
		return;
	}
	const int64_t max_bytes = CLAMP(int64_t(p_args.get("max_bytes", 262144)), 1, 4 * 1024 * 1024);
	const uint64_t size = f->get_length();
	Vector<uint8_t> data = f->get_buffer(MIN((int64_t)size, max_bytes));
	String text;
	Dictionary out;
	out["path"] = path;
	out["size"] = (int64_t)size;
	out["truncated"] = (int64_t)size > max_bytes;
	if (text.append_utf8((const char *)data.ptr(), data.size()) == OK) {
		out["text"] = text;
	} else {
		out["binary"] = true;
	}
	p_call->complete_data(out);
}

static void _tool_rescan(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	efs->scan_changes();
	Dictionary out;
	out["scanning"] = efs->is_scanning();
	p_call->complete_data(out);
}

static void _tool_undo_redo(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *) {
	const String action = p_args.get("action", "undo");
	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	bool ok = false;
	if (action == "undo") {
		ok = ur->undo();
	} else if (action == "redo") {
		ok = ur->redo();
	} else {
		p_call->fail("action must be \"undo\" or \"redo\".");
		return;
	}
	Dictionary out;
	out["action"] = action;
	out["performed"] = ok;
	p_call->complete_data(out);
}

// --- registration --------------------------------------------------------------------------------------

static Dictionary _prop(const String &p_type, const String &p_description) {
	Dictionary d;
	d["type"] = p_type;
	d["description"] = p_description;
	return d;
}

static Dictionary _array_prop(const String &p_item_type, const String &p_description) {
	Dictionary items;
	items["type"] = p_item_type;
	Dictionary d;
	d["type"] = "array";
	d["items"] = items;
	d["description"] = p_description;
	return d;
}

static void _add(ClaudeBridgeMCP &r_mcp, const String &p_name, const String &p_title, const String &p_description, const Dictionary &p_properties, const PackedStringArray &p_required, ClaudeBridgeMCP::ToolFunc p_func, bool p_read_only, ClaudeBridgePlugin *p_plugin) {
	ClaudeBridgeMCP::Tool t;
	t.name = p_name;
	t.title = p_title;
	t.description = p_description;
	t.input_schema = ClaudeBridgeMCP::object_schema(p_properties, p_required);
	t.read_only = p_read_only;
	t.func = p_func;
	t.userdata = p_plugin;
	r_mcp.register_tool(t);
}

void register_editor_tools(ClaudeBridgeMCP &r_mcp, ClaudeBridgePlugin *p_plugin) {
	Dictionary p;
	p["path"] = _prop("string", "Node path relative to the edited scene root; \".\" or empty for the root.");
	p["depth"] = _prop("integer", "Maximum depth below the start node (default 8).");
	p["max_nodes"] = _prop("integer", "Maximum nodes returned (default 2000).");
	_add(r_mcp, "editor_scene_tree", "Edited scene tree", "Returns the node tree of the scene open in the editor (names, types, paths, scripts, instanced scenes) and the list of open scenes.", p, PackedStringArray(), &_tool_scene_tree, true, p_plugin);

	p.clear();
	p["path"] = _prop("string", "Node path relative to the edited scene root.");
	p["properties"] = _array_prop("string", "Optional property names; default: every editor-visible property.");
	_add(r_mcp, "editor_node_get", "Get node properties", "Returns editor-visible properties of a node in the edited scene. Non-JSON values are Godot literal strings such as \"Vector2(1, 2)\".", p, PackedStringArray(), &_tool_node_get, true, p_plugin);

	p.clear();
	p["path"] = _prop("string", "Node path relative to the edited scene root.");
	p["property"] = _prop("string", "Property name, e.g. \"position\".");
	Dictionary any;
	any["description"] = "New value: JSON value or a Godot literal string such as \"Vector2(10, 20)\" or \"Color(1, 0, 0, 1)\".";
	p["value"] = any;
	PackedStringArray req;
	req.push_back("path");
	req.push_back("property");
	req.push_back("value");
	_add(r_mcp, "editor_node_set", "Set node property", "Sets a property of a node in the edited scene as an undoable editor action.", p, req, &_tool_node_set, false, p_plugin);

	p.clear();
	p["path"] = _prop("string", "Scene path, e.g. res://main.tscn.");
	req.clear();
	req.push_back("path");
	_add(r_mcp, "editor_open_scene", "Open scene", "Opens a scene in the editor (or switches to its tab).", p, req, &_tool_open_scene, false, p_plugin);

	_add(r_mcp, "editor_save_scene", "Save scene", "Saves the scene currently edited.", Dictionary(), PackedStringArray(), &_tool_save_scene, false, p_plugin);

	p.clear();
	p["paths"] = _array_prop("string", "Node paths to select (relative to the edited scene root).");
	_add(r_mcp, "editor_select", "Select nodes", "Replaces the editor selection and shows the first node in the inspector.", p, PackedStringArray(), &_tool_select, false, p_plugin);

	_add(r_mcp, "project_info", "Project info", "Project name, main scene, features, renderer, path, engine version, edited scene and whether the game is running.", Dictionary(), PackedStringArray(), &_tool_project_info, true, p_plugin);

	p.clear();
	p["prefix"] = _prop("string", "Only settings whose name starts with this prefix, e.g. \"application/\" or \"input/\".");
	p["limit"] = _prop("integer", "Maximum number of settings (default 200).");
	_add(r_mcp, "project_settings_get", "Project settings", "Reads project settings (values as JSON or Godot literal strings).", p, PackedStringArray(), &_tool_project_settings, true, p_plugin);

	p.clear();
	p["dir"] = _prop("string", "Directory inside res:// (default res://).");
	p["recursive"] = _prop("boolean", "Recurse into subdirectories (default true).");
	p["extensions"] = _array_prop("string", "Optional extensions filter, e.g. [\"tscn\", \"gd\"].");
	p["limit"] = _prop("integer", "Maximum number of files (default 500).");
	_add(r_mcp, "project_list_files", "List project files", "Lists files in the project (hidden directories such as .godot are skipped).", p, PackedStringArray(), &_tool_list_files, true, p_plugin);

	p.clear();
	p["path"] = _prop("string", "File path inside res://.");
	p["max_bytes"] = _prop("integer", "Maximum bytes to read (default 262144).");
	req.clear();
	req.push_back("path");
	_add(r_mcp, "project_read_file", "Read project file", "Reads a project file as UTF-8 text (binary files report only their size).", p, req, &_tool_read_file, true, p_plugin);

	_add(r_mcp, "editor_rescan", "Rescan filesystem", "Asks the editor to rescan the project files (the editor otherwise rescans only when it regains focus).", Dictionary(), PackedStringArray(), &_tool_rescan, false, p_plugin);

	p.clear();
	p["action"] = _prop("string", "\"undo\" or \"redo\".");
	req.clear();
	req.push_back("action");
	_add(r_mcp, "editor_undo_redo", "Undo/redo", "Undoes or redoes the last editor action.", p, req, &_tool_undo_redo, false, p_plugin);
}

} // namespace ClaudeBridgeTools
