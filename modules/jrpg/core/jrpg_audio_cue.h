/**************************************************************************/
/*  jrpg_audio_cue.h                                                      */
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

#include "core/io/resource.h"

// A sound or music reference used by database data: the audio file as a uid:// (or res://) string, plus volume,
// pitch and pan. Core code never holds AudioStream objects (plan/specs/01_module_layout.md); the runtime loads the
// stream when it plays the cue.
class JRPGAudioCue : public Resource {
	GDCLASS(JRPGAudioCue, Resource);

	String stream;
	int volume = 100;
	int pitch = 100;
	int pan = 0;

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);

public:
	void set_stream(const String &p_stream);
	String get_stream() const { return stream; }
	void set_volume(int p_volume);
	int get_volume() const { return volume; }
	void set_pitch(int p_pitch);
	int get_pitch() const { return pitch; }
	void set_pan(int p_pan);
	int get_pan() const { return pan; }

	bool is_empty() const { return stream.is_empty(); }
};
