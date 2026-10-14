/**************************************************************************/
/*  test_jrpg_managed_addon.h                                             */
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

#ifdef TOOLS_ENABLED

#include "core/crypto/crypto_core.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/editor/jrpg_addon_files.gen.h"
#include "modules/jrpg/editor/jrpg_managed_addon.h"

namespace TestJRPGManagedAddon {

static JRPGManagedAddon::SourceFile source(const String &p_path, const String &p_text) {
	JRPGManagedAddon::SourceFile file;
	file.path = p_path;
	file.content = p_text.to_utf8_buffer();
	return file;
}

static String read_text(const String &p_path) {
	return FileAccess::get_file_as_string(p_path);
}

static void write_text(const String &p_path, const String &p_text) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	REQUIRE(f.is_valid());
	f->store_string(p_text);
}

// A fresh, empty folder under the test temp directory.
static String empty_folder(const String &p_name) {
	const String dir = TestUtils::get_temp_path(p_name);
	if (DirAccess::exists(dir)) {
		Ref<DirAccess> da = DirAccess::open(dir);
		REQUIRE(da.is_valid());
		REQUIRE(da->erase_contents_recursive() == OK);
	}
	REQUIRE(DirAccess::make_dir_recursive_absolute(dir) == OK);
	return dir;
}

TEST_CASE("[JRPG][Addon] The embedded addon unpacks to its recorded hashes and installs once") {
	const PackedStringArray files = JRPGManagedAddon::get_embedded_files();
	CHECK(files.has("README.md"));
	CHECK(files.has("LICENSE.txt"));
	CHECK(JRPGManagedAddon::get_embedded_version().begins_with("1-"));
	const Vector<JRPGManagedAddon::SourceFile> sources = JRPGManagedAddon::get_embedded_sources();
	REQUIRE(sources.size() == JRPG_ADDON_FILE_COUNT);
	for (int i = 0; i < sources.size(); i++) {
		unsigned char hash[32];
		CryptoCore::sha256(sources[i].content.ptr(), sources[i].content.size(), hash);
		CHECK_MESSAGE(String::hex_encode_buffer(hash, 32) == JRPG_ADDON_FILES[i].sha256, sources[i].path);
		CHECK_FALSE(sources[i].content.has('\r')); // Text is stored with LF endings on every machine.
	}

	const String dir = empty_folder("jrpg_addon_install");
	Dictionary status = JRPGManagedAddon::get_status(dir);
	CHECK(String(status["installed_version"]).is_empty());
	CHECK(String(Dictionary(Array(status["files"])[0])["state"]) == "missing");

	Dictionary report = JRPGManagedAddon::install(dir);
	CHECK(PackedStringArray(report["added"]).size() == files.size());
	CHECK(read_text(dir.path_join("README.md")).contains("Talewright managed addon"));
	Ref<ConfigFile> stamp;
	stamp.instantiate();
	REQUIRE(stamp->load(dir.path_join(JRPGManagedAddon::STAMP_FILE)) == OK);
	CHECK(String(stamp->get_value("addon", "version", "")) == JRPGManagedAddon::get_embedded_version());
	status = JRPGManagedAddon::get_status(dir);
	CHECK(String(status["installed_version"]) == JRPGManagedAddon::get_embedded_version());
	for (const Variant &entry : Array(status["files"])) {
		CHECK(String(Dictionary(entry)["state"]) == "current");
	}

	// A second run changes nothing.
	report = JRPGManagedAddon::install(dir);
	CHECK(PackedStringArray(report["unchanged"]).size() == files.size());
	CHECK(PackedStringArray(report["added"]).is_empty());
	CHECK(PackedStringArray(report["replaced"]).is_empty());
}

TEST_CASE("[JRPG][Addon] Upgrades replace untouched files only") {
	const String dir = empty_folder("jrpg_addon_upgrade");
	Vector<JRPGManagedAddon::SourceFile> v1;
	v1.push_back(source("screens/title.gd", "# title v1\n"));
	v1.push_back(source("screens/menu.gd", "# menu v1\n"));
	Dictionary report = JRPGManagedAddon::install_files(dir, v1, "1-a");
	CHECK(PackedStringArray(report["added"]).size() == 2);

	write_text(dir.path_join("screens/menu.gd"), "# menu v1, changed by the user\n");
	write_text(dir.path_join("notes.gd"), "# the user's own file\n"); // Same name as a later addon file.

	Vector<JRPGManagedAddon::SourceFile> v2;
	v2.push_back(source("screens/title.gd", "# title v2\n"));
	v2.push_back(source("screens/menu.gd", "# menu v2\n"));
	v2.push_back(source("screens/shop.gd", "# shop v2\n"));
	v2.push_back(source("notes.gd", "# addon notes\n"));
	report = JRPGManagedAddon::install_files(dir, v2, "1-b");
	CHECK(PackedStringArray(report["replaced"]) == PackedStringArray({ "screens/title.gd" }));
	CHECK(PackedStringArray(report["kept_modified"]) == PackedStringArray({ "screens/menu.gd", "notes.gd" }));
	CHECK(PackedStringArray(report["added"]) == PackedStringArray({ "screens/shop.gd" }));
	CHECK(read_text(dir.path_join("screens/title.gd")) == "# title v2\n");
	CHECK(read_text(dir.path_join("screens/menu.gd")) == "# menu v1, changed by the user\n");
	CHECK(read_text(dir.path_join("notes.gd")) == "# the user's own file\n"); // Never installed by us: kept.

	Ref<ConfigFile> stamp;
	stamp.instantiate();
	REQUIRE(stamp->load(dir.path_join(JRPGManagedAddon::STAMP_FILE)) == OK);
	CHECK(String(stamp->get_value("addon", "version", "")) == "1-b");

	// The changed file stays "changed" on later upgrades too.
	Vector<JRPGManagedAddon::SourceFile> v3;
	v3.push_back(source("screens/menu.gd", "# menu v3\n"));
	report = JRPGManagedAddon::install_files(dir, v3, "1-c");
	CHECK(PackedStringArray(report["kept_modified"]) == PackedStringArray({ "screens/menu.gd" }));
	// Once the user restores the installed text, the next upgrade replaces it again.
	write_text(dir.path_join("screens/menu.gd"), "# menu v1\n");
	report = JRPGManagedAddon::install_files(dir, v3, "1-c");
	CHECK(PackedStringArray(report["replaced"]) == PackedStringArray({ "screens/menu.gd" }));

	// Paths leaving the addon folder are refused.
	Vector<JRPGManagedAddon::SourceFile> bad;
	bad.push_back(source("../escape.gd", "# no\n"));
	ERR_PRINT_OFF;
	report = JRPGManagedAddon::install_files(dir, bad, "1-d");
	ERR_PRINT_ON;
	CHECK_FALSE(FileAccess::exists(dir.get_base_dir().path_join("escape.gd")));
}

} // namespace TestJRPGManagedAddon

#endif // TOOLS_ENABLED
