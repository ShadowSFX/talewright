/**************************************************************************/
/*  jrpg_managed_addon.cpp                                                */
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

#include "jrpg_managed_addon.h"

#include "jrpg_addon_files.gen.h"

#include "core/crypto/crypto_core.h"
#include "core/io/compression.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/class_db.h"
#include "editor/file_system/editor_file_system.h"

namespace {

String sha256_hex(const PackedByteArray &p_data) {
	unsigned char hash[32];
	CryptoCore::sha256(p_data.ptr(), p_data.size(), hash);
	return String::hex_encode_buffer(hash, 32);
}

String file_hash(const String &p_path) {
	return FileAccess::exists(p_path) ? FileAccess::get_sha256(p_path) : String();
}

} // namespace

String JRPGManagedAddon::get_embedded_version() {
	return JRPG_ADDON_VERSION;
}

PackedStringArray JRPGManagedAddon::get_embedded_files() {
	PackedStringArray paths;
	for (int i = 0; i < JRPG_ADDON_FILE_COUNT; i++) {
		paths.push_back(JRPG_ADDON_FILES[i].path);
	}
	return paths;
}

Vector<JRPGManagedAddon::SourceFile> JRPGManagedAddon::get_embedded_sources() {
	Vector<SourceFile> files;
	for (int i = 0; i < JRPG_ADDON_FILE_COUNT; i++) {
		const JRPGAddonFile &embedded = JRPG_ADDON_FILES[i];
		SourceFile file;
		file.path = embedded.path;
		file.content.resize(embedded.size);
		const int64_t size = Compression::decompress(file.content.ptrw(), embedded.size, embedded.data, embedded.compressed_size, Compression::MODE_DEFLATE);
		ERR_CONTINUE_MSG(size != embedded.size, vformat("JRPG managed addon: \"%s\" is damaged in this build.", embedded.path));
		files.push_back(file);
	}
	return files;
}

Dictionary JRPGManagedAddon::get_status(const String &p_directory) {
	Ref<ConfigFile> stamp;
	stamp.instantiate();
	const bool installed = stamp->load(p_directory.path_join(STAMP_FILE)) == OK;
	Dictionary status;
	status["directory"] = p_directory;
	status["embedded_version"] = get_embedded_version();
	status["installed_version"] = installed ? String(stamp->get_value("addon", "version", "")) : String();
	Array files;
	for (int i = 0; i < JRPG_ADDON_FILE_COUNT; i++) {
		const JRPGAddonFile &embedded = JRPG_ADDON_FILES[i];
		const String current = file_hash(p_directory.path_join(embedded.path));
		const String recorded = stamp->get_value("files", embedded.path, "");
		String state = "modified";
		if (current.is_empty()) {
			state = "missing";
		} else if (current == embedded.sha256) {
			state = "current";
		} else if (current == recorded) {
			state = "outdated";
		}
		Dictionary entry;
		entry["path"] = embedded.path;
		entry["state"] = state;
		files.push_back(entry);
	}
	status["files"] = files;
	return status;
}

Dictionary JRPGManagedAddon::install(const String &p_directory) {
	return install_files(p_directory, get_embedded_sources(), get_embedded_version());
}

Dictionary JRPGManagedAddon::install_files(const String &p_directory, const Vector<SourceFile> &p_files, const String &p_version) {
	Dictionary report;
	ERR_FAIL_COND_V_MSG(p_directory.is_empty(), report, "JRPG managed addon: no target folder.");
	const String stamp_path = p_directory.path_join(STAMP_FILE);
	Ref<ConfigFile> stamp;
	stamp.instantiate();
	stamp->load(stamp_path); // Missing on a first install.

	PackedStringArray added, replaced, unchanged, kept_modified;
	for (const SourceFile &file : p_files) {
		ERR_CONTINUE_MSG(file.path.is_empty() || file.path.contains("..") || file.path.is_absolute_path(), vformat("JRPG managed addon: bad file path \"%s\".", file.path));
		const String target = p_directory.path_join(file.path);
		const String wanted = sha256_hex(file.content);
		const String current = file_hash(target);
		const String recorded = stamp->get_value("files", file.path, "");
		if (current == wanted) {
			unchanged.push_back(file.path);
			stamp->set_value("files", file.path, wanted);
			continue;
		}
		if (!current.is_empty() && current != recorded) {
			// Changed by the user since it was installed (or not installed by us): keep it, keep the old record.
			kept_modified.push_back(file.path);
			continue;
		}
		const Error dir_err = DirAccess::make_dir_recursive_absolute(target.get_base_dir());
		ERR_CONTINUE_MSG(dir_err != OK, vformat("JRPG managed addon: cannot create \"%s\".", target.get_base_dir()));
		Ref<FileAccess> out = FileAccess::open(target, FileAccess::WRITE);
		ERR_CONTINUE_MSG(out.is_null(), vformat("JRPG managed addon: cannot write \"%s\".", target));
		out->store_buffer(file.content);
		out.unref();
		(current.is_empty() ? added : replaced).push_back(file.path);
		stamp->set_value("files", file.path, wanted);
	}
	stamp->set_value("addon", "version", p_version);
	const Error save_err = stamp->save(stamp_path);
	ERR_FAIL_COND_V_MSG(save_err != OK, report, vformat("JRPG managed addon: cannot write \"%s\".", stamp_path));

	if (p_directory.begins_with("res://") && EditorFileSystem::get_singleton()) {
		EditorFileSystem::get_singleton()->scan_changes(); // Let the FileSystem dock and the importers see the files.
	}
	report["version"] = p_version;
	report["added"] = added;
	report["replaced"] = replaced;
	report["unchanged"] = unchanged;
	report["kept_modified"] = kept_modified;
	return report;
}

void JRPGManagedAddon::_bind_methods() {
	ClassDB::bind_static_method("JRPGManagedAddon", D_METHOD("get_embedded_version"), &JRPGManagedAddon::get_embedded_version);
	ClassDB::bind_static_method("JRPGManagedAddon", D_METHOD("get_embedded_files"), &JRPGManagedAddon::get_embedded_files);
	ClassDB::bind_static_method("JRPGManagedAddon", D_METHOD("get_status", "directory"), &JRPGManagedAddon::get_status, DEFVAL(DEFAULT_DIRECTORY));
	ClassDB::bind_static_method("JRPGManagedAddon", D_METHOD("install", "directory"), &JRPGManagedAddon::install, DEFVAL(DEFAULT_DIRECTORY));
}
