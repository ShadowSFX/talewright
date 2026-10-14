/**************************************************************************/
/*  test_jrpg_random.h                                                    */
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

#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_random.h"

namespace TestJRPGRandom {

TEST_CASE("[JRPG][Random] PCG32 reference sequence") {
	// The published PCG32 demo seeds (initstate 42, sequence 54); values checked against an independent
	// implementation of the algorithm.
	Ref<JRPGRandom> rng;
	rng.instantiate();
	rng->seed(42, 54);
	const uint32_t expected[] = { 0xa15c02b7, 0x7b47f409, 0xba1d3330, 0x83d2f293, 0xbfa4784b, 0xcbed606e };
	for (uint32_t value : expected) {
		CHECK(rng->next_u32() == value);
	}
}

TEST_CASE("[JRPG][Random] Range and chance boundaries") {
	Ref<JRPGRandom> rng;
	rng.instantiate();
	rng->seed(7);
	for (int i = 0; i < 1000; i++) {
		CHECK(rng->range(5, 5) == 5);
		const int64_t v = rng->range(-3, 3);
		CHECK(v >= -3);
		CHECK(v <= 3);
		CHECK_FALSE(rng->chance_bp(0));
		CHECK_FALSE(rng->chance_bp(-50));
		CHECK(rng->chance_bp(10000));
		CHECK(rng->chance_bp(25000));
	}
	const int64_t wide = rng->range(0, 4294967295LL); // The full 32-bit span is allowed.
	CHECK(wide >= 0);
	CHECK(wide <= 4294967295LL);
	ERR_PRINT_OFF;
	CHECK(rng->range(9, 2) == 9); // Inverted range: refused, returns low.
	ERR_PRINT_ON;

	int hits = 0;
	for (int i = 0; i < 100000; i++) {
		hits += rng->chance_bp(2500) ? 1 : 0;
	}
	CHECK(hits > 24000);
	CHECK(hits < 26000);
}

TEST_CASE("[JRPG][Random] Uniformity of range(0, 5) (chi-square)") {
	Ref<JRPGRandom> rng;
	rng.instantiate();
	rng->seed(2026);
	int counts[6] = {};
	const int draws = 100000;
	for (int i = 0; i < draws; i++) {
		counts[rng->range(0, 5)]++;
	}
	const double expected = draws / 6.0;
	double chi2 = 0;
	for (int c : counts) {
		chi2 += (c - expected) * (c - expected) / expected;
	}
	// 5 degrees of freedom: the 0.1 % critical value is 20.52. Deterministic for this seed.
	CHECK_MESSAGE(chi2 < 20.52, "chi-square = ", chi2);
}

TEST_CASE("[JRPG][Random] Weighted pick and shuffle") {
	Ref<JRPGRandom> rng;
	rng.instantiate();
	rng->seed(99);
	CHECK(rng->pick_weighted(PackedInt32Array()) == -1);
	CHECK(rng->pick_weighted(PackedInt32Array({ 0, -5, 0 })) == -1);
	for (int i = 0; i < 200; i++) {
		CHECK(rng->pick_weighted(PackedInt32Array({ 0, 3, 0 })) == 1);
	}
	int first = 0;
	for (int i = 0; i < 30000; i++) {
		first += rng->pick_weighted(PackedInt32Array({ 1, 2 })) == 0 ? 1 : 0;
	}
	CHECK(first > 9500); // About one third.
	CHECK(first < 10500);

	Array a = { 1, 2, 3, 4, 5, 6, 7, 8 };
	rng->shuffle(a);
	CHECK(a.size() == 8);
	int sum = 0;
	for (int i = 0; i < a.size(); i++) {
		sum += int(a[i]);
	}
	CHECK(sum == 36); // Same elements, new order.
}

TEST_CASE("[JRPG][Random] Streams are independent and their state round-trips") {
	Ref<JRPGRandomStreams> a;
	a.instantiate();
	a->seed_all(1234);
	Ref<JRPGRandomStreams> b;
	b.instantiate();
	b->seed_all(1234);

	// Drawing from map_ai in one copy does not change the next battle draw.
	for (int i = 0; i < 50; i++) {
		a->get_stream(JRPGRandomStreams::STREAM_MAP_AI)->next_u32();
	}
	CHECK(a->get_stream(JRPGRandomStreams::STREAM_BATTLE)->next_u32() == b->get_stream(JRPGRandomStreams::STREAM_BATTLE)->next_u32());
	// Different streams give different sequences for the same seed.
	CHECK(b->get_stream(JRPGRandomStreams::STREAM_LOOT)->next_u32() != b->get_stream(JRPGRandomStreams::STREAM_ENCOUNTER)->next_u32());

	const Dictionary saved = a->serialize();
	CHECK(saved.has("battle"));
	CHECK(saved.has("map_ai"));
	const uint32_t next_battle = a->get_stream(JRPGRandomStreams::STREAM_BATTLE)->next_u32();
	const uint32_t next_ai = a->get_stream(JRPGRandomStreams::STREAM_MAP_AI)->next_u32();

	Ref<JRPGRandomStreams> restored;
	restored.instantiate();
	CHECK(restored->deserialize(saved));
	CHECK(restored->get_stream(JRPGRandomStreams::STREAM_BATTLE)->next_u32() == next_battle);
	CHECK(restored->get_stream(JRPGRandomStreams::STREAM_MAP_AI)->next_u32() == next_ai);

	Dictionary bad;
	Dictionary even_inc;
	even_inc["state"] = 1;
	even_inc["inc"] = 2; // PCG increments are always odd.
	bad["battle"] = even_inc;
	bad["loot"] = "nope";
	ERR_PRINT_OFF;
	CHECK_FALSE(restored->deserialize(bad));
	ERR_PRINT_ON;
}

} // namespace TestJRPGRandom
