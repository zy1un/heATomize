extends CanvasLayer

## Score feedback layer — pure visual, no gameplay logic.
## Animation pipeline:
##   Normal (chain 1): per-ball base scores → particles → scoreboard
##   Chain  (chain ≥2): per-ball base scores → multiplier banner →
##                        base scores fade out → multiplied scores pop in →
##                        particles → scoreboard
##
## Pixel style: Press Start 2P font, bold outline, punchy colors bound to heat.

var PIXEL_FONT: FontFile

func _init() -> void:
	# Load TTF dynamically — works without .import file
	PIXEL_FONT = FontFile.new()
	PIXEL_FONT.load_dynamic_font("res://assets/fonts/PressStart2P-Regular.ttf")

# ── Font sizes ──────────────────────────────────────────────
const SURVIVAL_FONT_SIZE := 32
const PER_BALL_FONT_SIZE := 26
const PER_BALL_FONT_STEP := 4       # per chain depth above 1
const PER_BALL_FONT_MAX_STEP := 16  # cap at chain 5
const MULTIPLIER_BASE_FONT_SIZE := 72
const MULTIPLIER_FONT_STEP := 14

# ── Per-heat colors ────────────────────────────────────────
const HEAT_FILL_COLORS := {
	1: Color8(180, 196, 210),   # cool steel
	2: Color8(230, 60, 40),     # hot red
	3: Color8(255, 130, 30),    # blazing orange
	4: Color8(255, 210, 50),    # molten gold
	5: Color8(255, 255, 240),   # white-hot
}
const HEAT_OUTLINE_COLORS := {
	1: Color8(30, 36, 50),
	2: Color8(80, 10, 5),
	3: Color8(110, 40, 0),
	4: Color8(90, 60, 0),
	5: Color8(180, 50, 20),
}

# ── Survival text ──────────────────────────────────────────
const SURVIVAL_FILL := Color8(220, 216, 200)
const SURVIVAL_OUTLINE := Color8(50, 46, 38)

# ── Multiplier text ────────────────────────────────────────
const MULTIPLIER_FILL := Color8(255, 230, 60)
const MULTIPLIER_OUTLINE := Color8(100, 50, 0)

# ── Timing ─────────────────────────────────────────────────
const BALL_STAGGER_SECONDS := 0.06    # delay between each ball's popup
const BASE_SCORE_HOLD_SECONDS := 0.8  # how long base scores stay before fade/particle
const FADE_OUT_SECONDS := 0.25        # base score fade-out duration
const REVEAL_PAUSE_SECONDS := 0.15    # pause between base fade-out and multiplied pop-in ("揭晓"感)
const POP_IN_SCALE_OVERSHOOT := 1.3
const POP_IN_DURATION := 0.20
const POP_IN_SETTLE_DURATION := 0.10
const MULTIPLIED_HOLD_SECONDS := 0.9  # how long multiplied scores stay before particle
const MULTIPLIER_BANNER_DURATION := 2.0

var _board: Node2D
var _feedback_layer: Control


func setup(board: Node2D) -> void:
	_board = board
	board.score_event.connect(_on_score_event)
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
			_show_clear(data)
		"multiplier":
			_show_multiplier(data)


# ═══════════════════════════════════════════════════════════
#  Survival — faint "+2" drifting up
# ═══════════════════════════════════════════════════════════

func _show_survival(data: Dictionary) -> void:
	var cell: Vector2i = data.get("cell", Vector2i(-1, -1))
	var amount: int = data.get("amount", 0)
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


# ═══════════════════════════════════════════════════════════
#  Clear — per-ball score pipeline
# ═══════════════════════════════════════════════════════════

func _show_clear(data: Dictionary) -> void:
	var chain_depth: int = data.get("chain_depth", 1)
	var multiplier: int = data.get("multiplier", 1)
	var balls: Array = data.get("balls", [])

	if balls.is_empty():
		return

	# Spawn per-ball base score labels with stagger
	var labels: Array[Label] = []
	for index in range(balls.size()):
		var ball_data: Dictionary = balls[index]
		var cell: Vector2i = ball_data.get("cell", Vector2i(-1, -1))
		var heat: int = ball_data.get("heat", 1)
		var base_score: int = ball_data.get("base_score", 0)
		if cell.x < 0:
			continue

		var screen_pos: Vector2 = _board.grid_to_pixel_center(cell) + _board.global_position
		var fill: Color = HEAT_FILL_COLORS.get(heat, Color.WHITE)
		var outline: Color = HEAT_OUTLINE_COLORS.get(heat, Color.BLACK)
		var label := _spawn_per_ball_score("+" + str(base_score), screen_pos, fill, outline, PER_BALL_FONT_SIZE, index * BALL_STAGGER_SECONDS)
		labels.append(label)

	if chain_depth >= 2:
		# Chain path: hold base scores → multiplier banner will show separately →
		# then _apply_multiplier_transition is called when multiplier event arrives
		# Store labels for later transition
		_pending_multiplier_labels.append({
			"labels": labels,
			"balls": balls,
			"multiplier": multiplier,
			"chain_depth": chain_depth,
		})
	else:
		# Normal path: hold briefly, then fade and drift up
		for label in labels:
			if not is_instance_valid(label):
				continue
			var tween: Tween = label.create_tween()
			tween.tween_interval(BASE_SCORE_HOLD_SECONDS)
			_fade_and_drift(tween, label, 0.6)


# ═══════════════════════════════════════════════════════════
#  Multiplier — chain ≥2 banner + triggers base→multiplied transition
# ═══════════════════════════════════════════════════════════

var _pending_multiplier_labels: Array[Dictionary] = []

func _show_multiplier(data: Dictionary) -> void:
	var chain_depth: int = data.get("chain_depth", 2)
	var multiplier: int = data.get("multiplier", 2)

	# Show the banner
	var text := "x" + str(multiplier)
	var font_size: int = MULTIPLIER_BASE_FONT_SIZE + mini(chain_depth - 2, 6) * MULTIPLIER_FONT_STEP
	var viewport_size: Vector2 = get_viewport().get_visible_rect().size
	var center: Vector2 = viewport_size * 0.5
	_spawn_bouncing_text(text, center, MULTIPLIER_FILL, MULTIPLIER_OUTLINE, font_size, MULTIPLIER_BANNER_DURATION)

	# Trigger base → multiplied transition for the most recent clear event
	if _pending_multiplier_labels.is_empty():
		return

	var pending: Dictionary = _pending_multiplier_labels.pop_front()
	var labels: Array = pending.get("labels", [])
	var balls: Array = pending.get("balls", [])
	var mult: int = pending.get("multiplier", multiplier)

	# Fade out base scores
	for label in labels:
		if not is_instance_valid(label):
			continue
		var tween: Tween = label.create_tween()
		tween.tween_property(label, "modulate:a", 0.0, FADE_OUT_SECONDS).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		tween.tween_callback(label.queue_free)

	# Spawn multiplied scores with stagger (after fade-out + reveal pause)
	var fade_delay := FADE_OUT_SECONDS + REVEAL_PAUSE_SECONDS
	for index in range(balls.size()):
		var ball_data: Dictionary = balls[index]
		var cell: Vector2i = ball_data.get("cell", Vector2i(-1, -1))
		var heat: int = ball_data.get("heat", 1)
		var base_score: int = ball_data.get("base_score", 0)
		if cell.x < 0:
			continue

		var multiplied_score: int = base_score * mult
		var screen_pos: Vector2 = _board.grid_to_pixel_center(cell) + _board.global_position
		var fill: Color = HEAT_FILL_COLORS.get(heat, Color.WHITE)
		var outline: Color = HEAT_OUTLINE_COLORS.get(heat, Color.BLACK)
		var font_step: int = mini(chain_depth - 1, 4) * PER_BALL_FONT_STEP
		var sized_font: int = PER_BALL_FONT_SIZE + min(font_step, PER_BALL_FONT_MAX_STEP)
		var stagger: float = fade_delay + index * BALL_STAGGER_SECONDS

		var label := _spawn_per_ball_score(
			"+" + str(multiplied_score), screen_pos, fill, outline, sized_font, stagger
		)

		# Add pop-in overshoot animation
		var pop_tween: Tween = label.create_tween()
		pop_tween.tween_property(label, "scale", Vector2.ONE * POP_IN_SCALE_OVERSHOOT, POP_IN_DURATION).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
		pop_tween.tween_property(label, "scale", Vector2.ONE, POP_IN_SETTLE_DURATION).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN_OUT)
		# Hold then fade and drift
		pop_tween.tween_interval(MULTIPLIED_HOLD_SECONDS)
		_fade_and_drift(pop_tween, label, 0.7)


# ═══════════════════════════════════════════════════════════
#  Label factories
# ═══════════════════════════════════════════════════════════

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

	var tween: Tween = label.create_tween()
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
	duration: float
) -> void:
	var label := _make_pixel_label(text, fill_color, outline_color, font_size)
	var half_size: Vector2 = label.get_combined_minimum_size() * 0.5
	label.position = position - half_size
	label.scale = Vector2(0.15, 0.15)
	_feedback_layer.add_child(label)

	var tween: Tween = label.create_tween()
	# Slam in with overshoot — punchy
	tween.tween_property(label, "scale", Vector2.ONE * 1.25, 0.18).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tween.tween_property(label, "scale", Vector2.ONE * 1.0, 0.09).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN_OUT)
	# Hold then drift up and fade
	tween.tween_interval(duration * 0.45)
	tween.set_parallel(true)
	tween.tween_property(label, "modulate:a", 0.0, duration * 0.45).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
	tween.tween_property(label, "position:y", label.position.y - 30, duration * 0.45).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
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

	# Staggered pop-in
	var tween: Tween = label.create_tween()
	tween.tween_interval(stagger_delay)
	tween.set_parallel(true)
	tween.tween_property(label, "scale", Vector2.ONE * 1.15, POP_IN_DURATION).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tween.tween_property(label, "modulate:a", 1.0, POP_IN_DURATION * 0.4).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.chain().tween_property(label, "scale", Vector2.ONE, POP_IN_SETTLE_DURATION).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN_OUT)

	return label


# ── Shared fade-and-drift helper (appended to existing tween) ──

func _fade_and_drift(tween: Tween, label: Label, duration: float) -> void:
	tween.set_parallel(true)
	tween.tween_property(label, "modulate:a", 0.0, duration * 0.6).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
	tween.tween_property(label, "position:y", label.position.y - 24, duration * 0.6).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.chain().tween_callback(label.queue_free)
