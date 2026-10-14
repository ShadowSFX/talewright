/**************************************************************************/
/*  jrpg_managed_addon.h                                                  */
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

#include "core/object/ref_counted.h"

// The managed addon (D-004): GDScript and scene sources embedded in the editor binary (modules/jrpg/addon_template,
// packed at build time by jrpg_builders.py) and copied into projects at res://addons/talewright. The folder's
// addon.cfg records the installed version and the SHA-256 of every installed file. Installing again upgrades: a file
// is replaced only while it still has its installed hash, so files the user changed are kept and reported.
// Editor-only authoring API; the bridge exposes it as jrpg_addon_status and jrpg_addon_install (D-012, D-030).
class JRPGManagedAddon : public RefCounted {
	GDCLASS(JRPGManagedAddon, RefCounted);

protected:
	static void _bind_methods();

public:
	static constexpr const char *DEFAULT_DIRECTORY = "res://addons/talewright";
	static constexpr const char *STAMP_FILE = "addon.cfg";

	struct SourceFile {
		String path; // Relative to the addon folder, with "/" separators.
		PackedByteArray content;
	};

	static String get_embedded_version();
	static PackedStringArray get_embedded_files();
	static Vector<SourceFile> get_embedded_sources();

	// {directory, embedded_version, installed_version ("" when not installed), files: [{path, state}]} where state is
	// "missing", "current" (same as embedded), "outdated" (as installed by an older version) or "modified".
	static Dictionary get_status(const String &p_directory);
	// Installs or upgrades the embedded addon in p_directory. Returns {version, added, replaced, unchanged,
	// kept_modified} (lists of relative paths).
	static Dictionary install(const String &p_directory);
	// install() for any set of files and version (tests use it to simulate upgrades).
	static Dictionary install_files(const String &p_directory, const Vector<SourceFile> &p_files, const String &p_version);
};
