extends CanvasLayer

## Score feedback layer — pure visual, no gameplay logic.
## Receives score_event signals from the board and shows:
##   - survival: faint "+2" drifting up from the target cell
##   - clear: bouncing score number at elimination site, colored by heat
##   - multiplier: centered "x2" / "x4" / "x8" banner (chain >= 2 only)
##
## Pixel style: Press Start 2P font, bold outline, punchy colors bound to heat.

var PIXEL_FONT: FontFile

func _init() -> void:
	# Load TTF dynamically — works without .import file
	PIXEL_FONT = FontFile.new()
	PIXEL_FONT.load_dynamic_font("res://assets/fonts/PressStart2P-Regular.ttf")

# Font sizes — bigger and bolder for pixel feel
const SURVIVAL_FONT_SIZE := 32
const CLEAR_FONT_SIZE := 48
const MULTIPLIER_BASE_FONT_SIZE := 72
const MULTIPLIER_FONT_STEP := 14

# Per-heat fill colors — vivid, high contrast on dark board
const HEAT_FILL_COLORS := {
	1: Color8(180, 196, 210),   # cool steel
	2: Color8(230, 60, 40),     # hot red
	3: Color8(255, 130, 30),    # blazing orange
	4: Color8(255, 210, 50),    # molten gold
	5: Color8(255, 255, 240),   # white-hot
}
# Per-heat outline colors — deep, punchy contrast
const HEAT_OUTLINE_COLORS := {
	1: Color8(30, 36, 50),
	2: Color8(80, 10, 5),
	3: Color8(110, 40, 0),
	4: Color8(90, 60, 0),
	5: Color8(180, 50, 20),
}
# Survival text — subtle but readable
const SURVIVAL_FILL := Color8(220, 216, 200)
const SURVIVAL_OUTLINE := Color8(50, 46, 38)
# Multiplier text — explosive yellow-gold
const MULTIPLIER_FILL := Color8(255, 230, 60)
const MULTIPLIER_OUTLINE := Color8(100, 50, 0)

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


func _show_clear(data: Dictionary) -> void:
	var heat: int = data.get("heat", 1)
	var base_score: int = data.get("base_score", 0)
	var cells = data.get("cells", [])

	var fill_color: Color = HEAT_FILL_COLORS.get(heat, Color.WHITE)
	var outline_color: Color = HEAT_OUTLINE_COLORS.get(heat, Color.BLACK)
	var text := "+" + str(base_score)

	# Find center of eliminated cells
	var center: Vector2 = Vector2.ZERO
	for c in cells:
		var cell_vec: Vector2i = c
		center += _board.grid_to_pixel_center(cell_vec)
	if cells.size() > 0:
		center /= float(cells.size())
	else:
		center = _board.grid_to_pixel_center(Vector2i(0, 0))
	center += _board.global_position

	_spawn_bouncing_text(text, center, fill_color, outline_color, CLEAR_FONT_SIZE, 1.2)


func _show_multiplier(data: Dictionary) -> void:
	var chain_depth: int = data.get("chain_depth", 2)
	var multiplier: int = data.get("multiplier", 2)

	var text := "x" + str(multiplier)
	var font_size: int = MULTIPLIER_BASE_FONT_SIZE + mini(chain_depth - 2, 6) * MULTIPLIER_FONT_STEP

	# Center of viewport
	var viewport_size: Vector2 = get_viewport().get_visible_rect().size
	var center: Vector2 = viewport_size * 0.5

	_spawn_bouncing_text(text, center, MULTIPLIER_FILL, MULTIPLIER_OUTLINE, font_size, 1.5)


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
	duration: float
) -> void:
	var label := _make_pixel_label(text, fill_color, outline_color, font_size)
	var half_size: Vector2 = label.get_combined_minimum_size() * 0.5
	label.position = position - half_size
	label.scale = Vector2(0.15, 0.15)
	_feedback_layer.add_child(label)

	var tween := label.create_tween()
	# Slam in with overshoot — punchy
	tween.tween_property(label, "scale", Vector2.ONE * 1.25, 0.14).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tween.tween_property(label, "scale", Vector2.ONE * 1.0, 0.07).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN_OUT)
	# Hold then drift up and fade
	tween.tween_interval(duration * 0.4)
	tween.set_parallel(true)
	tween.tween_property(label, "modulate:a", 0.0, duration * 0.45).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
	tween.tween_property(label, "position:y", label.position.y - 30, duration * 0.45).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.chain().tween_callback(label.queue_free)
