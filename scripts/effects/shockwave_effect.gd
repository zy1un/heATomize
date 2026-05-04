class_name ShockwaveEffect
extends Node2D

enum Mode {
	ELIMINATION,
	AFTERSHOCK,
}

class Shard:
	var angle := 0.0
	var distance := 0.0
	var start_offset := Vector2.ZERO
	var size := Vector2.ONE
	var pixel_size := 4.0
	var blocks: Array[Vector2i] = []
	var spin := 0.0
	var color := Color.WHITE
	var drag := 0.0
	var delay := 0.0
	var is_dust := false


var radius: float = 27.0
var fill_color: Color = Color.WHITE
var outline_color: Color = Color.BLACK
var heat := 1
var effect_scale: float = 1.0
var mode: Mode = Mode.ELIMINATION
var progress: float = 0.0:
	set(value):
		progress = clampf(value, 0.0, 1.0)
		queue_redraw()

var _shards: Array[Shard] = []


static func spawn(
	parent: Node,
	origin: Vector2,
	ball_radius: float,
	ball_fill_color: Color,
	ball_outline_color: Color,
	visual_scale: float,
	wave_mode: Mode = Mode.ELIMINATION,
	source_heat: int = 1
) -> ShockwaveEffect:
	if parent == null or visual_scale <= 0.0:
		return null

	var effect := ShockwaveEffect.new()
	effect.position = origin
	effect.radius = ball_radius
	effect.fill_color = ball_fill_color
	effect.outline_color = ball_outline_color
	effect.heat = clampi(source_heat, 1, 5)
	effect.effect_scale = clampf(visual_scale, 0.0, 1.0)
	effect.mode = wave_mode
	parent.add_child(effect)
	effect.play()
	return effect


func _ready() -> void:
	z_index = 12
	_build_shards()


func play() -> void:
	var duration := 1.45
	if mode == Mode.AFTERSHOCK:
		duration = 0.72

	var tween := create_tween()
	tween.tween_property(self, "progress", 1.0, duration).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.finished.connect(queue_free)


func _draw() -> void:
	if effect_scale <= 0.0:
		return

	var palette := _get_heat_palette()
	if mode == Mode.AFTERSHOCK:
		_draw_aftershock_shatter(palette)
	else:
		_draw_elimination_shatter(palette)


func _draw_elimination_shatter(palette: Dictionary) -> void:
	var flash_alpha := pow(1.0 - progress, 5.0) * effect_scale
	if flash_alpha > 0.01:
		var flash_size := radius * lerpf(0.88, 0.22, minf(progress * 5.0, 1.0))
		draw_rect(
			Rect2(Vector2(-flash_size, -flash_size), Vector2.ONE * flash_size * 2.0),
			_with_alpha(palette["core"], 0.34 * flash_alpha),
			true
		)

	for shard in _shards:
		var local_t := clampf((progress - shard.delay) / (1.0 - shard.delay), 0.0, 1.0)
		if local_t <= 0.0:
			continue

		var burst_t := clampf(local_t / 0.42, 0.0, 1.0)
		var residue_t := clampf((local_t - 0.42) / 0.58, 0.0, 1.0)
		var burst := _ease_out_cubic(burst_t)
		var fade := _get_residue_fade(local_t) * effect_scale
		if fade <= 0.01:
			continue

		var direction := Vector2(cos(shard.angle), sin(shard.angle))
		var tangent := Vector2(-direction.y, direction.x)
		var drag_offset := tangent * sin(burst_t * PI) * shard.drag
		var settle_drift := (direction * radius * 0.28 + tangent * shard.drag * 0.22) * _ease_out_cubic(residue_t)
		var shard_pos := shard.start_offset + direction * shard.distance * burst + drag_offset + settle_drift
		var shard_scale := lerpf(1.0, 0.5 if not shard.is_dust else 0.24, local_t)
		var shard_pixel_size := shard.pixel_size * shard_scale
		var rotation := shard.spin * local_t

		_draw_pixel_shard(shard, shard_pos, shard_pixel_size, rotation, _with_alpha(shard.color, fade))
		if local_t < 0.5 and not shard.is_dust:
			var trail_alpha := fade * (1.0 - local_t / 0.5) * 0.26
			var trail_pos := shard_pos - direction * shard.pixel_size * shard_scale * 2.2 * lerpf(0.7, 1.5, burst_t)
			_draw_pixel_shard(shard, trail_pos, shard_pixel_size * 0.72, rotation, _with_alpha(palette["smoke"], trail_alpha))


func _draw_aftershock_shatter(palette: Dictionary) -> void:
	for shard in _shards:
		var local_t := clampf((progress - shard.delay) / (1.0 - shard.delay), 0.0, 1.0)
		if local_t <= 0.0:
			continue

		var burst_t := clampf(local_t / 0.55, 0.0, 1.0)
		var residue_t := clampf((local_t - 0.55) / 0.45, 0.0, 1.0)
		var burst := _ease_out_cubic(burst_t)
		var fade := lerpf(1.0, 0.0, _ease_in_cubic(residue_t)) * effect_scale
		if fade <= 0.01:
			continue

		var direction := Vector2(cos(shard.angle), sin(shard.angle))
		var shard_pos := shard.start_offset + direction * shard.distance * 0.48 * burst + direction * radius * 0.12 * _ease_out_cubic(residue_t)
		var shard_pixel_size := shard.pixel_size * lerpf(0.68, 0.28, local_t)
		_draw_pixel_shard(shard, shard_pos, shard_pixel_size, shard.spin * local_t, _with_alpha(shard.color.lerp(palette["core"], 0.2), fade * 0.72))


func _draw_pixel_shard(shard: Shard, center: Vector2, pixel_size: float, rotation: float, color: Color) -> void:
	if shard.blocks.is_empty():
		draw_rect(Rect2(center - Vector2.ONE * pixel_size * 0.5, Vector2.ONE * pixel_size), color, true)
		return

	var block_center_offset := _get_block_center_offset(shard.blocks)
	for block in shard.blocks:
		var local_cell := Vector2(block.x, block.y) - block_center_offset
		var block_center := center + (local_cell * pixel_size).rotated(rotation)
		var block_size := Vector2.ONE * maxf(1.0, floorf(pixel_size))
		draw_rect(Rect2(block_center - block_size * 0.5, block_size), color, true)


func _build_shards() -> void:
	if not _shards.is_empty():
		return

	var palette := _get_heat_palette()
	var count := 26
	var max_distance := radius * 3.45
	if mode == Mode.AFTERSHOCK:
		count = 10
		max_distance = radius * 1.7

	var fracture_angles := _build_fracture_angles()
	for index in range(count):
		var shard := Shard.new()
		var fracture_angle: float = fracture_angles[index % fracture_angles.size()]
		shard.angle = fracture_angle + randf_range(-0.22, 0.22)
		shard.is_dust = index >= int(float(count) * 0.66)
		if shard.is_dust:
			shard.distance = randf_range(radius * 0.75, max_distance * 0.72)
		else:
			shard.distance = randf_range(radius * 1.1, max_distance)
		shard.start_offset = Vector2(randf_range(-radius * 0.24, radius * 0.24), randf_range(-radius * 0.24, radius * 0.24))
		if shard.is_dust:
			shard.pixel_size = randf_range(3.0, 4.5)
			shard.blocks = [Vector2i.ZERO]
		else:
			shard.pixel_size = randf_range(4.5, 6.5)
			shard.blocks = _pick_block_shape()
		shard.spin = randf_range(-1.6, 1.6) if not shard.is_dust else randf_range(-2.4, 2.4)
		shard.drag = randf_range(-radius * 0.2, radius * 0.2)
		shard.delay = randf_range(0.0, 0.14) if not shard.is_dust else randf_range(0.06, 0.22)
		shard.color = _pick_shard_color(palette)
		_shards.append(shard)


func _pick_shard_color(palette: Dictionary) -> Color:
	var roll := randf()
	if roll < 0.08:
		return palette["core"].lerp(palette["mid"], 0.35)
	if roll < 0.52:
		return palette["mid"].lerp(palette["smoke"], randf_range(0.08, 0.28))
	if roll < 0.86:
		return palette["edge"].lerp(palette["smoke"], randf_range(0.12, 0.36))
	return fill_color.lerp(palette["smoke"], randf_range(0.35, 0.68))


func _build_fracture_angles() -> Array[float]:
	var base := randf_range(0.0, TAU)
	return [
		base,
		base + PI * 0.5 + randf_range(-0.18, 0.18),
		base + PI + randf_range(-0.2, 0.2),
		base + PI * 1.5 + randf_range(-0.18, 0.18),
	]


func _pick_block_shape() -> Array[Vector2i]:
	var shapes: Array[Array] = [
		[Vector2i.ZERO, Vector2i(1, 0), Vector2i(-1, 0), Vector2i(0, 1)],
		[Vector2i.ZERO, Vector2i(1, 0), Vector2i(-1, 0), Vector2i(0, -1)],
		[Vector2i.ZERO, Vector2i(1, 0), Vector2i(0, 1), Vector2i(1, 1)],
		[Vector2i.ZERO, Vector2i(1, 0), Vector2i(-1, 0), Vector2i(2, 0)],
		[Vector2i.ZERO, Vector2i(0, 1), Vector2i(0, -1), Vector2i(0, 2)],
		[Vector2i.ZERO, Vector2i(1, 0), Vector2i(0, 1), Vector2i(-1, 1)],
		[Vector2i.ZERO, Vector2i(-1, 0), Vector2i(0, 1), Vector2i(1, 1)],
		[Vector2i.ZERO, Vector2i(1, 0), Vector2i(0, 1)],
		[Vector2i.ZERO, Vector2i(-1, 0), Vector2i(0, -1)],
	]
	var source: Array = shapes.pick_random()
	var shape: Array[Vector2i] = []
	for block in source:
		shape.append(block)
	return shape


func _get_block_center_offset(blocks: Array[Vector2i]) -> Vector2:
	var sum := Vector2.ZERO
	for block in blocks:
		sum += Vector2(block.x, block.y)
	return sum / float(blocks.size())


func _get_heat_palette() -> Dictionary:
	match heat:
		1:
			return {
				"core": Color8(252, 254, 255),
				"mid": Color8(188, 199, 210),
				"edge": Color8(72, 84, 98),
				"smoke": Color8(52, 58, 65),
			}
		2:
			return {
				"core": Color8(255, 190, 154),
				"mid": Color8(212, 48, 36),
				"edge": Color8(82, 16, 20),
				"smoke": Color8(40, 18, 18),
			}
		3:
			return {
				"core": Color8(255, 232, 148),
				"mid": Color8(250, 92, 24),
				"edge": Color8(137, 29, 16),
				"smoke": Color8(54, 25, 18),
			}
		4:
			return {
				"core": Color8(255, 236, 164),
				"mid": Color8(222, 137, 28),
				"edge": Color8(126, 58, 8),
				"smoke": Color8(58, 42, 24),
			}
		_:
			return {
				"core": Color8(255, 248, 218),
				"mid": Color8(236, 176, 82),
				"edge": Color8(184, 48, 28),
				"smoke": Color8(74, 48, 32),
			}


func _with_alpha(color: Color, alpha: float) -> Color:
	return Color(color.r, color.g, color.b, clampf(alpha, 0.0, 1.0))


func _get_residue_fade(local_t: float) -> float:
	if local_t < 0.38:
		return 1.0
	if local_t < 0.68:
		return lerpf(1.0, 0.72, (local_t - 0.38) / 0.3)
	return lerpf(0.72, 0.0, _ease_in_cubic((local_t - 0.68) / 0.32))


func _ease_out_cubic(value: float) -> float:
	return 1.0 - pow(1.0 - clampf(value, 0.0, 1.0), 3.0)


func _ease_in_cubic(value: float) -> float:
	var clamped := clampf(value, 0.0, 1.0)
	return clamped * clamped * clamped


func _ease_out_quart(value: float) -> float:
	return 1.0 - pow(1.0 - clampf(value, 0.0, 1.0), 4.0)
