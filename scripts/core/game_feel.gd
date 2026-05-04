class_name GameFeel
extends RefCounted

## Version A — Classic approach.
## Hit-stop: briefly sets Engine.time_scale near zero, then restores.
## Screen-shake: procedural random offset applied to a target Node2D's position.

static func hitstop(duration: float = 0.06, time_scale: float = 0.02) -> void:
	var tree := Engine.get_main_loop() as SceneTree
	if tree == null:
		return
	var original := Engine.time_scale
	Engine.time_scale = time_scale
	await tree.create_timer(duration, true, false, true).timeout
	Engine.time_scale = original


static func screen_shake(
	target: Node2D,
	intensity: float = 6.0,
	duration: float = 0.25,
	frequency: float = 30.0
) -> void:
	if target == null or not is_instance_valid(target):
		return
	var original_pos: Vector2 = target.position
	var elapsed := 0.0
	var decay := 1.0
	while elapsed < duration:
		var dt: float = target.get_process_delta_time()
		if dt <= 0.0:
			dt = 0.016
		elapsed += dt
		decay = 1.0 - (elapsed / duration)
		var offset := Vector2(
			randf_range(-1.0, 1.0) * intensity * decay,
			randf_range(-1.0, 1.0) * intensity * decay
		)
		target.position = original_pos + offset
		await target.get_tree().process_frame
	target.position = original_pos


## Convenience: fire hitstop and shake together.
static func impact(
	target: Node2D,
	hitstop_duration: float = 0.06,
	shake_intensity: float = 6.0,
	shake_duration: float = 0.25
) -> void:
	screen_shake(target, shake_intensity, shake_duration)
	hitstop(hitstop_duration)
