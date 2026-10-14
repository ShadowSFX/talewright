## The game's screens (task 10.3, D-054): menus, shops, the save screen and other full screens are pushed on top of
## the map and popped back. The screen on top is active and has the focus; the screens under it are paused (their
## process mode is disabled, and they take neither the focus nor the mouse, so their windows read no input) until
## they are on top again. Pushing, popping and replacing fade through black unless told not to; from the start of a
## fade to its end the stack refuses other changes (a second press of cancel pops one screen, not two), clicks stop at
## the fade, and the screen on top already reads no input.
##
## A screen is any [Control]. It may define [code]_on_screen_activated()[/code] (called when it becomes the top screen)
## and [code]_on_screen_suspended()[/code] (called when another screen covers it). The control that had the focus when
## a screen was covered gets it back when the screen is on top again; a new screen keeps a focus it set itself, or
## gives the focus to its first control that takes it. A screen that leaves the tree by itself (it freed itself) is
## taken off the stack, and the screen under it becomes active.
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
	_forget_freed()
	return null if _screens.is_empty() else _screens.back()


## How many screens are stacked.
func count() -> int:
	_forget_freed()
	return _screens.size()


## The screens, from the bottom one to the top one.
func get_screens() -> Array[Control]:
	_forget_freed()
	return _screens.duplicate()


## True from the start of a faded push, pop or replace to its end; other changes are refused meanwhile.
func is_fading() -> bool:
	return _fading


## Puts [param screen] on top; the others pause until it is taken off. Refused while a fade is under way (returns
## false). With [param fade], await it.
func push(screen: Control, fade := true) -> bool:
	if _fading:
		return false
	var covered := top()
	if covered:
		_suspend(covered)  # Reads no input from now on, while the screen fades out too.
	if fade:
		_begin_fade()
		await _fade_to(1.0)
	_add(screen)
	if fade:
		await _fade_to(0.0)
		_end_fade()
	if is_instance_valid(screen) and screen == top():
		_activate(screen)
	return true


## Takes the top screen off; the one under it becomes active. The screen is freed and null is returned, unless
## [param keep]: then the screen is returned, out of the tree, for the caller to keep. Returns null when there is no
## screen or a fade is under way. With [param fade], await it.
func pop(fade := true, keep := false) -> Control:
	if _fading or top() == null:
		return null
	var screen: Control = _screens.back()
	_suspend(screen, false)  # Reads no input from now on.
	if fade:
		_begin_fade()
		await _fade_to(1.0)
	_take_off(screen)
	var below := top()
	if fade:
		await _fade_to(0.0)
		_end_fade()
	if below and is_instance_valid(below) and below == top():
		_activate(below)
	if keep and is_instance_valid(screen):
		_reset(screen)
		return screen
	if is_instance_valid(screen):
		screen.queue_free()
	return null


## Puts [param screen] in place of the top screen (which is freed), in one fade. Refused while a fade is under way
## (returns false). With [param fade], await it.
func replace(screen: Control, fade := true) -> bool:
	if _fading:
		return false
	var old := top()
	if old:
		_suspend(old, false)
	if fade:
		_begin_fade()
		await _fade_to(1.0)
	if old and is_instance_valid(old):
		_take_off(old)
		old.queue_free()
	_add(screen)
	if fade:
		await _fade_to(0.0)
		_end_fade()
	if is_instance_valid(screen) and screen == top():
		_activate(screen)
	return true


## Takes every screen off and frees them, without a fade. Refused while a fade is under way (returns false).
func clear() -> bool:
	if _fading:
		return false
	_forget_freed()
	while not _screens.is_empty():
		var screen: Control = _screens.back()
		_take_off(screen)
		screen.queue_free()
	return true


func _add(screen: Control) -> void:
	add_child(screen)
	_screens.push_back(screen)
	if not screen.tree_exited.is_connected(_on_screen_left):
		screen.tree_exited.connect(_on_screen_left.bind(screen))
	screen_pushed.emit(screen)


# Off the stack and out of the tree, by the stack itself.
func _take_off(screen: Control) -> void:
	_screens.erase(screen)
	if screen.tree_exited.is_connected(_on_screen_left):
		screen.tree_exited.disconnect(_on_screen_left)
	if screen.get_parent() == self:
		remove_child(screen)
	screen_popped.emit(screen)


# A stacked screen left the tree without the stack (it freed itself): it is taken off, and the screen under it
# becomes active if it was the top one.
func _on_screen_left(screen: Control) -> void:
	if not is_inside_tree():
		return  # The whole stack left the tree with its screens: they stay stacked.
	var index := _screens.find(screen)
	if index < 0:
		return
	var was_top := index == _screens.size() - 1
	_screens.remove_at(index)
	if screen.tree_exited.is_connected(_on_screen_left):
		screen.tree_exited.disconnect(_on_screen_left)
	screen_popped.emit(screen)
	var below := top()
	if was_top and not _fading and below and below.is_inside_tree():
		_activate(below)


# Screens freed while out of the tree's reach are no longer stacked.
func _forget_freed() -> void:
	_screens = _screens.filter(func(screen) -> bool: return is_instance_valid(screen))  # Untyped: it may be freed.


# Takes the screen's input away; [param covered]: another screen goes over it (its hook is called).
func _suspend(screen: Control, covered := true) -> void:
	if screen.has_meta(&"_jrpg_suspended"):
		return
	screen.set_meta(&"_jrpg_suspended", true)
	var owner_of_focus: Control = screen.get_viewport().gui_get_focus_owner() if screen.is_inside_tree() else null
	if owner_of_focus and (owner_of_focus == screen or screen.is_ancestor_of(owner_of_focus)):
		screen.set_meta(&"_jrpg_focus", screen.get_path_to(owner_of_focus))
		owner_of_focus.release_focus()
	# What the screen had, given back when it is on top again.
	screen.set_meta(&"_jrpg_process_mode", screen.process_mode)
	screen.set_meta(&"_jrpg_focus_behavior", screen.focus_behavior_recursive)
	screen.set_meta(&"_jrpg_mouse_behavior", screen.mouse_behavior_recursive)
	screen.process_mode = Node.PROCESS_MODE_DISABLED
	# A click would otherwise still focus a control of a paused screen (a click gives the focus whatever the
	# process mode).
	screen.focus_behavior_recursive = Control.FOCUS_BEHAVIOR_DISABLED
	screen.mouse_behavior_recursive = Control.MOUSE_BEHAVIOR_DISABLED
	if covered and screen.has_method(&"_on_screen_suspended"):
		screen.call(&"_on_screen_suspended")


# What a covered screen had, given back (it is on top again, or taken off and kept).
func _reset(screen: Control) -> void:
	if not screen.has_meta(&"_jrpg_suspended"):
		return
	screen.remove_meta(&"_jrpg_suspended")
	screen.process_mode = screen.get_meta(&"_jrpg_process_mode", Node.PROCESS_MODE_INHERIT)
	screen.focus_behavior_recursive = screen.get_meta(&"_jrpg_focus_behavior", Control.FOCUS_BEHAVIOR_INHERITED)
	screen.mouse_behavior_recursive = screen.get_meta(&"_jrpg_mouse_behavior", Control.MOUSE_BEHAVIOR_INHERITED)
	for key: StringName in [&"_jrpg_process_mode", &"_jrpg_focus_behavior", &"_jrpg_mouse_behavior"]:
		screen.remove_meta(key)


func _activate(screen: Control) -> void:
	_reset(screen)
	var target: Control = null
	if screen.has_meta(&"_jrpg_focus"):
		target = screen.get_node_or_null(screen.get_meta(&"_jrpg_focus")) as Control
		screen.remove_meta(&"_jrpg_focus")
	var current: Control = screen.get_viewport().gui_get_focus_owner()
	if target == null and current and (current == screen or screen.is_ancestor_of(current)):
		target = current  # The screen chose its focus itself.
	if target == null:
		target = screen if screen.focus_mode != Control.FOCUS_NONE else screen.find_next_valid_focus()
		if target and target != screen and not screen.is_ancestor_of(target):
			target = null  # The next focusable control is not this screen's.
	if target:
		target.grab_focus()
	if screen.has_method(&"_on_screen_activated"):
		screen.call(&"_on_screen_activated")


func _begin_fade() -> void:
	_fading = true
	_fade.mouse_filter = Control.MOUSE_FILTER_STOP  # Clicks stop at the fade while it runs.


func _end_fade() -> void:
	_fade.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_fading = false


func _fade_to(alpha: float) -> void:
	var tween := create_tween()
	tween.tween_property(_fade, "color:a", alpha, fade_seconds)
	await tween.finished
