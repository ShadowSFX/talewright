/**************************************************************************/
/*  jrpg_random.cpp                                                       */
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

#include "jrpg_random.h"

#include "core/object/class_db.h"

void JRPGRandom::seed(uint64_t p_seed, uint64_t p_sequence) {
	pcg32_srandom_r(&pcg, p_seed, p_sequence);
}

uint32_t JRPGRandom::next_u32() {
	return pcg32_random_r(&pcg);
}

int64_t JRPGRandom::range(int64_t p_low, int64_t p_high) {
	ERR_FAIL_COND_V_MSG(p_low > p_high, p_low, "JRPGRandom::range: low is greater than high.");
	const uint64_t span = (uint64_t(p_high) - uint64_t(p_low)) + 1; // Unsigned: no signed overflow for any range.
	ERR_FAIL_COND_V_MSG(span > (uint64_t(1) << 32) || span == 0, p_low, "JRPGRandom::range: the range is wider than 2^32.");
	if (span == (uint64_t(1) << 32)) {
		return p_low + int64_t(next_u32());
	}
	return p_low + int64_t(pcg32_boundedrand_r(&pcg, uint32_t(span)));
}

bool JRPGRandom::chance_bp(int p_bp) {
	if (p_bp <= 0) {
		return false;
	}
	if (p_bp >= 10000) {
		return true;
	}
	return range(0, 9999) < p_bp;
}

int JRPGRandom::pick_weighted(const PackedInt32Array &p_weights) {
	int64_t total = 0;
	for (int w : p_weights) {
		total += MAX(0, w);
	}
	if (total == 0) {
		return -1;
	}
	ERR_FAIL_COND_V_MSG(total > int64_t(UINT32_MAX), -1, "JRPGRandom::pick_weighted: total weight is too large.");
	int64_t roll = range(0, total - 1);
	for (int i = 0; i < p_weights.size(); i++) {
		roll -= MAX(0, p_weights[i]);
		if (roll < 0) {
			return i;
		}
	}
	return p_weights.size() - 1; // Not reached.
}

void JRPGRandom::shuffle(Array p_array) {
	for (int i = p_array.size() - 1; i > 0; i--) {
		const int j = int(range(0, i));
		const Variant tmp = p_array[i];
		p_array[i] = p_array[j];
		p_array[j] = tmp;
	}
}

Dictionary JRPGRandom::get_state() const {
	Dictionary d;
	// Stored as signed 64-bit integers (Variant INT); the bits are restored unchanged.
	d["state"] = int64_t(pcg.state);
	d["inc"] = int64_t(pcg.inc);
	return d;
}

void JRPGRandom::set_state(const Dictionary &p_state) {
	ERR_FAIL_COND_MSG(!p_state.has("state") || !p_state.has("inc"), "JRPGRandom::set_state: missing state or inc.");
	ERR_FAIL_COND_MSG(p_state["state"].get_type() != Variant::INT || p_state["inc"].get_type() != Variant::INT, "JRPGRandom::set_state: state and inc must be integers.");
	const uint64_t inc = uint64_t(int64_t(p_state["inc"]));
	ERR_FAIL_COND_MSG((inc & 1) == 0, "JRPGRandom::set_state: inc must be odd.");
	pcg.state = uint64_t(int64_t(p_state["state"]));
	pcg.inc = inc;
}

void JRPGRandom::_bind_methods() {
	ClassDB::bind_method(D_METHOD("seed", "seed", "sequence"), &JRPGRandom::seed, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("next_u32"), &JRPGRandom::next_u32);
	ClassDB::bind_method(D_METHOD("range", "low", "high"), &JRPGRandom::range);
	ClassDB::bind_method(D_METHOD("chance_bp", "bp"), &JRPGRandom::chance_bp);
	ClassDB::bind_method(D_METHOD("pick_weighted", "weights"), &JRPGRandom::pick_weighted);
	ClassDB::bind_method(D_METHOD("shuffle", "array"), &JRPGRandom::shuffle);
	ClassDB::bind_method(D_METHOD("get_state"), &JRPGRandom::get_state);
	ClassDB::bind_method(D_METHOD("set_state", "state"), &JRPGRandom::set_state);
}

static const char *STREAM_NAMES[JRPGRandomStreams::STREAM_MAX] = { "battle", "map_ai", "encounter", "loot", "misc" };

String JRPGRandomStreams::get_stream_name(Stream p_stream) {
	ERR_FAIL_INDEX_V(p_stream, STREAM_MAX, String());
	return STREAM_NAMES[p_stream];
}

void JRPGRandomStreams::seed_all(uint64_t p_seed) {
	for (int i = 0; i < STREAM_MAX; i++) {
		streams[i]->seed(p_seed, uint64_t(i) + 1);
	}
}

Ref<JRPGRandom> JRPGRandomStreams::get_stream(Stream p_stream) const {
	ERR_FAIL_INDEX_V(p_stream, STREAM_MAX, Ref<JRPGRandom>());
	return streams[p_stream];
}

Dictionary JRPGRandomStreams::serialize() const {
	Dictionary d;
	for (int i = 0; i < STREAM_MAX; i++) {
		d[STREAM_NAMES[i]] = streams[i]->get_state();
	}
	return d;
}

bool JRPGRandomStreams::deserialize(const Dictionary &p_data) {
	// Everything is checked before anything changes, so a refused input leaves every stream as it was.
	for (int i = 0; i < STREAM_MAX; i++) {
		if (!p_data.has(STREAM_NAMES[i])) {
			continue; // Streams added after the save was written keep their current state.
		}
		const Variant v = p_data[STREAM_NAMES[i]];
		if (v.get_type() != Variant::DICTIONARY) {
			return false;
		}
		const Dictionary s = v;
		if (!s.has("state") || !s.has("inc") || s["state"].get_type() != Variant::INT || s["inc"].get_type() != Variant::INT || (int64_t(s["inc"]) & 1) == 0) {
			return false;
		}
	}
	for (int i = 0; i < STREAM_MAX; i++) {
		if (p_data.has(STREAM_NAMES[i])) {
			streams[i]->set_state(p_data[STREAM_NAMES[i]]);
		}
	}
	return true;
}

JRPGRandomStreams::JRPGRandomStreams() {
	for (int i = 0; i < STREAM_MAX; i++) {
		streams[i].instantiate();
	}
	seed_all(0);
}

void JRPGRandomStreams::_bind_methods() {
	ClassDB::bind_static_method("JRPGRandomStreams", D_METHOD("get_stream_name", "stream"), &JRPGRandomStreams::get_stream_name);
	ClassDB::bind_method(D_METHOD("seed_all", "seed"), &JRPGRandomStreams::seed_all);
	ClassDB::bind_method(D_METHOD("get_stream", "stream"), &JRPGRandomStreams::get_stream);
	ClassDB::bind_method(D_METHOD("serialize"), &JRPGRandomStreams::serialize);
	ClassDB::bind_method(D_METHOD("deserialize", "data"), &JRPGRandomStreams::deserialize);

	BIND_ENUM_CONSTANT(STREAM_BATTLE);
	BIND_ENUM_CONSTANT(STREAM_MAP_AI);
	BIND_ENUM_CONSTANT(STREAM_ENCOUNTER);
	BIND_ENUM_CONSTANT(STREAM_LOOT);
	BIND_ENUM_CONSTANT(STREAM_MISC);
	BIND_ENUM_CONSTANT(STREAM_MAX);
}
