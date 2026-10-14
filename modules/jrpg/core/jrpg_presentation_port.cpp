/**************************************************************************/
/*  jrpg_presentation_port.cpp                                            */
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

#include "jrpg_presentation_port.h"

#include "core/object/class_db.h"

int JRPGPresentationPort::_find(int p_id) const {
	for (int i = 0; i < requests.size(); i++) {
		if (requests[i].id == p_id) {
			return i;
		}
	}
	return -1;
}

void JRPGPresentationPort::_show_next() {
	if (shown_id != 0) {
		return;
	}
	for (const Request &request : requests) {
		if (!request.answered) {
			// Set before the presenter runs: it may answer at once, which shows the next request from here.
			shown_id = request.id;
			const StringName kind = request.kind;
			const Dictionary params = request.params;
			emit_signal(SNAME("request_shown"), shown_id, kind, params);
			show_request(shown_id, kind, params);
			return;
		}
	}
}

void JRPGPresentationPort::show_request(int p_id, const StringName &p_kind, const Dictionary &p_params) {
	GDVIRTUAL_CALL(_show, p_id, p_kind, p_params);
}

void JRPGPresentationPort::hide_request(int p_id) {
	GDVIRTUAL_CALL(_hide, p_id);
}

int JRPGPresentationPort::send(const StringName &p_kind, const Dictionary &p_params) {
	Request request;
	request.id = next_id++;
	request.kind = p_kind;
	request.params = p_params.duplicate(true);
	requests.push_back(request);
	_show_next();
	return request.id;
}

bool JRPGPresentationPort::is_answered(int p_id) const {
	const int at = _find(p_id);
	return at >= 0 && requests[at].answered;
}

Variant JRPGPresentationPort::take_reply(int p_id) {
	const int at = _find(p_id);
	if (at < 0 || !requests[at].answered) {
		return Variant();
	}
	const Variant value = requests[at].reply;
	requests.remove_at(at);
	return value;
}

Error JRPGPresentationPort::reply(int p_id, const Variant &p_value) {
	ERR_FAIL_COND_V_MSG(p_id == 0 || p_id != shown_id, ERR_INVALID_PARAMETER, vformat("JRPGPresentationPort: request %d is not the one shown; only the shown request can be answered.", p_id));
	const int at = _find(p_id);
	ERR_FAIL_COND_V(at < 0, ERR_BUG);
	requests.write[at].answered = true;
	requests.write[at].reply = p_value;
	shown_id = 0;
	emit_signal(SNAME("request_answered"), p_id, p_value);
	_show_next();
	return OK;
}

void JRPGPresentationPort::withdraw(int p_id) {
	const int at = _find(p_id);
	if (at < 0) {
		return;
	}
	requests.remove_at(at);
	if (shown_id == p_id) {
		shown_id = 0;
		emit_signal(SNAME("request_hidden"), p_id);
		hide_request(p_id);
		_show_next();
	}
}

bool JRPGPresentationPort::is_busy() const {
	return get_waiting_count() > 0;
}

StringName JRPGPresentationPort::get_shown_kind() const {
	const int at = _find(shown_id);
	return at >= 0 ? requests[at].kind : StringName();
}

Dictionary JRPGPresentationPort::get_shown_params() const {
	const int at = _find(shown_id);
	return at >= 0 ? requests[at].params.duplicate(true) : Dictionary();
}

int JRPGPresentationPort::get_waiting_count() const {
	int waiting = 0;
	for (const Request &request : requests) {
		waiting += request.answered ? 0 : 1;
	}
	return waiting;
}

void JRPGPresentationPort::_bind_methods() {
	ClassDB::bind_method(D_METHOD("send", "kind", "params"), &JRPGPresentationPort::send);
	ClassDB::bind_method(D_METHOD("has_request", "id"), &JRPGPresentationPort::has_request);
	ClassDB::bind_method(D_METHOD("is_answered", "id"), &JRPGPresentationPort::is_answered);
	ClassDB::bind_method(D_METHOD("take_reply", "id"), &JRPGPresentationPort::take_reply);
	ClassDB::bind_method(D_METHOD("reply", "id", "value"), &JRPGPresentationPort::reply);
	ClassDB::bind_method(D_METHOD("withdraw", "id"), &JRPGPresentationPort::withdraw);
	ClassDB::bind_method(D_METHOD("is_busy"), &JRPGPresentationPort::is_busy);
	ClassDB::bind_method(D_METHOD("get_shown_request"), &JRPGPresentationPort::get_shown_request);
	ClassDB::bind_method(D_METHOD("get_shown_kind"), &JRPGPresentationPort::get_shown_kind);
	ClassDB::bind_method(D_METHOD("get_shown_params"), &JRPGPresentationPort::get_shown_params);
	ClassDB::bind_method(D_METHOD("get_waiting_count"), &JRPGPresentationPort::get_waiting_count);

	GDVIRTUAL_BIND(_show, "id", "kind", "params");
	GDVIRTUAL_BIND(_hide, "id");

	ADD_SIGNAL(MethodInfo("request_shown", PropertyInfo(Variant::INT, "id"), PropertyInfo(Variant::STRING_NAME, "kind"), PropertyInfo(Variant::DICTIONARY, "params")));
	ADD_SIGNAL(MethodInfo("request_hidden", PropertyInfo(Variant::INT, "id")));
	ADD_SIGNAL(MethodInfo("request_answered", PropertyInfo(Variant::INT, "id"), PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)));
}
