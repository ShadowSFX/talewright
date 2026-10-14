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
#include "core/templates/hash_set.h"
#include "editor/file_system/editor_file_system.h"

namespace {

String sha256_hex(const PackedByteArray &p_data) {
	unsigned char hash[32];
	CryptoCore::sha256(p_data.ptr(), p_data.size(), hash);
	return String::hex_encode_buffer(hash, 32);
}

// The suffixes modules/jrpg/jrpg_builders.py stores with LF line endings (TEXT_SUFFIXES there).
bool is_text_file(const String &p_path) {
	static const char *const TEXT_SUFFIXES[] = { "gd", "tscn", "tres", "cfg", "md", "txt", "json", "gdshader", "csv", "svg" };
	const String extension = p_path.get_extension().to_lower();
	for (const char *suffix : TEXT_SUFFIXES) {
		if (extension == suffix) {
			return true;
		}
	}
	return false;
}

// The hash of a project file as the builder hashes the embedded one: text files with LF line endings, so a checkout
// that turned them into CRLF still matches (review finding 16).
String file_hash(const String &p_path) {
	if (!FileAccess::exists(p_path)) {
		return String();
	}
	if (!is_text_file(p_path)) {
		return FileAccess::get_sha256(p_path);
	}
	const PackedByteArray raw = FileAccess::get_file_as_bytes(p_path);
	PackedByteArray lf;
	lf.resize(raw.size());
	int64_t size = 0;
	for (int64_t i = 0; i < raw.size(); i++) {
		if (raw[i] == '\r' && i + 1 < raw.size() && raw[i + 1] == '\n') {
			continue;
		}
		lf.set(size++, raw[i]);
	}
	lf.resize(size);
	return sha256_hex(lf);
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
		if (embedded.size > 0) { // An empty file has nothing to decompress (review finding 30).
			file.content.resize(embedded.size);
			const int64_t size = Compression::decompress(file.content.ptrw(), embedded.size, embedded.data, embedded.compressed_size, Compression::MODE_DEFLATE);
			ERR_CONTINUE_MSG(size != embedded.size, vformat("JRPG managed addon: \"%s\" is damaged in this build.", embedded.path));
		}
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
	if (p_directory.is_empty()) {
		report["error"] = "No folder to install the addon in.";
		return report;
	}
	const String stamp_path = p_directory.path_join(STAMP_FILE);
	Ref<ConfigFile> stamp;
	stamp.instantiate();
	stamp->load(stamp_path); // Missing on a first install.
	bool stamp_changed = String(stamp->get_value("addon", "version", "")) != p_version;
	const auto record = [&](const String &p_path, const String &p_hash) {
		if (String(stamp->get_value("files", p_path, "")) != p_hash) {
			stamp->set_value("files", p_path, p_hash);
			stamp_changed = true;
		}
	};

	PackedStringArray added, replaced, removed, unchanged, kept_modified, failed;
	HashSet<String> shipped;
	for (const SourceFile &file : p_files) {
		if (file.path.is_empty() || file.path.contains("..") || file.path.is_absolute_path() || file.path == STAMP_FILE) {
			failed.push_back(file.path);
			continue;
		}
		shipped.insert(file.path);
		const String target = p_directory.path_join(file.path);
		const String wanted = sha256_hex(file.content);
		const String current = file_hash(target);
		const String recorded = stamp->get_value("files", file.path, "");
		if (current == wanted) {
			unchanged.push_back(file.path);
			record(file.path, wanted);
			continue;
		}
		if (!current.is_empty() && current != recorded) {
			// Changed by the user since it was installed (or not installed by us): keep it, keep the old record.
			kept_modified.push_back(file.path);
			continue;
		}
		Ref<FileAccess> out;
		if (DirAccess::make_dir_recursive_absolute(target.get_base_dir()) == OK) {
			out = FileAccess::open(target, FileAccess::WRITE);
		}
		if (out.is_null()) {
			failed.push_back(file.path); // Reported, not only logged (review finding 29).
			continue;
		}
		out->store_buffer(file.content);
		out.unref();
		(current.is_empty() ? added : replaced).push_back(file.path);
		record(file.path, wanted);
	}
	// Files an earlier version installed that this one no longer ships: removed when untouched, kept when the user
	// changed them (review finding 30).
	if (stamp->has_section("files")) {
		for (const String &path : stamp->get_section_keys("files")) {
			if (shipped.has(path) || path.contains("..") || path.is_absolute_path()) {
				continue;
			}
			const String target = p_directory.path_join(path);
			const String current = file_hash(target);
			if (current.is_empty() || current == String(stamp->get_value("files", path, ""))) {
				if (!current.is_empty() && DirAccess::remove_absolute(target) != OK) {
					failed.push_back(path);
					continue;
				}
				stamp->erase_section_key("files", path);
				stamp_changed = true;
				if (!current.is_empty()) {
					removed.push_back(path);
				}
			} else {
				kept_modified.push_back(path);
			}
		}
	}
	report["version"] = p_version;
	report["added"] = added;
	report["replaced"] = replaced;
	report["removed"] = removed;
	report["unchanged"] = unchanged;
	report["kept_modified"] = kept_modified;
	report["failed"] = failed;
	if (stamp_changed) { // Written only when something changed (review finding 30).
		stamp->set_value("addon", "version", p_version);
		if (stamp->save(stamp_path) != OK) {
			report["error"] = vformat("Could not write %s. Check that the project folder is writable, then install again.", stamp_path);
		}
	}
	if (!failed.is_empty() && !report.has("error")) {
		report["error"] = vformat("Could not install %s. Check that the project folder is writable, then install again.", String(", ").join(failed));
	}
	if (p_directory.begins_with("res://") && EditorFileSystem::get_singleton() && (!added.is_empty() || !replaced.is_empty() || !removed.is_empty())) {
		EditorFileSystem::get_singleton()->scan_changes(); // Let the FileSystem dock and the importers see the files.
	}
	return report;
}

void JRPGManagedAddon::_bind_methods() {
	ClassDB::bind_static_method("JRPGManagedAddon", D_METHOD("get_embedded_version"), &JRPGManagedAddon::get_embedded_version);
	ClassDB::bind_static_method("JRPGManagedAddon", D_METHOD("get_embedded_files"), &JRPGManagedAddon::get_embedded_files);
	ClassDB::bind_static_method("JRPGManagedAddon", D_METHOD("get_status", "directory"), &JRPGManagedAddon::get_status, DEFVAL(DEFAULT_DIRECTORY));
	ClassDB::bind_static_method("JRPGManagedAddon", D_METHOD("install", "directory"), &JRPGManagedAddon::install, DEFVAL(DEFAULT_DIRECTORY));
}
