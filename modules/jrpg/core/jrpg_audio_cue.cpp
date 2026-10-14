/**************************************************************************/
/*  jrpg_audio_cue.cpp                                                    */
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

#include "jrpg_audio_cue.h"

#include "jrpg_schema.h"

#include "core/object/class_db.h"

void JRPGAudioCue::set_stream(const String &p_stream) {
	stream = p_stream;
	emit_changed();
}

void JRPGAudioCue::set_volume(int p_volume) {
	volume = CLAMP(p_volume, 0, 100);
	emit_changed();
}

void JRPGAudioCue::set_pitch(int p_pitch) {
	pitch = CLAMP(p_pitch, 50, 150);
	emit_changed();
}

void JRPGAudioCue::set_pan(int p_pan) {
	pan = CLAMP(p_pan, -100, 100);
	emit_changed();
}

bool JRPGAudioCue::_set(const StringName &p_name, const Variant &p_value) {
	return JRPGSchema::set_renamed(this, p_name, p_value);
}

void JRPGAudioCue::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_stream", "stream"), &JRPGAudioCue::set_stream);
	ClassDB::bind_method(D_METHOD("get_stream"), &JRPGAudioCue::get_stream);
	ClassDB::bind_method(D_METHOD("set_volume", "volume"), &JRPGAudioCue::set_volume);
	ClassDB::bind_method(D_METHOD("get_volume"), &JRPGAudioCue::get_volume);
	ClassDB::bind_method(D_METHOD("set_pitch", "pitch"), &JRPGAudioCue::set_pitch);
	ClassDB::bind_method(D_METHOD("get_pitch"), &JRPGAudioCue::get_pitch);
	ClassDB::bind_method(D_METHOD("set_pan", "pan"), &JRPGAudioCue::set_pan);
	ClassDB::bind_method(D_METHOD("get_pan"), &JRPGAudioCue::get_pan);
	ClassDB::bind_method(D_METHOD("is_empty"), &JRPGAudioCue::is_empty);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "stream", PROPERTY_HINT_FILE, "*.ogg,*.mp3,*.wav"), "set_stream", "get_stream");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "volume", PROPERTY_HINT_RANGE, "0,100,1,suffix:%"), "set_volume", "get_volume");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "pitch", PROPERTY_HINT_RANGE, "50,150,1,suffix:%"), "set_pitch", "get_pitch");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "pan", PROPERTY_HINT_RANGE, "-100,100,1"), "set_pan", "get_pan");
}
