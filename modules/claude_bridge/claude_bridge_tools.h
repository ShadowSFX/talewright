/**************************************************************************/
/*  claude_bridge_tools.h                                                 */
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

class ClaudeBridgePlugin;

// Shared helpers and tool registration for the Claude bridge.
namespace ClaudeBridgeTools {

// JSON-safe view of a Variant: JSON natives stay native, math/engine types become Godot literal strings
// (e.g. "Vector2(1, 2)", parseable by str_to_var), objects become small descriptors. Bounded in depth/size.
Variant variant_to_json(const Variant &p_value, int p_depth = 0);
// Converts a JSON argument into a value for a property of type p_target (accepts Godot literal strings).
Variant json_to_value(const Variant &p_json, Variant::Type p_target, bool &r_ok);
// res:// path validation: inside the project, normalized, no traversal. Returns an empty string when invalid.
String validate_res_path(const String &p_path);

void register_editor_tools(ClaudeBridgeMCP &r_mcp, ClaudeBridgePlugin *p_plugin);

} // namespace ClaudeBridgeTools
