extends Node2D
## Smoke check: with `-- --smoke`, the player walks right for 120 physics frames and must be stopped by the wall.

var _frames := 0


func _physics_process(_delta: float) -> void:
	if not "--smoke" in OS.get_cmdline_user_args():
		return
	_frames += 1
	if _frames == 120:
		var player: CharacterBody2D = $Player
		var ok := player.position.x > 150.0 and player.position.x < 270.0
		print("PLAIN2D_OK" if ok else "PLAIN2D_FAIL x=%s" % player.position.x)
		get_tree().quit(0 if ok else 1)
