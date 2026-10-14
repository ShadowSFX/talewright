/**************************************************************************/
/*  claude_bridge_ui_tools.h                                              */
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

#include "claude_bridge_mcp.h"

class Node;

// Editor UI interaction for the Claude bridge (D-031): find the editor's visible controls and act on them through the
// same signals and setters a click or keystroke would use, so editor UI can be verified end to end. Only visible
// controls inside the given root (the editor's windows) are reachable, and disabled or read-only ones refuse actions.
namespace ClaudeBridgeUI {

// Visible controls and dialogs under p_root, in tree order, whose text, tooltip, accessibility name, placeholder,
// list items or window title contain p_text (case-insensitive; empty matches everything) and whose class is p_class
// or inherits it (empty: any). At most p_limit results; each is described as by describe().
Array find(Node *p_root, Node *p_under, const String &p_text, const String &p_class, int p_limit);
// {path (relative to p_root), class, text, tooltip, disabled, pressed, value, items, selected, window}: the fields
// that apply to p_node.
Dictionary describe(Node *p_root, Node *p_node);
// Performs p_action on the node at p_path (relative to p_root) and fills r_result with its new description. Returns
// an empty string on success, otherwise why the action was refused (nothing happened then).
// Actions: press, toggle (value: bool, default the opposite), set_text, submit (value: text, optional), set_value,
// select and activate (value: item text or index), confirm, cancel (dialogs, or any control inside one).
String act(Node *p_root, const String &p_path, const String &p_action, const Variant &p_value, Dictionary &r_result);
// Visible dialogs under p_root: [{path, title, text}].
Array open_dialogs(Node *p_root);

void register_tools(ClaudeBridgeMCP &r_mcp);

} // namespace ClaudeBridgeUI
