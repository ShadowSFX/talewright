extends Node3D
## Smoke check: with `-- --smoke`, the box falls for 120 physics frames and must come to rest on the floor.

var _frames := 0


func _physics_process(_delta: float) -> void:
	if not "--smoke" in OS.get_cmdline_user_args():
		return
	_frames += 1
	if _frames == 120:
		var box: RigidBody3D = $Box
		var y := box.global_position.y
		var ok := y < 4.0 and y > 0.2
		print("PLAIN3D_OK" if ok else "PLAIN3D_FAIL y=%s" % y)
		get_tree().quit(0 if ok else 1)
