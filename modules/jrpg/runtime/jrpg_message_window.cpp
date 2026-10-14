/**************************************************************************/
/*  jrpg_message_window.cpp                                               */
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

#include "jrpg_message_window.h"

#include "core/input/input_event.h"
#include "core/object/class_db.h"
#include "scene/gui/label.h"
#include "scene/main/viewport.h"
#include "scene/resources/font.h"
#include "scene/theme/theme_db.h"

#include "modules/jrpg/core/jrpg_message_text.h"

void JRPGMessageWindow::ensure_default_theme() {
	ThemeDB *themes = ThemeDB::get_singleton();
	const Ref<Theme> theme = themes ? themes->get_default_theme() : Ref<Theme>();
	const StringName type = get_class_static();
	if (theme.is_null() || theme->has_constant(SNAME("line_height"), type)) {
		return;
	}
	theme->set_block_signals(true); // As JRPGWindow::ensure_default_skin: nothing has read these yet.
	theme->set_font_size(SNAME("font_size"), type, DEFAULT_FONT_SIZE);
	theme->set_constant(SNAME("line_height"), type, DEFAULT_LINE_HEIGHT);
	theme->set_block_signals(false);
}

PackedColorArray JRPGMessageWindow::get_default_text_colors() {
	// Talewright's own palette: 0 normal text, then colors for emphasis, system labels, warnings and gauges.
	static const uint32_t rgb[TEXT_COLOR_COUNT] = {
		0xffffff,
		0x8fc4ff,
		0xff8a75,
		0x8fe08a,
		0xc9a8ff,
		0xffd166,
		0x6fe3d6,
		0xb4b4b4, //
		0x7a7a7a,
		0x4f7cff,
		0xe5485f,
		0x3fae5b,
		0x8f6bff,
		0xf2a93b,
		0x2fb5ad,
		0x262626, //
		0x9ec9ff,
		0xffe066,
		0xff5a5a,
		0x353535,
		0xf2945e,
		0xffcf7a,
		0x5ea2f2,
		0x82d2ff, //
		0xa5e874,
		0xe87878,
		0xd7a6e6,
		0xa6d7e6,
		0x6bd49d,
		0xbcf08f,
		0xc4c4ff,
		0xffc4e2, //
	};
	PackedColorArray colors;
	colors.resize(TEXT_COLOR_COUNT);
	for (int i = 0; i < TEXT_COLOR_COUNT; i++) {
		colors.set(i, Color::hex(rgb[i] << 8 | 0xff));
	}
	return colors;
}

void JRPGMessageWindow::_update_theme_item_cache() {
	ensure_default_theme();
	JRPGWindow::_update_theme_item_cache();
}

void JRPGMessageWindow::set_text_colors(const PackedColorArray &p_colors) {
	text_colors = p_colors.is_empty() ? get_default_text_colors() : p_colors;
	queue_redraw();
}

real_t JRPGMessageWindow::_text_left() const {
	return face.is_valid() ? FACE_SIZE + FACE_SPACING : 0;
}

void JRPGMessageWindow::_layout(const Array &p_ops) {
	pieces.clear();
	events.clear();
	total = 0;
	const Ref<Font> font = theme_cache.font;
	Point2 at(_text_left(), 0);
	int size = CLAMP(theme_cache.font_size, MIN_FONT_SIZE, MAX_FONT_SIZE);
	Color color = text_colors.is_empty() ? Color(1, 1, 1) : text_colors[0];
	for (const Variant &entry : p_ops) {
		const Dictionary op = entry;
		const int kind = op.get("op", -1);
		switch (kind) {
			case JRPGMessageText::OP_TEXT: {
				Piece piece;
				piece.text = op.get("text", "");
				piece.font_size = size;
				piece.color = color;
				piece.position = at;
				piece.width = font.is_valid() ? font->get_string_size(piece.text, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x : 0;
				piece.first = total;
				piece.count = piece.text.length();
				total += piece.count;
				at.x += piece.width;
				pieces.push_back(piece);
			} break;
			case JRPGMessageText::OP_NEW_LINE: {
				at = Point2(_text_left(), at.y + theme_cache.line_height);
			} break;
			case JRPGMessageText::OP_COLOR: {
				const int index = op.get("index", 0);
				color = index >= 0 && index < text_colors.size() ? text_colors[index] : color; // An unknown color changes nothing.
			} break;
			case JRPGMessageText::OP_ICON: {
				Piece piece;
				piece.icon = op.get("index", 0);
				piece.color = Color(1, 1, 1);
				piece.position = at;
				piece.width = get_icon_size();
				piece.first = total;
				piece.count = 1;
				total += 1;
				at.x += piece.width + 4;
				pieces.push_back(piece);
			} break;
			case JRPGMessageText::OP_FONT_BIGGER: {
				size = MIN(size + FONT_SIZE_STEP, MAX_FONT_SIZE);
			} break;
			case JRPGMessageText::OP_FONT_SMALLER: {
				size = MAX(size - FONT_SIZE_STEP, MIN_FONT_SIZE);
			} break;
			case JRPGMessageText::OP_FONT_SIZE: {
				size = CLAMP(int(op.get("size", size)), MIN_FONT_SIZE, MAX_FONT_SIZE);
			} break;
			case JRPGMessageText::OP_POSITION_X: {
				at.x = int(op.get("x", 0));
			} break;
			case JRPGMessageText::OP_POSITION_Y: {
				at.y = int(op.get("y", 0));
			} break;
			default: {
				// The codes that act while the text appears: waits, instant display, the gold window, no input wait.
				Event event;
				event.at = total;
				event.op = kind;
				event.value = op.get("frames", 0);
				events.push_back(event);
			} break;
		}
	}
}

Array JRPGMessageWindow::get_layout() const {
	Array out;
	for (const Piece &piece : pieces) {
		Dictionary entry;
		if (piece.icon >= 0) {
			entry["icon"] = piece.icon;
		} else {
			entry["text"] = piece.text;
			entry["font_size"] = piece.font_size;
			entry["color"] = piece.color;
		}
		entry["position"] = piece.position;
		entry["first"] = piece.first;
		entry["count"] = piece.count;
		out.push_back(entry);
	}
	return out;
}

void JRPGMessageWindow::_apply_placement() {
	if (!is_inside_tree()) {
		return;
	}
	const Size2 screen = get_viewport_rect().size;
	const real_t height = LINES * theme_cache.line_height + JRPGWindow::DEFAULT_PADDING * 2;
	real_t y = screen.y - height;
	if (placement == PLACEMENT_TOP) {
		y = 0;
	} else if (placement == PLACEMENT_MIDDLE) {
		y = Math::round((screen.y - height) / 2);
	}
	set_position(Point2(0, y));
	set_size(Size2(screen.x, height));
	set_frame_visible(background == BACKGROUND_WINDOW);
	set_dim_visible(background == BACKGROUND_DIM);
	_update_name_box();
}

void JRPGMessageWindow::_update_name_box() {
	name_box->set_visible(!speaker.is_empty());
	if (speaker.is_empty()) {
		return;
	}
	name_label->set_text(speaker);
	name_label->add_theme_font_size_override(SceneStringName(font_size), theme_cache.font_size);
	name_box->set_frame_visible(background == BACKGROUND_WINDOW);
	name_box->set_dim_visible(background == BACKGROUND_DIM);
	const Size2 box = name_box->get_combined_minimum_size();
	// Above the message window, or below it when the message is at the top of the screen.
	name_box->set_position(Point2(0, placement == PLACEMENT_TOP ? get_size().y : -box.y));
	name_box->set_size(box);
}

void JRPGMessageWindow::show_message(const String &p_text, const Ref<Texture2D> &p_face, const String &p_speaker, Background p_background, Placement p_placement) {
	face = p_face;
	speaker = p_speaker;
	background = p_background;
	placement = p_placement;
	_layout(JRPGMessageText::parse(p_text));
	shown = 0;
	next_event = 0;
	budget = 0;
	wait_left = 0;
	waiting_input = false;
	instant = false;
	skipping = false;
	no_input_wait = false;
	active = true;
	set_pause_visible(false);
	_apply_placement();
	set_focus_mode(FOCUS_ALL); // Only while a message is under way (epic 10 review B5).
	if (is_inside_tree()) {
		grab_focus();
		set_process_unhandled_input(true);
	}
	if (!is_open() && !is_opening()) {
		open();
	}
	_update_process();
	queue_redraw();
}

void JRPGMessageWindow::_finish() {
	active = false;
	waiting_input = false;
	skipping = false;
	set_pause_visible(false);
	set_process_unhandled_input(false);
	set_focus_mode(FOCUS_NONE); // A finished message takes no clicks or focus (choices may be shown beside it).
	_update_process();
	emit_signal(SNAME("message_finished"));
}

void JRPGMessageWindow::_press() {
	if (!active || !is_open()) {
		return;
	}
	if (waiting_input) {
		waiting_input = false;
		set_pause_visible(false);
		if (shown >= total && next_event >= events.size()) {
			_finish();
		}
		return;
	}
	skipping = true; // The rest at once, up to the next wait for the player.
}

void JRPGMessageWindow::_advance() {
	const double delta = get_process_delta_time();
	if (!active || !is_open() || waiting_input) {
		return;
	}
	if (wait_left > 0) {
		wait_left = skipping ? 0.0 : wait_left - delta;
		if (wait_left > CMP_EPSILON) {
			return;
		}
		wait_left = 0;
	}
	// Frames close to 1/60 s count as exactly one letter, so slight jitter does not give 0 then 2 letters; after a long
	// frame the text catches up a few letters at a time (epic 10 review B12).
	budget += delta * 60.0;
	const double whole = Math::round(budget);
	if (Math::abs(budget - whole) < 0.05) {
		budget = whole;
	}
	budget = MIN(budget, double(MAX_LETTERS_PER_FRAME));
	while (true) {
		while (next_event < events.size() && events[next_event].at == shown) {
			const Event event = events[next_event++];
			switch (event.op) {
				case JRPGMessageText::OP_WAIT: {
					if (!skipping) {
						wait_left = event.value / 60.0;
						budget = 0;
						return;
					}
				} break;
				case JRPGMessageText::OP_WAIT_INPUT: {
					waiting_input = true;
					skipping = false;
					set_pause_visible(true);
					budget = 0;
					return;
				}
				case JRPGMessageText::OP_INSTANT_ON: {
					instant = true;
				} break;
				case JRPGMessageText::OP_INSTANT_OFF: {
					instant = false;
				} break;
				case JRPGMessageText::OP_GOLD_WINDOW: {
					emit_signal(SNAME("gold_window_requested"));
				} break;
				case JRPGMessageText::OP_NO_INPUT_WAIT: {
					no_input_wait = true;
				} break;
				default:
					break;
			}
		}
		if (shown >= total) {
			budget = 0;
			if (no_input_wait) {
				_finish();
			} else {
				waiting_input = true;
				skipping = false;
				set_pause_visible(true);
			}
			return;
		}
		if (!instant && !skipping) {
			// A hair of tolerance, so 60 steps of 1/60 s show 60 characters.
			if (budget + CMP_EPSILON < 1.0) {
				return;
			}
			budget -= 1.0;
		}
		shown++;
		queue_redraw();
	}
}

void JRPGMessageWindow::gui_input(const Ref<InputEvent> &p_event) {
	if (input_reader.handle_event(p_event, has_focus()) >= 0) {
		accept_event(); // Its actions, and the focus keys: the focus stays while the message is under way.
		return;
	}
	const Ref<InputEventMouseButton> button = p_event;
	const bool click = button.is_valid() && button->is_pressed() && (button->get_button_index() == MouseButton::LEFT || button->get_button_index() == MouseButton::RIGHT);
	if (click && active) {
		_press();
		accept_event();
	}
}

void JRPGMessageWindow::unhandled_input(const Ref<InputEvent> &p_event) {
	// A click anywhere a control does not stop it: left as OK, right as cancel, both read the message (RM-INP-001).
	const Ref<InputEventMouseButton> button = p_event;
	const bool click = button.is_valid() && button->is_pressed() && (button->get_button_index() == MouseButton::LEFT || button->get_button_index() == MouseButton::RIGHT);
	if (click && active) {
		_press();
		get_viewport()->set_input_as_handled();
	}
}

void JRPGMessageWindow::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (active) {
				_apply_placement();
			}
		} break;
		case NOTIFICATION_FOCUS_ENTER: {
			input_reader.reset(); // Keys down now are not new for this window.
			_update_process();
		} break;
		case NOTIFICATION_FOCUS_EXIT: {
			_update_process();
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (has_focus()) {
				input_reader.update(get_process_delta_time());
				if (input_reader.is_fresh(JRPGInputReader::ACTION_OK) || input_reader.is_fresh(JRPGInputReader::ACTION_CANCEL)) {
					_press();
				}
			}
			_advance();
		} break;
		case NOTIFICATION_DRAW: {
			const Rect2 content = get_content_rect();
			if (!is_open()) {
				break; // The text shows once the window is open.
			}
			if (face.is_valid()) {
				const real_t side = MIN(real_t(FACE_SIZE), content.size.y);
				draw_texture_rect(face, Rect2(content.position, Size2(side, side)));
			}
			const Ref<Font> font = theme_cache.font;
			const Ref<Texture2D> icons = get_icon_set_texture();
			for (const Piece &piece : pieces) {
				if (piece.first >= shown) {
					break;
				}
				const Point2 origin = content.position + piece.position;
				if (piece.icon >= 0) {
					const Rect2 region = get_icon_region(piece.icon);
					if (icons.is_valid() && region.has_area()) {
						const real_t side = region.size.y;
						draw_texture_rect_region(icons, Rect2(origin + Point2(0, (theme_cache.line_height - side) / 2), region.size), region);
					}
					continue;
				}
				if (font.is_null()) {
					continue;
				}
				const int letters = MIN(piece.count, shown - piece.first);
				const real_t ascent = font->get_ascent(piece.font_size);
				const real_t height = font->get_height(piece.font_size);
				const Point2 baseline = origin + Point2(0, (theme_cache.line_height - height) / 2 + ascent);
				draw_string(font, baseline, piece.text.substr(0, letters), HORIZONTAL_ALIGNMENT_LEFT, -1, piece.font_size, piece.color);
			}
		} break;
	}
}

void JRPGMessageWindow::_bind_methods() {
	ClassDB::bind_method(D_METHOD("show_message", "text", "face", "speaker", "background", "placement"), &JRPGMessageWindow::show_message, DEFVAL(Ref<Texture2D>()), DEFVAL(String()), DEFVAL(BACKGROUND_WINDOW), DEFVAL(PLACEMENT_BOTTOM));
	ClassDB::bind_method(D_METHOD("is_busy"), &JRPGMessageWindow::is_busy);
	ClassDB::bind_method(D_METHOD("is_waiting_for_input"), &JRPGMessageWindow::is_waiting_for_input);
	ClassDB::bind_method(D_METHOD("get_shown_characters"), &JRPGMessageWindow::get_shown_characters);
	ClassDB::bind_method(D_METHOD("get_total_characters"), &JRPGMessageWindow::get_total_characters);
	ClassDB::bind_method(D_METHOD("get_layout"), &JRPGMessageWindow::get_layout);
	ClassDB::bind_method(D_METHOD("get_face"), &JRPGMessageWindow::get_face);
	ClassDB::bind_method(D_METHOD("get_speaker"), &JRPGMessageWindow::get_speaker);
	ClassDB::bind_method(D_METHOD("get_background"), &JRPGMessageWindow::get_background);
	ClassDB::bind_method(D_METHOD("get_placement"), &JRPGMessageWindow::get_placement);
	ClassDB::bind_method(D_METHOD("get_name_box"), &JRPGMessageWindow::get_name_box);
	ClassDB::bind_method(D_METHOD("set_text_colors", "colors"), &JRPGMessageWindow::set_text_colors);
	ClassDB::bind_method(D_METHOD("get_text_colors"), &JRPGMessageWindow::get_text_colors);
	ClassDB::bind_static_method("JRPGMessageWindow", D_METHOD("get_default_text_colors"), &JRPGMessageWindow::get_default_text_colors);
	ClassDB::bind_static_method("JRPGMessageWindow", D_METHOD("ensure_default_theme"), &JRPGMessageWindow::ensure_default_theme);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_COLOR_ARRAY, "text_colors"), "set_text_colors", "get_text_colors");

	ADD_SIGNAL(MethodInfo("message_finished"));
	ADD_SIGNAL(MethodInfo("gold_window_requested"));

	BIND_ENUM_CONSTANT(BACKGROUND_WINDOW);
	BIND_ENUM_CONSTANT(BACKGROUND_DIM);
	BIND_ENUM_CONSTANT(BACKGROUND_TRANSPARENT);
	BIND_ENUM_CONSTANT(PLACEMENT_TOP);
	BIND_ENUM_CONSTANT(PLACEMENT_MIDDLE);
	BIND_ENUM_CONSTANT(PLACEMENT_BOTTOM);
	BIND_CONSTANT(LINES);
	BIND_CONSTANT(DEFAULT_LINE_HEIGHT);
	BIND_CONSTANT(DEFAULT_FONT_SIZE);
	BIND_CONSTANT(FONT_SIZE_STEP);
	BIND_CONSTANT(MIN_FONT_SIZE);
	BIND_CONSTANT(MAX_FONT_SIZE);
	BIND_CONSTANT(FACE_SIZE);
	BIND_CONSTANT(FACE_SPACING);
	BIND_CONSTANT(TEXT_COLOR_COUNT);
	BIND_CONSTANT(MAX_LETTERS_PER_FRAME);

	BIND_THEME_ITEM(Theme::DATA_TYPE_FONT, JRPGMessageWindow, font);
	BIND_THEME_ITEM(Theme::DATA_TYPE_FONT_SIZE, JRPGMessageWindow, font_size);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, JRPGMessageWindow, line_height);
}

JRPGMessageWindow::JRPGMessageWindow() {
	set_focus_mode(FOCUS_NONE); // Focusable while a message is under way.
	text_colors = get_default_text_colors();
	name_box = memnew(JRPGWindow);
	name_box->set_name("NameBox");
	name_box->set_visible(false);
	name_box->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(name_box, false, INTERNAL_MODE_FRONT);
	name_label = memnew(Label);
	name_label->set_name("Name");
	name_box->add_child(name_label);
}
