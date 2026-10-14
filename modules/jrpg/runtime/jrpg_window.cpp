/**************************************************************************/
/*  jrpg_window.cpp                                                       */
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

#include "jrpg_window.h"

#include "jrpg_default_skin.gen.h"

#include "core/io/image.h"
#include "core/object/class_db.h"
#include "scene/resources/image_texture.h"
#include "scene/theme/theme_db.h"
#include "servers/rendering/rendering_server.h"

void JRPGWindow::ensure_default_skin() {
	ThemeDB *themes = ThemeDB::get_singleton();
	const Ref<Theme> theme = themes ? themes->get_default_theme() : Ref<Theme>();
	const StringName type = get_class_static();
	if (theme.is_null() || theme->has_icon(SNAME("back"), type)) {
		return;
	}
	struct Part {
		const char *name;
		const unsigned char *data;
		int size;
	};
	const Part parts[] = {
		{ "back", JRPG_SKIN_BACK, JRPG_SKIN_BACK_SIZE },
		{ "border", JRPG_SKIN_BORDER, JRPG_SKIN_BORDER_SIZE },
		{ "cursor", JRPG_SKIN_CURSOR, JRPG_SKIN_CURSOR_SIZE },
		{ "arrows", JRPG_SKIN_ARROWS, JRPG_SKIN_ARROWS_SIZE },
	};
	// No change to announce: no control has read these items yet, and a theme change here would notify every control
	// again from inside this one's theme update (scene/resources/theme.cpp:1679-1688 emits signals only).
	theme->set_block_signals(true);
	for (const Part &part : parts) {
		Vector<uint8_t> bytes;
		bytes.resize(part.size);
		memcpy(bytes.ptrw(), part.data, part.size);
		Ref<Image> image;
		image.instantiate();
		ERR_CONTINUE_MSG(image->load_png_from_buffer(bytes) != OK, vformat("JRPGWindow: the built-in skin's %s image cannot be read.", part.name));
		theme->set_icon(part.name, type, ImageTexture::create_from_image(image));
	}
	theme->set_constant(SNAME("padding"), type, DEFAULT_PADDING);
	theme->set_constant(SNAME("frame_margin"), type, DEFAULT_FRAME_MARGIN);
	theme->set_block_signals(false);
}

void JRPGWindow::_update_theme_item_cache() {
	ensure_default_skin();
	Container::_update_theme_item_cache();
	_update_toned_back();
}

void JRPGWindow::_update_toned_back() {
	toned_back.unref();
	const Ref<Texture2D> back = theme_cache.back;
	if (tone == Vector3i() || back.is_null()) {
		return;
	}
	Ref<Image> image = back->get_image();
	ERR_FAIL_COND_MSG(image.is_null(), "JRPGWindow: the back texture's pixels cannot be read, so the window tone is not applied.");
	// A copy: the texture may share its image (servers/rendering/dummy/storage/texture_storage.h:108-112).
	image = image->duplicate();
	if (image->is_compressed()) {
		image->decompress();
	}
	image->convert(Image::FORMAT_RGBA8);
	uint8_t *pixels = image->ptrw();
	const int64_t count = int64_t(image->get_width()) * image->get_height();
	for (int64_t i = 0; i < count; i++) {
		for (int channel = 0; channel < 3; channel++) {
			pixels[i * 4 + channel] = uint8_t(CLAMP(int(pixels[i * 4 + channel]) + tone[channel], 0, 255));
		}
	}
	toned_back = ImageTexture::create_from_image(image);
}

void JRPGWindow::set_tone(const Vector3i &p_tone) {
	const Vector3i clamped = p_tone.clamp(Vector3i(-255, -255, -255), Vector3i(255, 255, 255));
	if (tone == clamped) {
		return;
	}
	tone = clamped;
	_update_toned_back();
	queue_redraw();
}

void JRPGWindow::_set_openness(double p_openness) {
	const double clamped = CLAMP(p_openness, 0.0, 1.0);
	if (clamped == openness) {
		return;
	}
	openness = clamped;
	queue_redraw();
}

void JRPGWindow::_update_process() {
	set_process_internal(opening != 0 || _needs_process());
}

void JRPGWindow::set_openness(double p_openness) {
	opening = 0;
	_update_process();
	_set_openness(p_openness);
}

void JRPGWindow::open() {
	if (is_open()) {
		opening = 0;
		_update_process();
		emit_signal(SNAME("opened")); // Already open: done at once.
		return;
	}
	opening = 1;
	_update_process();
}

void JRPGWindow::close() {
	if (is_closed()) {
		opening = 0;
		_update_process();
		emit_signal(SNAME("closed"));
		return;
	}
	opening = -1;
	_update_process();
}

void JRPGWindow::set_scroll_up_visible(bool p_visible) {
	scroll_up_visible = p_visible;
	queue_redraw();
}

void JRPGWindow::set_scroll_down_visible(bool p_visible) {
	scroll_down_visible = p_visible;
	queue_redraw();
}

void JRPGWindow::set_pause_visible(bool p_visible) {
	pause_visible = p_visible;
	queue_redraw();
}

Rect2 JRPGWindow::get_content_rect() const {
	const real_t padding = MAX(theme_cache.padding, 0);
	return Rect2(Point2(padding, padding), (get_size() - Size2(padding, padding) * 2).maxf(0));
}

Rect2 JRPGWindow::get_open_rect() const {
	const Size2 size = get_size();
	return Rect2(0, size.y * (1.0 - openness) / 2.0, size.x, size.y * openness);
}

Ref<Texture2D> JRPGWindow::get_back_texture() const {
	return toned_back.is_valid() ? Ref<Texture2D>(toned_back) : theme_cache.back;
}

Size2 JRPGWindow::get_minimum_size() const {
	Size2 minimum;
	for (int i = 0; i < get_child_count(); i++) {
		const Control *child = as_sortable_control(get_child(i), SortableVisibilityMode::VISIBLE);
		if (child) {
			minimum = minimum.max(child->get_combined_minimum_size());
		}
	}
	return minimum + Size2(theme_cache.padding, theme_cache.padding) * 2;
}

void JRPGWindow::_draw_mark(Mark p_mark, const Point2 &p_position) {
	if (theme_cache.arrows.is_null()) {
		return;
	}
	draw_texture_rect_region(theme_cache.arrows, Rect2(p_position, Size2(ARROW_SIZE, ARROW_SIZE)), Rect2(int(p_mark) * ARROW_SIZE, 0, ARROW_SIZE, ARROW_SIZE));
}

void JRPGWindow::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_SORT_CHILDREN: {
			const Rect2 content = get_content_rect();
			for (int i = 0; i < get_child_count(); i++) {
				Control *child = as_sortable_control(get_child(i));
				if (child) {
					fit_child_in_rect(child, content);
				}
			}
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (opening == 0) {
				break; // Processing for a subclass.
			}
			// OPEN_FRAMES frames at 60 a second, whatever the frame rate.
			const double step = get_process_delta_time() * 60.0 / OPEN_FRAMES;
			double next = openness + (opening > 0 ? step : -step);
			if (Math::is_equal_approx(next, Math::round(next))) {
				next = Math::round(next); // OPEN_FRAMES steps of 1/60 s land on the end, not a hair short of it.
			}
			_set_openness(next);
			if ((opening > 0 && is_open()) || (opening < 0 && is_closed())) {
				const bool opened = opening > 0;
				opening = 0;
				_update_process();
				emit_signal(opened ? SNAME("opened") : SNAME("closed"));
			}
		} break;
		case NOTIFICATION_DRAW: {
			RenderingServer *rs = RenderingServer::get_singleton();
			const RID ci = get_canvas_item();
			const Size2 size = get_size();
			if (openness < 1.0) {
				// Control has just set its whole rectangle (scene/gui/control.cpp:4617-4620): the window and its
				// children show through the opening band only, and the frame is squashed into it.
				rs->canvas_item_set_custom_rect(ci, true, get_open_rect());
				rs->canvas_item_set_clip(ci, true);
				rs->canvas_item_add_set_transform(ci, Transform2D(0, Size2(1, openness), 0, Vector2(0, size.y * (1.0 - openness) / 2.0)));
			}
			const Rect2 rect(Point2(), size);
			const Vector2 margin(theme_cache.frame_margin, theme_cache.frame_margin);
			for (const Ref<Texture2D> &layer : { get_back_texture(), theme_cache.border }) {
				if (layer.is_valid()) {
					rs->canvas_item_add_nine_patch(ci, rect, Rect2(Point2(), layer->get_size()), layer->get_rid(), margin, margin);
				}
			}
			if (openness < 1.0) {
				rs->canvas_item_add_set_transform(ci, Transform2D()); // Contents drawn after this are clipped, not squashed.
				break;
			}
			const real_t middle = Math::round((size.x - ARROW_SIZE) / 2);
			if (scroll_up_visible) {
				_draw_mark(MARK_SCROLL_UP, Point2(middle, 2));
			}
			if (scroll_down_visible || pause_visible) {
				_draw_mark(pause_visible ? MARK_PAUSE : MARK_SCROLL_DOWN, Point2(middle, size.y - ARROW_SIZE - 2));
			}
		} break;
	}
}

void JRPGWindow::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tone", "tone"), &JRPGWindow::set_tone);
	ClassDB::bind_method(D_METHOD("get_tone"), &JRPGWindow::get_tone);
	ClassDB::bind_method(D_METHOD("set_openness", "openness"), &JRPGWindow::set_openness);
	ClassDB::bind_method(D_METHOD("get_openness"), &JRPGWindow::get_openness);
	ClassDB::bind_method(D_METHOD("open"), &JRPGWindow::open);
	ClassDB::bind_method(D_METHOD("close"), &JRPGWindow::close);
	ClassDB::bind_method(D_METHOD("is_open"), &JRPGWindow::is_open);
	ClassDB::bind_method(D_METHOD("is_closed"), &JRPGWindow::is_closed);
	ClassDB::bind_method(D_METHOD("is_opening"), &JRPGWindow::is_opening);
	ClassDB::bind_method(D_METHOD("is_closing"), &JRPGWindow::is_closing);
	ClassDB::bind_method(D_METHOD("set_scroll_up_visible", "visible"), &JRPGWindow::set_scroll_up_visible);
	ClassDB::bind_method(D_METHOD("is_scroll_up_visible"), &JRPGWindow::is_scroll_up_visible);
	ClassDB::bind_method(D_METHOD("set_scroll_down_visible", "visible"), &JRPGWindow::set_scroll_down_visible);
	ClassDB::bind_method(D_METHOD("is_scroll_down_visible"), &JRPGWindow::is_scroll_down_visible);
	ClassDB::bind_method(D_METHOD("set_pause_visible", "visible"), &JRPGWindow::set_pause_visible);
	ClassDB::bind_method(D_METHOD("is_pause_visible"), &JRPGWindow::is_pause_visible);
	ClassDB::bind_method(D_METHOD("get_content_rect"), &JRPGWindow::get_content_rect);
	ClassDB::bind_method(D_METHOD("get_open_rect"), &JRPGWindow::get_open_rect);
	ClassDB::bind_method(D_METHOD("get_back_texture"), &JRPGWindow::get_back_texture);
	ClassDB::bind_method(D_METHOD("get_cursor_texture"), &JRPGWindow::get_cursor_texture);
	ClassDB::bind_method(D_METHOD("get_arrows_texture"), &JRPGWindow::get_arrows_texture);
	ClassDB::bind_static_method("JRPGWindow", D_METHOD("ensure_default_skin"), &JRPGWindow::ensure_default_skin);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "tone"), "set_tone", "get_tone");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "openness", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_openness", "get_openness");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_up_visible"), "set_scroll_up_visible", "is_scroll_up_visible");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_down_visible"), "set_scroll_down_visible", "is_scroll_down_visible");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "pause_visible"), "set_pause_visible", "is_pause_visible");

	ADD_SIGNAL(MethodInfo("opened"));
	ADD_SIGNAL(MethodInfo("closed"));

	BIND_CONSTANT(OPEN_FRAMES);
	BIND_CONSTANT(DEFAULT_PADDING);
	BIND_CONSTANT(DEFAULT_FRAME_MARGIN);
	BIND_CONSTANT(ARROW_SIZE);

	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, JRPGWindow, back);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, JRPGWindow, border);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, JRPGWindow, cursor);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, JRPGWindow, arrows);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, JRPGWindow, padding);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, JRPGWindow, frame_margin);
}

JRPGWindow::JRPGWindow() {
	set_mouse_filter(MOUSE_FILTER_STOP); // A window takes the clicks on it (they do not reach the map below).
}
