/**************************************************************************/
/*  jrpg_misc_data.h                                                      */
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

#include "jrpg_entry.h"

// An animation played in battle or on the map (RM-ANI-*). Only the reference to its effect exists so far; timing,
// sounds and flashes join with the animation epic.
class JRPGAnimationData : public JRPGEntry {
	GDCLASS(JRPGAnimationData, JRPGEntry);

	String effect; // uid:// or res:// path of the effect scene or resource.

protected:
	static void _bind_methods();

public:
	void set_effect(const String &p_effect);
	String get_effect() const { return effect; }
};

// A tileset: the Godot TileSet a map uses, the ordered tile palette map cells index into, and the tile flags baked
// from the TileSet's custom data so core code never needs the TileSet itself (plan/specs/05_maps_tiles.md).
class JRPGTilesetData : public JRPGEntry {
	GDCLASS(JRPGTilesetData, JRPGEntry);

public:
	enum EdgeMode {
		EDGE_EXTEND, // Outside the map counts as the same autotile kind.
		EDGE_BORDER, // Outside the map counts as a different kind.
	};

	// Bit layout of one baked flag word (spec 05, "Tile flags").
	enum FlagBits : uint32_t {
		FLAG_PASS_MASK = 0x3, // 0 open, 1 blocked, 2 transparent.
		FLAG_NO_ENTRY_SHIFT = 2, // 4 bits: sides N, E, S, W that cannot be entered.
		FLAG_NO_EXIT_SHIFT = 6, // 4 bits: sides that cannot be left.
		FLAG_OVERHEAD = 1u << 10,
		FLAG_LADDER = 1u << 11,
		FLAG_BUSH = 1u << 12,
		FLAG_COUNTER = 1u << 13,
		FLAG_DAMAGE = 1u << 14,
		FLAG_TERRAIN_TAG_SHIFT = 16, // 4 bits: terrain tag 0..15.
	};
	enum PassMode {
		PASS_OPEN,
		PASS_BLOCKED,
		PASS_TRANSPARENT,
	};

private:
	String tileset_uid;
	Array palette; // Dictionaries {kind, name, source_id, atlas_coords, alternative[, frames]}; palette index = position + 1.
	PackedInt32Array flags; // One baked word per palette index (index 0 = empty cell).
	String flags_hash; // Hash of the TileSet custom data the flags were baked from.
	EdgeMode edge_mode = EDGE_EXTEND;

protected:
	static void _bind_methods();

public:
	void set_tileset_uid(const String &p_uid);
	String get_tileset_uid() const { return tileset_uid; }
	void set_palette(const Array &p_palette);
	Array get_palette() const { return palette; }
	void set_flags(const PackedInt32Array &p_flags);
	PackedInt32Array get_flags() const { return flags; }
	void set_flags_hash(const String &p_hash);
	String get_flags_hash() const { return flags_hash; }
	void set_edge_mode(EdgeMode p_mode);
	EdgeMode get_edge_mode() const { return edge_mode; }

	// Flag word of a palette index; 0 (open, no flags) for the empty cell and unknown indices.
	int get_tile_flags(int p_palette_index) const;
	static int pack_flags(PassMode p_pass, int p_no_entry, int p_no_exit, int p_flags, int p_terrain_tag);
	static PassMode get_pass_mode(int p_flags) { return PassMode(p_flags & FLAG_PASS_MASK); }
	static int get_no_entry(int p_flags) { return (p_flags >> FLAG_NO_ENTRY_SHIFT) & 0xF; }
	static int get_no_exit(int p_flags) { return (p_flags >> FLAG_NO_EXIT_SHIFT) & 0xF; }
	static int get_terrain_tag(int p_flags) { return (p_flags >> FLAG_TERRAIN_TAG_SHIFT) & 0xF; }
};

// A common event: a command list callable from anywhere, or run automatically while a switch is on (RM-CEV-*). Its
// command list joins with the event command model (epic 9).
class JRPGCommonEventData : public JRPGEntry {
	GDCLASS(JRPGCommonEventData, JRPGEntry);

public:
	enum Trigger {
		TRIGGER_NONE,
		TRIGGER_AUTORUN,
		TRIGGER_PARALLEL,
	};

private:
	Trigger trigger = TRIGGER_NONE;
	int switch_id = 1;

protected:
	static void _bind_methods();

public:
	void set_trigger(Trigger p_trigger);
	Trigger get_trigger() const { return trigger; }
	void set_switch_id(int p_id);
	int get_switch_id() const { return switch_id; }
};

VARIANT_ENUM_CAST(JRPGTilesetData::EdgeMode);
VARIANT_ENUM_CAST(JRPGTilesetData::PassMode);
VARIANT_ENUM_CAST(JRPGCommonEventData::Trigger);
