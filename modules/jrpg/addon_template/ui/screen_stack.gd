## The game's screens (task 10.3, D-054): menus, shops, the save screen and other full screens are pushed on top of
## the map and popped back. The screen on top is active and has the focus; the screens under it are paused (their
## process mode is disabled, so their windows read no input) until they are on top again. Pushing, popping and
## replacing fade through black unless told not to.
##
## A screen is any [Control]. It may define [code]_on_screen_activated()[/code] (called when it becomes the top screen)
## and [code]_on_screen_suspended()[/code] (called when another screen covers it). The control that had the focus when
## a screen was covered gets it back when the screen is on top again; a new screen gives the focus to its first
## control that takes it.
class_name JRPGScreenStack
extends CanvasLayer

## Emitted after a screen is put on top.
signal screen_pushed(screen: Control)
## Emitted after a screen is taken off (it is freed unless it was popped with keep).
signal screen_popped(screen: Control)

## Seconds each half of a fade lasts.
@export var fade_seconds := 0.15

var _screens: Array[Control] = []
var _fade: ColorRect
var _fading := false


func _ready() -> void:
	_fade = ColorRect.new()
	_fade.name = "Fade"
	_fade.color = Color(0, 0, 0, 0)
	_fade.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_fade.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(_fade, false, Node.INTERNAL_MODE_BACK)


## The screen on top, or null.
func top() -> Control:
	return null if _screens.is_empty() else _screens.back()


## How many screens are stacked.
func count() -> int:
	return _screens.size()


## The screens, from the bottom one to the top one.
func get_screens() -> Array[Control]:
	return _screens.duplicate()


## True while a fade is under way.
func is_fading() -> bool:
	return _fading


## Puts [param screen] on top; the others pause until it is taken off. With [param fade], await it.
func push(screen: Control, fade := true) -> void:
	if fade:
		await _fade_to(1.0)
	var covered := top()
	if covered:
		_suspend(covered)
	add_child(screen)
	_screens.push_back(screen)
	screen_pushed.emit(screen)
	if fade:
		await _fade_to(0.0)
	_activate(screen)


## Takes the top screen off and returns it; the one under it becomes active. The screen is freed unless
## [param keep]. With [param fade], await it.
func pop(fade := true, keep := false) -> Control:
	if _screens.is_empty():
		return null
	if fade:
		await _fade_to(1.0)
	var screen: Control = _screens.pop_back()
	remove_child(screen)
	screen_popped.emit(screen)
	if not keep:
		screen.queue_free()
	if fade:
		await _fade_to(0.0)
	var below := top()
	if below:
		_activate(below)
	return screen


## Puts [param screen] in place of the top screen (which is freed), in one fade. With [param fade], await it.
func replace(screen: Control, fade := true) -> void:
	if fade:
		await _fade_to(1.0)
	if not _screens.is_empty():
		var old: Control = _screens.pop_back()
		remove_child(old)
		screen_popped.emit(old)
		old.queue_free()
	add_child(screen)
	_screens.push_back(screen)
	screen_pushed.emit(screen)
	if fade:
		await _fade_to(0.0)
	_activate(screen)


## Takes every screen off and frees them, without a fade.
func clear() -> void:
	while not _screens.is_empty():
		var screen: Control = _screens.pop_back()
		remove_child(screen)
		screen_popped.emit(screen)
		screen.queue_free()


func _suspend(screen: Control) -> void:
	var owner_of_focus: Control = screen.get_viewport().gui_get_focus_owner() if screen.is_inside_tree() else null
	if owner_of_focus and (owner_of_focus == screen or screen.is_ancestor_of(owner_of_focus)):
		screen.set_meta(&"_jrpg_focus", screen.get_path_to(owner_of_focus))
		owner_of_focus.release_focus()
	screen.process_mode = Node.PROCESS_MODE_DISABLED
	if screen.has_method(&"_on_screen_suspended"):
		screen.call(&"_on_screen_suspended")


func _activate(screen: Control) -> void:
	screen.process_mode = Node.PROCESS_MODE_INHERIT
	var target: Control = null
	if screen.has_meta(&"_jrpg_focus"):
		target = screen.get_node_or_null(screen.get_meta(&"_jrpg_focus")) as Control
		screen.remove_meta(&"_jrpg_focus")
	if target == null:
		target = screen if screen.focus_mode != Control.FOCUS_NONE else screen.find_next_valid_focus()
		if target and target != screen and not screen.is_ancestor_of(target):
			target = null  # The next focusable control is not this screen's.
	if target:
		target.grab_focus()
	if screen.has_method(&"_on_screen_activated"):
		screen.call(&"_on_screen_activated")


func _fade_to(alpha: float) -> void:
	_fading = true
	var tween := create_tween()
	tween.tween_property(_fade, "color:a", alpha, fade_seconds)
	await tween.finished
	_fading = false
