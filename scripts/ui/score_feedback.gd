extends CanvasLayer

## Visual score pipeline. Gameplay score is owned by board.gd; this layer only
## stages score events, multiplier impact, particles, and scoreboard injection.

const PIXEL_FONT: FontFile = preload("res://assets/fonts/PressStart2P-Regular.ttf")


const SURVIVAL_FONT_SIZE := 32
const PER_BALL_FONT_SIZE := 26
const PER_BALL_FONT_STEP := 4
const PER_BALL_FONT_MAX_STEP := 16
const MULTIPLIER_BASE_FONT_SIZE := 72

const HEAT_FILL_COLORS := {
	1: Color8(180, 196, 210),
	2: Color8(230, 60, 40),
	3: Color8(255, 130, 30),
	4: Color8(255, 210, 50),
	5: Color8(255, 255, 240),
}
const HEAT_OUTLINE_COLORS := {
	1: Color8(30, 36, 50),
	2: Color8(80, 10, 5),
	3: Color8(110, 40, 0),
	4: Color8(90, 60, 0),
	5: Color8(180, 50, 20),
}

const SURVIVAL_FILL := Color8(220, 216, 200)
const SURVIVAL_OUTLINE := Color8(50, 46, 38)

const BALL_STAGGER_SECONDS := 0.06
const BASE_SCORE_HOLD_SECONDS := 0.8
const CHAIN_MULTIPLIER_START_SECONDS := 0.36
const FADE_OUT_SECONDS := 0.25
const REVEAL_PAUSE_SECONDS := 0.15
const POP_IN_SCALE_OVERSHOOT := 1.3
const POP_IN_DURATION := 0.20
const POP_IN_SETTLE_DURATION := 0.10
const MULTIPLIED_HOLD_SECONDS := 0.18

const PARTICLES_PER_SCORE := 5
const PARTICLE_SIZE := 5.0
const PARTICLE_FLIGHT_SECONDS := 0.48
const SCOREBOARD_INJECT_DELAY := 0.08

var _board: Node2D
var _hud: CanvasLayer
var _feedback_layer: Control
var _clear_queue: Array[Dictionary] = []
var _is_playing_clear := false


func setup(board: Node2D, hud: CanvasLayer) -> void:
	_board = board
	_hud = hud
	board.score_event.connect(_on_score_event)
	if board.has_method("register_score_feedback_sync"):
		board.register_score_feedback_sync()
	_feedback_layer = Control.new()
	_feedback_layer.name = "ScoreFeedbackLayer"
	_feedback_layer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_feedback_layer.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(_feedback_layer)


func _on_score_event(event_type: String, data: Dictionary) -> void:
	match event_type:
		"survival":
			_show_survival(data)
		"clear":
			_queue_clear(data)
		"multiplier":
			pass


func _show_survival(data: Dictionary) -> void:
	var cell: Vector2i = data.get("cell", Vector2i(-1, -1))
	var amount: int = data.get("amount", 0)
	var target_score: int = data.get("target_score", -1)
	if cell.x < 0:
		return

	var screen_pos: Vector2 = _board.grid_to_pixel_center(cell) + _board.global_position
	_spawn_floating_text(
		"+" + str(amount),
		screen_pos,
		SURVIVAL_FILL,
		SURVIVAL_OUTLINE,
		SURVIVAL_FONT_SIZE,
		1.0,
		-56.0
	)
	if target_score >= 0 and _hud != null and _hud.has_method("inject_score_to"):
		_hud.inject_score_to(target_score, 1, 1)


func _queue_clear(data: Dictionary) -> void:
	_clear_queue.append(data)
	if not _is_playing_clear:
		_drain_clear_queue()


func _drain_clear_queue() -> void:
	_is_playing_clear = true
	while not _clear_queue.is_empty():
		var data: Dictionary = _clear_queue.pop_front()
		await _play_clear_pipeline(data)
	_is_playing_clear = false


func _play_clear_pipeline(data: Dictionary) -> void:
	var event_id: int = data.get("event_id", -1)
	var chain_depth: int = data.get("chain_depth", 1)
	var multiplier: int = data.get("multiplier", 1)
	var heat: int = data.get("heat", 1)
	var balls: Array = data.get("balls", [])
	var target_score: int = data.get("target_score", -1)
	if balls.is_empty():
		_notify_clear_pipeline_done(event_id)
		return

	var base_labels := _spawn_score_labels(balls, 1, PER_BALL_FONT_SIZE, 0.0)

	if chain_depth >= 2:
		await get_tree().create_timer(CHAIN_MULTIPLIER_START_SECONDS).timeout
		_spawn_multiplier_banner(multiplier, heat)
		await get_tree().create_timer(_multiplier_reveal_seconds(multiplier)).timeout
		await _fade_labels(base_labels)
		await get_tree().create_timer(REVEAL_PAUSE_SECONDS).timeout
		var multiplied_font := _multiplied_font_size(chain_depth)
		var multiplied_labels := _spawn_score_labels(balls, multiplier, multiplied_font, 0.0)
		await get_tree().create_timer(_label_pop_span(multiplied_labels.size()) + MULTIPLIED_HOLD_SECONDS).timeout
		await _atomize_labels_to_scoreboard(multiplied_labels, heat)
	else:
		await get_tree().create_timer(_label_pop_span(base_labels.size()) + BASE_SCORE_HOLD_SECONDS).timeout
		await _atomize_labels_to_scoreboard(base_labels, heat)

	if target_score >= 0 and _hud != null and _hud.has_method("inject_score_to"):
		_hud.inject_score_to(target_score, heat, _multiplier_intensity(multiplier))
	_notify_clear_pipeline_done(event_id)


func _notify_clear_pipeline_done(event_id: int) -> void:
	if event_id >= 0 and _board != null and _board.has_method("notify_score_feedback_complete"):
		_board.notify_score_feedback_complete(event_id)


func _spawn_score_labels(
	balls: Array,
	multiplier: int,
	font_size: int,
	delay_offset: float
) -> Array[Label]:
	var labels: Array[Label] = []
	for index in range(balls.size()):
		var ball_data: Dictionary = balls[index]
		var cell: Vector2i = ball_data.get("cell", Vector2i(-1, -1))
		var heat: int = ball_data.get("heat", 1)
		var base_score: int = ball_data.get("base_score", 0)
		if cell.x < 0:
			continue

		var score := base_score * multiplier
		var screen_pos: Vector2 = _board.grid_to_pixel_center(cell) + _board.global_position
		var fill: Color = HEAT_FILL_COLORS.get(heat, Color.WHITE)
		var outline: Color = HEAT_OUTLINE_COLORS.get(heat, Color.BLACK)
		var label := _spawn_per_ball_score(
			"+" + str(score),
			screen_pos,
			fill,
			outline,
			font_size,
			delay_offset + index * BALL_STAGGER_SECONDS
		)
		labels.append(label)
	return labels


func _spawn_multiplier_banner(multiplier: int, heat: int) -> void:
	var viewport_size: Vector2 = get_viewport().get_visible_rect().size
	var center: Vector2 = viewport_size * 0.5
	var fill: Color = HEAT_FILL_COLORS.get(heat, Color.WHITE)
	var outline: Color = HEAT_OUTLINE_COLORS.get(heat, Color.BLACK)
	var font_size := _multiplier_font_size(multiplier)
	var intensity := _multiplier_intensity(multiplier)
	var hold_seconds := _multiplier_hold_seconds(multiplier)
	_spawn_bouncing_text("x" + str(multiplier), center, fill, outline, font_size, hold_seconds, intensity)


func _multiplier_font_size(multiplier: int) -> int:
	if multiplier <= 2:
		return MULTIPLIER_BASE_FONT_SIZE
	if multiplier <= 4:
		return MULTIPLIER_BASE_FONT_SIZE + 18
	if multiplier <= 8:
		return MULTIPLIER_BASE_FONT_SIZE + 34
	return MULTIPLIER_BASE_FONT_SIZE + 48


func _multiplier_intensity(multiplier: int) -> int:
	if multiplier <= 2:
		return 1
	if multiplier <= 4:
		return 2
	if multiplier <= 8:
		return 3
	return 4


func _multiplier_hold_seconds(multiplier: int) -> float:
	if multiplier <= 2:
		return 0.8
	if multiplier <= 4:
		return 0.95
	if multiplier <= 8:
		return 1.1
	return 1.2


func _multiplier_reveal_seconds(multiplier: int) -> float:
	return 0.22 + 0.08 * float(_multiplier_intensity(multiplier))


func _multiplied_font_size(chain_depth: int) -> int:
	var font_step: int = mini(chain_depth - 1, 4) * PER_BALL_FONT_STEP
	return PER_BALL_FONT_SIZE + min(font_step, PER_BALL_FONT_MAX_STEP)


func _label_pop_span(label_count: int) -> float:
	return max(0.0, float(label_count - 1) * BALL_STAGGER_SECONDS + POP_IN_DURATION + POP_IN_SETTLE_DURATION)


func _fade_labels(labels: Array[Label]) -> void:
	for label in labels:
		if not is_instance_valid(label):
			continue
		var tween := label.create_tween()
		tween.tween_property(label, "modulate:a", 0.0, FADE_OUT_SECONDS).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		tween.tween_callback(label.queue_free)
	await get_tree().create_timer(FADE_OUT_SECONDS).timeout


func _atomize_labels_to_scoreboard(labels: Array[Label], heat: int) -> void:
	var target := _scoreboard_target_position()
	var fill: Color = HEAT_FILL_COLORS.get(heat, Color.WHITE)
	for label in labels:
		if not is_instance_valid(label):
			continue
		_spawn_label_particles(label, target, fill)
		label.queue_free()
	await get_tree().create_timer(PARTICLE_FLIGHT_SECONDS + SCOREBOARD_INJECT_DELAY).timeout


func _scoreboard_target_position() -> Vector2:
	if _hud != null and _hud.has_method("get_score_target_position"):
		return _hud.get_score_target_position()
	var viewport_size: Vector2 = get_viewport().get_visible_rect().size
	return Vector2(viewport_size.x - 180.0, 90.0)


func _spawn_label_particles(label: Label, target: Vector2, fill_color: Color) -> void:
	var rect := label.get_global_rect()
	var center := rect.position + rect.size * 0.5
	for index in range(PARTICLES_PER_SCORE):
		var particle := ColorRect.new()
		particle.color = fill_color
		particle.custom_minimum_size = Vector2(PARTICLE_SIZE, PARTICLE_SIZE)
		particle.size = Vector2(PARTICLE_SIZE, PARTICLE_SIZE)
		particle.position = center + Vector2(
			float(index - 2) * 7.0,
			float((index % 2) * 2 - 1) * 5.0
		)
		particle.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_feedback_layer.add_child(particle)

		var arc := Vector2(
			lerpf(particle.position.x, target.x, 0.45),
			min(particle.position.y, target.y) - 56.0 - float(index % 3) * 12.0
		)
		var tween := particle.create_tween()
		tween.tween_property(particle, "position", arc, PARTICLE_FLIGHT_SECONDS * 0.45).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
		tween.tween_property(particle, "position", target, PARTICLE_FLIGHT_SECONDS * 0.55).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		tween.parallel().tween_property(particle, "modulate:a", 0.0, PARTICLE_FLIGHT_SECONDS * 0.35).set_delay(PARTICLE_FLIGHT_SECONDS * 0.65)
		tween.tween_callback(particle.queue_free)


func _make_pixel_label(
	text: String,
	fill_color: Color,
	outline_color: Color,
	font_size: int
) -> Label:
	var label := Label.new()
	label.text = text
	label.add_theme_font_override("font", PIXEL_FONT)
	label.add_theme_font_size_override("font_size", font_size)
	label.add_theme_color_override("font_color", fill_color)
	label.add_theme_color_override("font_outline_color", outline_color)
	label.add_theme_constant_override("outline_size", maxi(3, font_size / 8))
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return label


func _spawn_floating_text(
	text: String,
	position: Vector2,
	fill_color: Color,
	outline_color: Color,
	font_size: int,
	duration: float,
	rise_pixels: float
) -> void:
	var label := _make_pixel_label(text, fill_color, outline_color, font_size)
	label.position = position - Vector2(label.get_combined_minimum_size().x * 0.5, font_size * 0.5)
	label.modulate.a = 0.8
	_feedback_layer.add_child(label)

	var tween := label.create_tween()
	tween.set_parallel(true)
	tween.tween_property(label, "position:y", position.y + rise_pixels, duration).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.tween_property(label, "modulate:a", 0.0, duration * 0.5).set_delay(duration * 0.5).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
	tween.chain().tween_callback(label.queue_free)


func _spawn_bouncing_text(
	text: String,
	position: Vector2,
	fill_color: Color,
	outline_color: Color,
	font_size: int,
	duration: float,
	intensity: int
) -> void:
	var label := _make_pixel_label(text, fill_color, outline_color, font_size)
	var half_size: Vector2 = label.get_combined_minimum_size() * 0.5
	label.position = position - half_size
	label.scale = Vector2(0.12, 0.12)
	_feedback_layer.add_child(label)

	var overshoot := 1.14 + 0.08 * float(clampi(intensity, 1, 4))
	var drift := 24.0 + 7.0 * float(clampi(intensity, 1, 4))
	var tween := label.create_tween()
	tween.tween_property(label, "scale", Vector2.ONE * overshoot, 0.16).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tween.tween_property(label, "scale", Vector2.ONE, 0.10).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN_OUT)
	tween.tween_interval(duration * 0.45)
	tween.set_parallel(true)
	tween.tween_property(label, "modulate:a", 0.0, duration * 0.45).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
	tween.tween_property(label, "position:y", label.position.y - drift, duration * 0.45).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.chain().tween_callback(label.queue_free)


func _spawn_per_ball_score(
	text: String,
	position: Vector2,
	fill_color: Color,
	outline_color: Color,
	font_size: int,
	stagger_delay: float
) -> Label:
	var label := _make_pixel_label(text, fill_color, outline_color, font_size)
	var half_size: Vector2 = label.get_combined_minimum_size() * 0.5
	label.position = position - half_size
	label.scale = Vector2(0.2, 0.2)
	label.modulate.a = 0.0
	_feedback_layer.add_child(label)

	var tween := label.create_tween()
	tween.tween_interval(stagger_delay)
	tween.set_parallel(true)
	tween.tween_property(label, "scale", Vector2.ONE * 1.15, POP_IN_DURATION).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tween.tween_property(label, "modulate:a", 1.0, POP_IN_DURATION * 0.4).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.chain().tween_property(label, "scale", Vector2.ONE, POP_IN_SETTLE_DURATION).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN_OUT)

	return label
