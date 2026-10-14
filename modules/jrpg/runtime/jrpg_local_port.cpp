/**************************************************************************/
/*  jrpg_local_port.cpp                                                   */
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

#include "jrpg_local_port.h"

#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "scene/resources/texture.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_message_text.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/runtime/jrpg_choice_window.h"
#include "modules/jrpg/runtime/jrpg_gold_window.h"
#include "modules/jrpg/runtime/jrpg_message_window.h"
#include "modules/jrpg/runtime/jrpg_number_input_window.h"

namespace {

const StringName &kind_message() {
	static const StringName kind("message");
	return kind;
}

const StringName &kind_choices() {
	static const StringName kind("choices");
	return kind;
}

const StringName &kind_number() {
	static const StringName kind("number");
	return kind;
}

// p_params[p_key] as an int from p_min to p_max (p_default when it is not an int).
int read_int(const Dictionary &p_params, const char *p_key, int p_default, int p_min, int p_max) {
	const Variant value = p_params.get(p_key, p_default);
	return value.get_type() == Variant::INT ? CLAMP(int(int64_t(value)), p_min, p_max) : p_default;
}

} // namespace

JRPGMessageWindow *JRPGLocalPort::_message_window() const {
	return Object::cast_to<JRPGMessageWindow>(ObjectDB::get_instance(message_window));
}

JRPGChoiceWindow *JRPGLocalPort::_choice_window() const {
	return Object::cast_to<JRPGChoiceWindow>(ObjectDB::get_instance(choice_window));
}

JRPGNumberInputWindow *JRPGLocalPort::_number_window() const {
	return Object::cast_to<JRPGNumberInputWindow>(ObjectDB::get_instance(number_window));
}

JRPGGoldWindow *JRPGLocalPort::_gold_window() const {
	return Object::cast_to<JRPGGoldWindow>(ObjectDB::get_instance(gold_window));
}

String JRPGLocalPort::_expand(const String &p_text) const {
	return game_state.is_valid() ? JRPGMessageText::expand(p_text, game_state) : p_text;
}

void JRPGLocalPort::set_windows(JRPGMessageWindow *p_message, JRPGChoiceWindow *p_choices, JRPGNumberInputWindow *p_number, JRPGGoldWindow *p_gold) {
	if (JRPGMessageWindow *old = _message_window()) {
		old->disconnect(SNAME("message_finished"), callable_mp(this, &JRPGLocalPort::_on_message_finished));
		old->disconnect(SNAME("gold_window_requested"), callable_mp(this, &JRPGLocalPort::_on_gold_window_requested));
	}
	if (JRPGChoiceWindow *old = _choice_window()) {
		old->disconnect(SNAME("choice_made"), callable_mp(this, &JRPGLocalPort::_on_choice_made));
	}
	if (JRPGNumberInputWindow *old = _number_window()) {
		old->disconnect(SNAME("number_entered"), callable_mp(this, &JRPGLocalPort::_on_number_entered));
	}
	message_window = p_message ? p_message->get_instance_id() : ObjectID();
	choice_window = p_choices ? p_choices->get_instance_id() : ObjectID();
	number_window = p_number ? p_number->get_instance_id() : ObjectID();
	gold_window = p_gold ? p_gold->get_instance_id() : ObjectID();
	if (p_message) {
		p_message->connect(SNAME("message_finished"), callable_mp(this, &JRPGLocalPort::_on_message_finished));
		p_message->connect(SNAME("gold_window_requested"), callable_mp(this, &JRPGLocalPort::_on_gold_window_requested));
	}
	if (p_choices) {
		p_choices->connect(SNAME("choice_made"), callable_mp(this, &JRPGLocalPort::_on_choice_made));
	}
	if (p_number) {
		p_number->connect(SNAME("number_entered"), callable_mp(this, &JRPGLocalPort::_on_number_entered));
	}
}

void JRPGLocalPort::set_game_state(const Ref<JRPGGameState> &p_state) {
	game_state = p_state;
}

Ref<JRPGGameState> JRPGLocalPort::get_game_state() const {
	return game_state;
}

void JRPGLocalPort::show_request(int p_id, const StringName &p_kind, const Dictionary &p_params) {
	showing = p_kind;
	if (p_kind == kind_message()) {
		if (JRPGMessageWindow *window = _message_window()) {
			Ref<Texture2D> face;
			const String face_path = p_params.get("face", String());
			if (!face_path.is_empty()) {
				face = ResourceLoader::load(face_path);
				if (face.is_null()) {
					WARN_PRINT(vformat("JRPG: the face %s of a message cannot be loaded; the message shows without it.", face_path));
				}
			}
			window->show_message(_expand(p_params.get("text", String())), face, _expand(p_params.get("speaker", String())),
					JRPGMessageWindow::Background(read_int(p_params, "background", JRPGMessageWindow::BACKGROUND_WINDOW, 0, JRPGMessageWindow::BACKGROUND_TRANSPARENT)),
					JRPGMessageWindow::Placement(read_int(p_params, "placement", JRPGMessageWindow::PLACEMENT_BOTTOM, 0, JRPGMessageWindow::PLACEMENT_BOTTOM)));
			return;
		}
	} else if (p_kind == kind_choices()) {
		if (JRPGChoiceWindow *window = _choice_window()) {
			PackedStringArray choices;
			const Variant given = p_params.get("choices", Variant());
			if (given.get_type() == Variant::ARRAY || given.get_type() == Variant::PACKED_STRING_ARRAY) {
				for (const String &choice : PackedStringArray(given)) {
					choices.push_back(_expand(choice));
				}
			}
			window->show_choices(choices, read_int(p_params, "default", 0, -1, INT32_MAX), read_int(p_params, "cancel", JRPGChoiceWindow::CANCEL_DISALLOWED, JRPGChoiceWindow::CANCEL_DISALLOWED, INT32_MAX),
					JRPGChoiceWindow::Placement(read_int(p_params, "placement", JRPGChoiceWindow::PLACEMENT_RIGHT, 0, JRPGChoiceWindow::PLACEMENT_RIGHT)),
					JRPGMessageWindow::Background(read_int(p_params, "background", JRPGMessageWindow::BACKGROUND_WINDOW, 0, JRPGMessageWindow::BACKGROUND_TRANSPARENT)));
			return;
		}
	} else if (p_kind == kind_number()) {
		if (JRPGNumberInputWindow *window = _number_window()) {
			window->start(read_int(p_params, "digits", 1, 1, 8), read_int(p_params, "value", 0, 0, INT32_MAX));
			return;
		}
	}
	// No window shows it: answered at once, so the event does not wait for ever.
	if (!unknown_warned.has(p_kind)) {
		unknown_warned.insert(p_kind);
		WARN_PRINT(vformat("JRPG: the game has no window for \"%s\" requests; they go on without an answer.", p_kind));
	}
	showing = StringName();
	reply(p_id, Variant());
}

void JRPGLocalPort::hide_request(int p_id) {
	if (showing == kind_message()) {
		if (JRPGMessageWindow *window = _message_window()) {
			window->close();
		}
		if (JRPGGoldWindow *window = _gold_window()) {
			window->close();
		}
	} else if (showing == kind_choices()) {
		if (JRPGChoiceWindow *window = _choice_window()) {
			window->close();
		}
	} else if (showing == kind_number()) {
		if (JRPGNumberInputWindow *window = _number_window()) {
			window->close();
		}
	}
	showing = StringName();
}

void JRPGLocalPort::_on_message_finished() {
	if (showing == kind_message()) {
		showing = StringName();
		reply(get_shown_request(), true); // The window stays open: the next request may sit beside it.
	}
}

void JRPGLocalPort::_on_choice_made(int p_index) {
	if (showing == kind_choices()) {
		showing = StringName();
		if (JRPGChoiceWindow *window = _choice_window()) {
			window->close();
		}
		reply(get_shown_request(), p_index);
	}
}

void JRPGLocalPort::_on_number_entered(int p_value) {
	if (showing == kind_number()) {
		showing = StringName();
		if (JRPGNumberInputWindow *window = _number_window()) {
			window->close();
		}
		reply(get_shown_request(), p_value);
	}
}

void JRPGLocalPort::_on_gold_window_requested() {
	JRPGGoldWindow *window = _gold_window();
	if (!window || game_state.is_null()) {
		return;
	}
	const Ref<JRPGDatabase> db = game_state->get_database();
	const Ref<JRPGSystemData> system = db.is_valid() ? db->get_system() : Ref<JRPGSystemData>();
	window->show_gold(game_state->get_gold(), system.is_valid() ? system->get_currency_unit() : String());
}

void JRPGLocalPort::step() {
	const StringName kind = get_shown_kind();
	if (kind == kind_message() || kind == kind_choices() || kind == kind_number()) {
		return; // The message window is in use, or choices or a number sit beside it.
	}
	JRPGMessageWindow *window = _message_window();
	if (window && !window->is_closed() && !window->is_busy()) {
		window->close();
		if (JRPGGoldWindow *gold = _gold_window()) {
			gold->close();
		}
	}
}

void JRPGLocalPort::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_game_state", "state"), &JRPGLocalPort::set_game_state);
	ClassDB::bind_method(D_METHOD("get_game_state"), &JRPGLocalPort::get_game_state);
	ClassDB::bind_method(D_METHOD("step"), &JRPGLocalPort::step);
}
