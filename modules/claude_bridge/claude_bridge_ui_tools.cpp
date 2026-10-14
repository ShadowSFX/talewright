/**************************************************************************/
/*  claude_bridge_ui_tools.cpp                                            */
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

#include "claude_bridge_ui_tools.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/inspector/editor_inspector.h"
#include "scene/gui/base_button.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/tree.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace ClaudeBridgeUI {

namespace {

constexpr int MAX_ITEMS = 40; // Items listed per list, tree, tab bar or menu.
constexpr int MAX_TEXT = 300;
constexpr int MAX_VISITED = 200000; // Nodes walked per search.

String clip(const String &p_text) {
	return p_text.length() > MAX_TEXT ? p_text.substr(0, MAX_TEXT) + "..." : p_text;
}

// A viewport that takes no input, such as the one showing the edited scene (editor/editor_node.cpp:8871): a user
// cannot click into it, so neither can the tools (review finding 2). Windows are viewports that take input.
bool input_disabled(const Node *p_node) {
	const Viewport *vp = Object::cast_to<Viewport>(p_node);
	return vp && !Object::cast_to<Window>(p_node) && vp->is_input_disabled();
}

// A node is reachable when a user can get to it: its CanvasItem ancestors up to its window and every window up to
// p_root are shown (windows are separate surfaces: a dialog stays visible under a hidden control), no viewport on the
// way takes no input, and no window on the way has an open modal dialog other than the one the node is in: Godot
// gives a window with an exclusive child no input (scene/main/window.cpp:2000-2002; review finding 6).
bool reachable(Node *p_root, Node *p_node) {
	bool in_own_window = true;
	const Window *from = nullptr; // The window the walk came up through.
	for (Node *n = p_node; n; n = n->get_parent()) {
		if (Window *w = Object::cast_to<Window>(n)) {
			if (!w->is_visible()) {
				return false;
			}
			const Window *modal = w->get_exclusive_child();
			if (modal && modal->is_visible() && modal != from) {
				return false;
			}
			from = w;
			in_own_window = false;
		} else if (in_own_window) {
			const CanvasItem *ci = Object::cast_to<CanvasItem>(n);
			if (ci && !ci->is_visible()) {
				return false;
			}
		}
		if (input_disabled(n)) {
			return false;
		}
		if (n == p_root) {
			return true;
		}
	}
	return false; // Outside p_root.
}

struct TreeEntry {
	TreeItem *item = nullptr;
	PackedStringArray columns;
};

Vector<TreeEntry> tree_entries(Tree *p_tree) {
	Vector<TreeEntry> out;
	TreeItem *item = p_tree->get_root();
	if (item && p_tree->is_root_hidden()) {
		item = item->get_next_in_tree();
	}
	for (; item; item = item->get_next_in_tree()) {
		if (!item->is_visible_in_tree()) {
			continue;
		}
		TreeEntry e;
		e.item = item;
		for (int c = 0; c < p_tree->get_columns(); c++) {
			e.columns.push_back(item->get_text(c));
		}
		out.push_back(e);
	}
	return out;
}

PopupMenu *menu_of(Node *p_node) {
	if (MenuButton *mb = Object::cast_to<MenuButton>(p_node)) {
		return mb->get_popup();
	}
	return Object::cast_to<PopupMenu>(p_node);
}

TabBar *tab_bar_of(Node *p_node) {
	if (TabContainer *tc = Object::cast_to<TabContainer>(p_node)) {
		return tc->get_tab_bar();
	}
	return Object::cast_to<TabBar>(p_node);
}

// The entries a "select" or "activate" can pick, in order (empty when p_node has none).
PackedStringArray items_of(Node *p_node) {
	PackedStringArray items;
	if (OptionButton *ob = Object::cast_to<OptionButton>(p_node)) {
		for (int i = 0; i < ob->get_item_count(); i++) {
			items.push_back(ob->get_item_text(i));
		}
	} else if (ItemList *il = Object::cast_to<ItemList>(p_node)) {
		for (int i = 0; i < il->get_item_count(); i++) {
			items.push_back(il->get_item_text(i));
		}
	} else if (Tree *tree = Object::cast_to<Tree>(p_node)) {
		for (const TreeEntry &e : tree_entries(tree)) {
			items.push_back(String(" | ").join(e.columns));
		}
	} else if (TabBar *tabs = tab_bar_of(p_node)) {
		for (int i = 0; i < tabs->get_tab_count(); i++) {
			items.push_back(tabs->get_tab_title(i));
		}
	} else if (PopupMenu *menu = menu_of(p_node)) {
		for (int i = 0; i < menu->get_item_count(); i++) {
			items.push_back(menu->get_item_text(i));
		}
	}
	return items;
}

String text_of(Node *p_node) {
	if (const Button *b = Object::cast_to<Button>(p_node)) {
		return b->get_text();
	}
	if (const Label *l = Object::cast_to<Label>(p_node)) {
		return l->get_text();
	}
	if (const LineEdit *le = Object::cast_to<LineEdit>(p_node)) {
		// A secret field shows bullets to a user (export keys, passwords; review finding 14).
		return le->is_secret() ? le->get_secret_character().repeat(le->get_text().length()) : le->get_text();
	}
	if (const TextEdit *te = Object::cast_to<TextEdit>(p_node)) {
		return te->get_text();
	}
	if (const RichTextLabel *rtl = Object::cast_to<RichTextLabel>(p_node)) {
		return rtl->get_parsed_text();
	}
	if (const EditorProperty *ep = Object::cast_to<EditorProperty>(p_node)) {
		return ep->get_label();
	}
	if (const Window *w = Object::cast_to<Window>(p_node)) {
		return w->get_title();
	}
	return String();
}

bool matches(Node *p_node, const String &p_needle) {
	if (p_needle.is_empty()) {
		return true;
	}
	PackedStringArray haystack = items_of(p_node);
	haystack.push_back(text_of(p_node));
	if (const Control *c = Object::cast_to<Control>(p_node)) {
		haystack.push_back(c->get_tooltip_text());
		haystack.push_back(c->get_accessibility_name());
	}
	if (const LineEdit *le = Object::cast_to<LineEdit>(p_node)) {
		haystack.push_back(le->get_placeholder());
	}
	if (const AcceptDialog *d = Object::cast_to<AcceptDialog>(p_node)) {
		haystack.push_back(d->get_text());
	}
	if (const Tree *tree = Object::cast_to<Tree>(p_node)) {
		for (int c = 0; c < tree->get_columns(); c++) {
			haystack.push_back(tree->get_column_title(c));
		}
	}
	for (const String &s : haystack) {
		if (s.to_lower().contains(p_needle)) {
			return true;
		}
	}
	return false;
}

// p_node itself when it is a T, otherwise its first reachable T descendant in the same window (an EditorProperty's
// LineEdit, the OptionButton inside an enum property...); dialogs below p_node are other surfaces.
template <typename T>
T *actionable(Node *p_root, Node *p_node) {
	if (T *t = Object::cast_to<T>(p_node)) {
		return t;
	}
	List<Node *> queue;
	queue.push_back(p_node);
	while (!queue.is_empty()) {
		Node *n = queue.front()->get();
		queue.pop_front();
		for (int i = 0; i < n->get_child_count(); i++) {
			Node *child = n->get_child(i);
			if (Object::cast_to<Window>(child)) {
				continue;
			}
			if (T *t = Object::cast_to<T>(child)) {
				if (reachable(p_root, t)) {
					return t;
				}
			}
			queue.push_back(child);
		}
	}
	return nullptr;
}

// A whole number given as an integer or a float without a fraction (JSON has one number type).
bool whole_number(const Variant &p_value, int64_t &r_value) {
	if (p_value.get_type() == Variant::INT) {
		r_value = p_value;
		return true;
	}
	if (p_value.get_type() == Variant::FLOAT) {
		const double d = p_value;
		if (Math::is_finite(d) && d == Math::floor(d) && Math::abs(d) < 9.0e15) {
			r_value = int64_t(d);
			return true;
		}
	}
	return false;
}

// The index named by p_value: an integer index, or an item text (exact match first, ignoring case, then contained).
int pick(const PackedStringArray &p_items, const Variant &p_value) {
	if (p_value.get_type() == Variant::INT || p_value.get_type() == Variant::FLOAT) {
		int64_t index = -1;
		return (whole_number(p_value, index) && index >= 0 && index < p_items.size()) ? int(index) : -1;
	}
	const String want = String(p_value).to_lower();
	if (want.is_empty()) {
		return -1;
	}
	for (int i = 0; i < p_items.size(); i++) {
		if (p_items[i].to_lower() == want) {
			return i;
		}
	}
	for (int i = 0; i < p_items.size(); i++) {
		if (p_items[i].to_lower().contains(want)) {
			return i;
		}
	}
	return -1;
}

int pick_tree(const Vector<TreeEntry> &p_entries, const Variant &p_value) {
	if (p_value.get_type() != Variant::STRING) {
		PackedStringArray joined;
		for (const TreeEntry &e : p_entries) {
			joined.push_back(String(" | ").join(e.columns));
		}
		return pick(joined, p_value);
	}
	const String want = String(p_value).to_lower();
	for (int i = 0; i < p_entries.size(); i++) {
		for (const String &column : p_entries[i].columns) {
			if (column.to_lower() == want) {
				return i; // A whole cell matches.
			}
		}
	}
	PackedStringArray joined;
	for (const TreeEntry &e : p_entries) {
		joined.push_back(String(" | ").join(e.columns));
	}
	return pick(joined, p_value);
}

// A click, delivered as the key press and release a user gives a focused button: BaseButton handles ui_accept as it
// handles a click (scene/gui/base_button.cpp:95-100,236-283), so the press goes through pressed() and _pressed (a
// MenuButton or OptionButton opens), toggles (radio buttons only turn on), button_down and button_up (review
// finding 7).
String click(BaseButton *p_button) {
	if (p_button->is_disabled()) {
		return "The button is disabled.";
	}
	Ref<InputEventAction> press;
	press.instantiate();
	press->set_action(SNAME("ui_accept"));
	press->set_pressed(true);
	Control *control = p_button; // gui_input is public on Control, protected on BaseButton; the call is virtual.
	control->gui_input(press);
	Ref<InputEventAction> release;
	release.instantiate();
	release->set_action(SNAME("ui_accept"));
	release->set_pressed(false);
	control->gui_input(release);
	return String();
}

AcceptDialog *dialog_of(Node *p_node) {
	for (Node *n = p_node; n; n = n->get_parent()) {
		if (AcceptDialog *d = Object::cast_to<AcceptDialog>(n)) {
			return d;
		}
	}
	return nullptr;
}

String select_item(Node *p_root, Node *p_node, const Variant &p_value, bool p_activate, Dictionary &r_picked) {
	if (OptionButton *ob = actionable<OptionButton>(p_root, p_node)) {
		if (p_activate) {
			return "Option lists have no activate; use select.";
		}
		if (ob->is_disabled()) {
			return "The list is disabled.";
		}
		// Picked from its popup as a click does: opening it first lists choices that are filled on opening, and the
		// popup selects through OptionButton::_selected, which does nothing for the current item
		// (scene/gui/option_button.cpp:442-445; review finding 9).
		ob->get_popup()->emit_signal(SNAME("about_to_popup"));
		const int index = pick(items_of(ob), p_value);
		if (index < 0 || ob->is_item_disabled(index) || ob->is_item_separator(index)) {
			return "No selectable option matches.";
		}
		r_picked["index"] = index;
		r_picked["text"] = ob->get_item_text(index);
		ob->get_popup()->activate_item(index);
		return String();
	}
	if (ItemList *il = Object::cast_to<ItemList>(p_node)) {
		const int index = pick(items_of(il), p_value);
		if (index < 0 || il->is_item_disabled(index) || !il->is_item_selectable(index)) {
			return "No selectable item matches.";
		}
		// As a plain left click (scene/gui/item_list.cpp:761-766,818-855): toggle mode flips the item; otherwise the item
		// becomes the only selection, reported as item_selected in single mode (only when not selected yet, unless
		// reselect is allowed) and as multi_selected in multi mode.
		if (il->get_select_mode() == ItemList::SELECT_TOGGLE) {
			const bool on = !il->is_selected(index);
			if (on) {
				il->select(index, false);
			} else {
				il->deselect(index);
			}
			il->emit_signal(SNAME("multi_selected"), index, on);
		} else if (!il->is_selected(index) || il->get_allow_reselect() || il->get_select_mode() == ItemList::SELECT_MULTI) {
			il->select(index, true);
			if (il->get_select_mode() == ItemList::SELECT_SINGLE) {
				il->emit_signal(SceneStringName(item_selected), index);
			} else {
				il->emit_signal(SNAME("multi_selected"), index, true);
			}
		}
		il->ensure_current_is_visible();
		il->emit_signal(SNAME("item_clicked"), index, il->get_item_rect(index).position, MouseButton::LEFT);
		if (p_activate) {
			il->emit_signal(SNAME("item_activated"), index);
		}
		r_picked["index"] = index;
		r_picked["text"] = il->get_item_text(index);
		return String();
	}
	if (Tree *tree = Object::cast_to<Tree>(p_node)) {
		const Vector<TreeEntry> entries = tree_entries(tree);
		const int index = pick_tree(entries, p_value);
		if (index < 0 || !entries[index].item->is_selectable(0)) {
			return "No selectable tree item matches.";
		}
		TreeItem *item = entries[index].item;
		item->uncollapse_tree();
		// The click path (Tree::set_selected runs select_single_item, scene/gui/tree.cpp:5899-5905): the signals of the
		// tree's mode, none for a cell already selected (review finding 9).
		tree->set_selected(item, 0);
		tree->scroll_to_item(item);
		if (p_activate) {
			tree->emit_signal(SNAME("item_activated"));
		}
		r_picked["index"] = index;
		r_picked["text"] = String(" | ").join(entries[index].columns);
		return String();
	}
	if (TabBar *tabs = tab_bar_of(p_node)) {
		if (!reachable(p_root, tabs)) {
			return "The tabs are hidden."; // A TabContainer without visible tabs (review finding 31).
		}
		const int index = pick(items_of(p_node), p_value);
		if (index < 0 || tabs->is_tab_disabled(index) || tabs->is_tab_hidden(index)) {
			return "No available tab matches.";
		}
		if (TabContainer *tc = Object::cast_to<TabContainer>(p_node)) {
			tc->set_current_tab(index);
		} else {
			tabs->set_current_tab(index);
		}
		r_picked["index"] = index;
		r_picked["text"] = tabs->get_tab_title(index);
		return String();
	}
	if (PopupMenu *menu = menu_of(p_node)) {
		if (MenuButton *mb = Object::cast_to<MenuButton>(p_node)) {
			if (mb->is_disabled()) {
				return "The menu is disabled."; // Review finding 10.
			}
			mb->emit_signal(SNAME("about_to_popup")); // As MenuButton::show_popup (scene/gui/menu_button.cpp:86).
		}
		menu->emit_signal(SNAME("about_to_popup")); // Menus that fill themselves on opening (review finding 10).
		const int index = pick(items_of(p_node), p_value);
		if (index < 0 || menu->is_item_disabled(index) || menu->is_item_separator(index)) {
			return "No enabled menu item matches.";
		}
		r_picked["index"] = index;
		r_picked["text"] = menu->get_item_text(index);
		menu->activate_item(index);
		return String();
	}
	return "Nothing to select here: use a list, tree, option button, tab bar or menu.";
}

} // namespace

Dictionary describe(Node *p_root, Node *p_node) {
	Dictionary d;
	d["path"] = String(p_root->get_path_to(p_node));
	d["class"] = p_node->get_class();
	const String text = text_of(p_node);
	if (!text.is_empty()) {
		d["text"] = clip(text);
	}
	if (const Control *c = Object::cast_to<Control>(p_node)) {
		if (!c->get_tooltip_text().is_empty()) {
			d["tooltip"] = clip(c->get_tooltip_text());
		}
	}
	if (const BaseButton *b = Object::cast_to<BaseButton>(p_node)) {
		d["disabled"] = b->is_disabled();
		if (b->is_toggle_mode()) {
			d["pressed"] = b->is_pressed();
		}
	}
	if (const LineEdit *le = Object::cast_to<LineEdit>(p_node)) {
		d["editable"] = le->is_editable();
		if (!le->get_placeholder().is_empty()) {
			d["placeholder"] = le->get_placeholder();
		}
	} else if (const TextEdit *te = Object::cast_to<TextEdit>(p_node)) {
		d["editable"] = te->is_editable();
	} else if (const Range *r = Object::cast_to<Range>(p_node)) {
		d["value"] = r->get_value();
	}
	if (const AcceptDialog *dialog = Object::cast_to<AcceptDialog>(p_node)) {
		d["message"] = clip(dialog->get_text());
	}
	const PackedStringArray items = items_of(p_node);
	if (!items.is_empty()) {
		d["item_count"] = items.size();
		d["items"] = items.size() > MAX_ITEMS ? items.slice(0, MAX_ITEMS) : items;
	}
	if (const OptionButton *ob = Object::cast_to<OptionButton>(p_node)) {
		d["selected"] = ob->get_selected();
	} else if (ItemList *il = Object::cast_to<ItemList>(p_node)) {
		d["selected"] = il->get_selected_items();
	} else if (const Tree *tree = Object::cast_to<Tree>(p_node)) {
		const TreeItem *sel = tree->get_selected();
		if (sel) {
			d["selected"] = sel->get_text(0);
		}
		if (tree->are_column_titles_visible()) {
			PackedStringArray columns;
			for (int c = 0; c < tree->get_columns(); c++) {
				columns.push_back(tree->get_column_title(c));
			}
			d["columns"] = columns;
		}
	} else if (const TabBar *tabs = tab_bar_of(p_node)) {
		d["selected"] = tabs->get_current_tab();
	}
	for (Node *n = p_node->get_parent(); n && n != p_root; n = n->get_parent()) {
		if (const Window *w = Object::cast_to<Window>(n)) {
			d["window"] = w->get_title();
			break;
		}
	}
	return d;
}

Array find(Node *p_root, Node *p_under, const String &p_text, const String &p_class, int p_limit) {
	Array out;
	ERR_FAIL_NULL_V(p_root, out);
	Node *start = p_under ? p_under : p_root;
	if (start != p_root && !reachable(p_root, start)) {
		return out; // The root is where the search starts, not a control: an open dialog under it is still found.
	}
	const String needle = p_text.to_lower();
	// Depth first, children in order; inside a hidden control only windows (dialogs) can still be visible.
	struct Step {
		Node *node;
		bool hidden;
	};
	LocalVector<Step> stack;
	stack.push_back({ start, false });
	int visited = 0;
	while (!stack.is_empty() && out.size() < p_limit && visited < MAX_VISITED) {
		const Step step = stack[stack.size() - 1];
		stack.resize(stack.size() - 1);
		visited++;
		Node *n = step.node;
		bool hidden = step.hidden;
		if (input_disabled(n)) {
			continue; // The edited scene: not part of the editor's UI (review finding 2).
		}
		if (const Window *w = Object::cast_to<Window>(n)) {
			if (!w->is_visible()) {
				continue; // Nothing inside a hidden window is visible.
			}
			hidden = false;
		} else if (const CanvasItem *ci = Object::cast_to<CanvasItem>(n)) {
			hidden = hidden || !ci->is_visible();
		}
		const bool candidate = Object::cast_to<Control>(n) || Object::cast_to<Window>(n);
		if (!hidden && candidate && n != p_root && (p_class.is_empty() || n->is_class(p_class)) && matches(n, needle) && reachable(p_root, n)) {
			out.push_back(describe(p_root, n));
		}
		for (int i = n->get_child_count() - 1; i >= 0; i--) {
			stack.push_back({ n->get_child(i), hidden });
		}
	}
	return out;
}

Array open_dialogs(Node *p_root) {
	Array out;
	for (const Variant &v : find(p_root, nullptr, String(), "AcceptDialog", 20)) {
		Dictionary d = v;
		Dictionary brief;
		brief["path"] = d["path"];
		brief["title"] = d.get("text", "");
		brief["message"] = d.get("message", "");
		out.push_back(brief);
	}
	return out;
}

String act(Node *p_root, const String &p_path, const String &p_action, const Variant &p_value, Dictionary &r_result) {
	ERR_FAIL_NULL_V(p_root, "No editor.");
	Node *node = p_path.is_empty() ? nullptr : p_root->get_node_or_null(NodePath(p_path));
	if (!node || !p_root->is_ancestor_of(node)) {
		return "No editor control at that path (paths come from editor_ui_find)."; // Never the editor root itself.
	}
	if (!reachable(p_root, node)) {
		return "That control is not visible, or an open dialog is in front of it.";
	}
	String error;
	Dictionary picked;
	// What the action is done to, described before it: a dialog can leave the tree when it closes, and its path can
	// no longer be resolved then (review finding 15).
	ObjectID target; // By ID: an action can free what it acted on (a dialog deleted when it closes).
	Dictionary before;
	const auto aim = [&](Node *p_target) {
		target = p_target->get_instance_id();
		before = describe(p_root, p_target);
	};
	aim(node);
	if (p_action == "press") {
		BaseButton *button = Object::cast_to<BaseButton>(node);
		if (!button) {
			return "Only buttons can be pressed.";
		}
		error = click(button);
	} else if (p_action == "toggle") {
		BaseButton *button = actionable<BaseButton>(p_root, node);
		if (!button || !button->is_toggle_mode()) {
			return "Only check boxes and toggle buttons can be toggled.";
		}
		if (p_value.get_type() != Variant::NIL && p_value.get_type() != Variant::BOOL) {
			return "toggle takes true, false or nothing (the opposite of now)."; // Review finding 31.
		}
		aim(button);
		const bool want = p_value.get_type() == Variant::BOOL ? bool(p_value) : !button->is_pressed();
		if (button->is_pressed() != want) {
			error = click(button);
			if (error.is_empty() && button->is_pressed() != want) {
				error = "This option cannot be turned off; choose another one of its group.";
			}
		}
	} else if (p_action == "set_text" || p_action == "submit") {
		// Text as typed: numbers as their digits ("42", not "42.0"); nothing typed for a submit without a value
		// (review finding 8).
		String text;
		bool typed = true;
		int64_t whole = 0;
		if (p_value.get_type() == Variant::STRING || p_value.get_type() == Variant::STRING_NAME) {
			text = p_value;
		} else if (whole_number(p_value, whole)) {
			text = itos(whole);
		} else if (p_value.get_type() == Variant::FLOAT && Math::is_finite(double(p_value))) {
			text = String::num(double(p_value));
		} else if (p_value.get_type() == Variant::NIL && p_action == "submit") {
			typed = false;
		} else {
			return p_action == "set_text" ? "set_text needs the text to type." : "submit takes the text to type, or nothing to submit what is there.";
		}
		if (LineEdit *le = actionable<LineEdit>(p_root, node)) {
			aim(le);
			if (!le->is_editable()) {
				return "The text field is read-only.";
			}
			if (typed) {
				le->set_text(text);
				le->emit_signal(SceneStringName(text_changed), le->get_text()); // As typing does.
			}
			if (p_action == "submit") {
				le->emit_signal(SceneStringName(text_submitted), le->get_text()); // As Enter does.
			}
		} else if (TextEdit *te = actionable<TextEdit>(p_root, node)) {
			aim(te);
			if (p_action == "submit") {
				return "A text area has no Enter to submit; Enter adds a line. Use set_text.";
			}
			if (!te->is_editable()) {
				return "The text area is read-only.";
			}
			te->set_text(text);
			te->emit_signal(SceneStringName(text_changed));
		} else {
			return "No text field here.";
		}
	} else if (p_action == "set_value") {
		// Number fields and sliders a user drags or types into; not progress or scroll bars (review finding 31).
		Range *range = actionable<SpinBox>(p_root, node);
		if (!range) {
			range = actionable<EditorSpinSlider>(p_root, node);
		}
		if (!range) {
			range = actionable<Slider>(p_root, node);
		}
		if (!range) {
			return "No number field or slider here.";
		}
		if ((p_value.get_type() != Variant::INT && p_value.get_type() != Variant::FLOAT) || !Math::is_finite(double(p_value))) {
			return "set_value needs a number.";
		}
		const SpinBox *spin = Object::cast_to<SpinBox>(range);
		const EditorSpinSlider *slider = Object::cast_to<EditorSpinSlider>(range);
		const Slider *plain = Object::cast_to<Slider>(range);
		if ((spin && !spin->is_editable()) || (slider && slider->is_read_only()) || (plain && !plain->is_editable())) {
			return "The number field is read-only.";
		}
		aim(range);
		range->set_value(p_value); // Emits value_changed.
	} else if (p_action == "select" || p_action == "activate") {
		error = select_item(p_root, node, p_value, p_action == "activate", picked);
	} else if (p_action == "confirm" || p_action == "cancel") {
		AcceptDialog *dialog = dialog_of(node);
		if (!dialog) {
			return "Not a dialog or a control inside one.";
		}
		aim(dialog);
		if (p_action == "confirm") {
			if (!dialog->get_ok_button()->is_visible()) {
				return "This dialog has no OK button to confirm with."; // Review finding 31.
			}
			error = click(dialog->get_ok_button());
		} else if (ConfirmationDialog *cd = Object::cast_to<ConfirmationDialog>(dialog); cd && cd->get_cancel_button()->is_visible()) {
			error = click(cd->get_cancel_button());
		} else {
			// As Esc or the window's close button: the dialog's own cancel path, overrides included (review finding 15).
			dialog->notification(Window::NOTIFICATION_WM_CLOSE_REQUEST);
		}
	} else {
		return "Unknown action: use press, toggle, set_text, submit, set_value, select, activate, confirm or cancel.";
	}
	if (!error.is_empty()) {
		return error;
	}
	Node *acted_on = ObjectDB::get_instance<Node>(target);
	if (acted_on && acted_on->is_inside_tree() && p_root->is_ancestor_of(acted_on)) {
		r_result = describe(p_root, acted_on);
	} else {
		r_result = before; // The dialog closed and left the tree, or was freed.
		r_result["closed"] = true;
	}
	if (!picked.is_empty()) {
		r_result["picked"] = picked;
	}
	return String();
}

namespace {

Node *editor_root() {
	return EditorNode::get_singleton() ? EditorNode::get_singleton()->get_tree()->get_root() : nullptr;
}

Dictionary schema_property(const String &p_type, const String &p_description) {
	Dictionary property;
	property["type"] = p_type;
	property["description"] = p_description;
	return property;
}

void tool_find(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	Node *root = editor_root();
	const String under_path = p_args.get("under", "");
	Node *under = under_path.is_empty() ? nullptr : root->get_node_or_null(NodePath(under_path));
	if (!under_path.is_empty() && (!under || !reachable(root, under))) {
		p_call->fail("\"under\" is not a visible editor control (paths come from editor_ui_find).");
		return;
	}
	Dictionary result;
	int64_t limit = 30;
	if (p_args.has("limit") && !whole_number(p_args["limit"], limit)) {
		p_call->fail("\"limit\" must be a whole number.");
		return;
	}
	result["matches"] = find(root, under, p_args.get("text", ""), p_args.get("class", ""), int(CLAMP(limit, int64_t(1), int64_t(500))));
	result["open_dialogs"] = open_dialogs(root);
	p_call->complete_data(result);
}

void complete_act(const Ref<ClaudeBridgeToolCall> &p_call, Dictionary p_result) {
	p_result["open_dialogs"] = open_dialogs(editor_root());
	p_call->complete_data(p_result);
}

void tool_act(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	Node *root = editor_root();
	Dictionary result;
	const String error = act(root, p_args.get("path", ""), p_args.get("action", ""), p_args.get("value", Variant()), result);
	if (!error.is_empty()) {
		p_call->fail(error + " Nothing was done.");
		return;
	}
	// Answered after the calls the action deferred: a cancelled dialog hides deferred (scene/gui/dialogs.cpp:156), and
	// the open dialogs reported should be the ones left.
	callable_mp_static(&complete_act).call_deferred(p_call, result);
}

} // namespace

void register_tools(ClaudeBridgeMCP &r_mcp) {
	ClaudeBridgeMCP::Tool find_tool;
	find_tool.name = "editor_ui_find";
	find_tool.title = "Find editor controls";
	find_tool.description = "Lists the editor controls and dialogs a user can reach (visible, not behind an open dialog, not the edited scene) whose text, tooltip, placeholder, list items, column titles or title contain \"text\" (case-insensitive), optionally of a class and under a path. Each match has the path editor_ui_act takes, plus its text (secret fields masked), state, value and items (lists that fill on opening show their current item until opened). Also lists open dialogs.";
	Dictionary find_properties;
	find_properties["text"] = schema_property("string", "Text to look for (empty: every visible control, use with class or under).");
	find_properties["class"] = schema_property("string", "Only controls of this class or a subclass, e.g. Button, LineEdit, Tree, AcceptDialog.");
	find_properties["under"] = schema_property("string", "Only inside this control or dialog (a path from a previous result).");
	find_properties["limit"] = schema_property("integer", "Maximum matches (default 30).");
	find_tool.input_schema = ClaudeBridgeMCP::object_schema(find_properties);
	find_tool.func = &tool_find;
	r_mcp.register_tool(find_tool);

	ClaudeBridgeMCP::Tool act_tool;
	act_tool.name = "editor_ui_act";
	act_tool.title = "Use an editor control";
	act_tool.description = "Acts on an editor control a user can reach, the way a user would: press a button (a click: menu and option buttons open); toggle a check box (value true/false); set_text or submit a text field (typing, then Enter; submit without a value submits what is there); set_value of a number field or slider; select or activate (double-click) an item of a list, tree, option button, tab bar or menu by text or index (menus are opened first; picking the current item changes nothing); confirm or cancel a dialog (its OK or Cancel button, or Esc). Containers such as inspector properties act through the control inside them. Disabled and read-only controls refuse, and so does anything behind an open dialog. Edits made this way are the editor's own, undoable as usual.";
	Dictionary act_properties;
	act_properties["path"] = schema_property("string", "Control path from editor_ui_find.");
	Dictionary action = schema_property("string", "What to do.");
	action["enum"] = Array({ "press", "toggle", "set_text", "submit", "set_value", "select", "activate", "confirm", "cancel" });
	act_properties["action"] = action;
	Dictionary value;
	value["description"] = "Text for set_text/submit, number for set_value, item text or index for select/activate, true/false for toggle.";
	act_properties["value"] = value;
	act_tool.input_schema = ClaudeBridgeMCP::object_schema(act_properties, PackedStringArray({ "path", "action" }));
	act_tool.read_only = false;
	// Left destructive: like a user, it can reach any editor action, deleting files after their confirmation included.
	act_tool.func = &tool_act;
	r_mcp.register_tool(act_tool);
}

} // namespace ClaudeBridgeUI
