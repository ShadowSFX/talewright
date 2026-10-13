extends Node2D
## Prints markers the bridge tests look for and moves a square with the ui_* actions.

var frames := 0
var accepts := 0


func _ready() -> void:
	print("PROBE_READY")
	push_error("PROBE_ERROR deliberate test error")


func _process(delta: float) -> void:
	frames += 1
	var dir := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	$Square.position += dir * 200.0 * delta
	$Info.text = "frame %d  pos %s  accepts %d" % [frames, $Square.position.round(), accepts]
	if frames % 120 == 0:
		print("PROBE_TICK %d" % frames)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_accept"):
		accepts += 1
		print("PROBE_ACCEPT %d" % accepts)
