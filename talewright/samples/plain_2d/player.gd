extends CharacterBody2D

const SPEED := 200.0


func _physics_process(_delta: float) -> void:
	var direction := Input.get_axis("ui_left", "ui_right")
	if "--smoke" in OS.get_cmdline_user_args():
		direction = 1.0
	velocity = Vector2(direction * SPEED, 0.0)
	move_and_slide()
