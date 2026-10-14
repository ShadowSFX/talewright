/**************************************************************************/
/*  jrpg_random.h                                                         */
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
#include "core/variant/typed_array.h"

#include <thirdparty/misc/pcg.h>

// Deterministic integer random numbers for game logic (plan/specs/04_formulas_battle.md, D-010): the PCG32 generator
// vendored by Godot, integer outputs only, with a state that saves and loads exactly. Floats are never drawn here, so
// results are identical on every platform and on clients and servers.
class JRPGRandom : public RefCounted {
	GDCLASS(JRPGRandom, RefCounted);

	pcg32_random_t pcg = { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL };

protected:
	static void _bind_methods();

public:
	// Seeds the generator; p_sequence picks one of 2^63 independent streams for the same seed.
	void seed(uint64_t p_seed, uint64_t p_sequence = 0);
	uint32_t next_u32();
	// Uniform integer in [p_low, p_high], both included. Unbiased (rejection sampling).
	int64_t range(int64_t p_low, int64_t p_high);
	// True with probability p_bp / 10000 (basis points): always for 10000 and above, never for 0 and below.
	bool chance_bp(int p_bp);
	// Index picked with probability proportional to its weight (negative weights count as 0); -1 when all are 0.
	int pick_weighted(const PackedInt32Array &p_weights);
	// Fisher-Yates shuffle in place.
	void shuffle(Array p_array);

	// {state, inc} as two integers, restored exactly by set_state().
	Dictionary get_state() const;
	void set_state(const Dictionary &p_state);
};

// The game's independent random streams (spec 04): battle, map AI, encounters, loot and everything else. Each is its
// own PCG32 sequence, so drawing from one never changes another, and a battle's results do not depend on how many
// NPC steps happened before it.
class JRPGRandomStreams : public RefCounted {
	GDCLASS(JRPGRandomStreams, RefCounted);

public:
	enum Stream {
		STREAM_BATTLE,
		STREAM_MAP_AI,
		STREAM_ENCOUNTER,
		STREAM_LOOT,
		STREAM_MISC,
		STREAM_MAX,
	};

private:
	Ref<JRPGRandom> streams[STREAM_MAX];

protected:
	static void _bind_methods();

public:
	static String get_stream_name(Stream p_stream);

	void seed_all(uint64_t p_seed);
	Ref<JRPGRandom> get_stream(Stream p_stream) const;

	// {stream name: {state, inc}} for saves (spec 06).
	Dictionary serialize() const;
	// Restores the streams present in p_data (others keep their state). A malformed entry changes nothing and
	// returns false.
	bool deserialize(const Dictionary &p_data);

	JRPGRandomStreams();
};

VARIANT_ENUM_CAST(JRPGRandomStreams::Stream);
