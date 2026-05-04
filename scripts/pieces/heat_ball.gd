extends Node2D

@export var radius: float = 18.0
@export var pulse_scale: float = 1.08
@export var pulse_speed: float = 3.2
@export var feedback_scale: float = 1.22
@export var heat_change_seconds: float = 0.56
@export var aftershock_seconds: float = 0.64
@export var elimination_seconds: float = 0.72

const PIXEL_FONT: FontFile = preload("res://assets/fonts/PressStart2P-Regular.ttf")
const GameFeel = preload("res://scripts/core/game_feel.gd")

@onready var visual: ColorRect = $Visual
@onready var heat_label: Label = %HeatLabel
var shader_material: ShaderMaterial
var feedback_tween: Tween

var base_fill_color: Color = Color.WHITE
var base_outline_color: Color = Color.BLACK
var is_selected := false
var is_feedback_active := false
var has_configured := false
var pulse_time := 0.0


func _ready() -> void:
	heat_label.add_theme_font_override("font", PIXEL_FONT)
	heat_label.add_theme_font_size_override("font_size", 20)
	ensure_unique_material()
	update_visual_scale()
	set_process(true)

func _process(delta: float) -> void:
	if is_feedback_active:
		return

	if not is_selected:
		if scale != Vector2.ONE:
			scale = Vector2.ONE
		return

	pulse_time += delta * pulse_speed
	var pulse: float = 0.5 + 0.5 * sin(pulse_time)
	var current_scale: float = lerpf(1.0, pulse_scale, pulse)
	scale = Vector2.ONE * current_scale


func configure(heat: int, fill_color: Color, outline_color: Color, apply_immediately := true) -> void:
	ensure_unique_material()
	base_fill_color = fill_color
	base_outline_color = outline_color
	if apply_immediately or not has_configured:
		apply_colors(fill_color, outline_color)
	update_heat_label(heat)
	update_visual_scale()
	name = "HeatBall_%d" % heat
	has_configured = true

func set_selected(selected: bool) -> void:
	is_selected = selected
	if is_feedback_active:
		return

	if selected:
		pulse_time = 0.0
		apply_colors(base_outline_color, base_fill_color)
	else:
		scale = Vector2.ONE
		apply_colors(base_fill_color, base_outline_color)

func set_heat_label_visible(visible: bool) -> void:
	heat_label.visible = visible

func play_spawn_feedback() -> void:
	stop_feedback()
	is_feedback_active = true
	modulate.a = 0.0
	scale = Vector2.ONE * 0.28

	feedback_tween = create_tween()
	feedback_tween.set_parallel(true)
	feedback_tween.tween_property(self, "scale", Vector2.ONE, 0.26).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	feedback_tween.tween_property(self, "modulate:a", 1.0, 0.18)
	feedback_tween.finished.connect(_on_feedback_finished)

func play_heat_feedback() -> void:
	play_color_flash(Color(1.0, 0.88, 0.34, 1.0), heat_change_seconds, feedback_scale)

func play_aftershock_feedback() -> void:
	play_color_flash(Color(1.0, 0.38, 0.18, 1.0), aftershock_seconds, feedback_scale * 1.08)

func play_elimination_feedback() -> void:
	stop_feedback()
	is_feedback_active = true
	apply_colors(Color.WHITE, Color(1.0, 0.25, 0.1, 1.0))
	spawn_elimination_shockwave()
	spawn_pixel_fragments()

	var effect_scale := clampf(GameFeel.get_flash_scale(), 0.0, 2.0)
	feedback_tween = create_tween()
	feedback_tween.set_parallel(true)
	feedback_tween.tween_property(self, "scale", Vector2.ONE * (1.0 + 0.25 * effect_scale), elimination_seconds * 0.28).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	feedback_tween.tween_property(heat_label, "modulate:a", 0.0, elimination_seconds * 0.22)
	feedback_tween.chain().tween_property(self, "scale", Vector2.ONE * 0.15, elimination_seconds * 0.5).set_trans(Tween.TRANS_EXPO).set_ease(Tween.EASE_IN)
	feedback_tween.parallel().tween_property(self, "modulate:a", 0.0, elimination_seconds * 0.5)

func play_color_flash(flash_color: Color, duration: float, target_scale: float) -> void:
	stop_feedback()
	var effect_scale := clampf(GameFeel.get_flash_scale(), 0.0, 2.0)
	if effect_scale <= 0.0:
		apply_colors(base_fill_color, base_outline_color)
		scale = Vector2.ONE
		set_selected(is_selected)
		return

	is_feedback_active = true
	var selected_scale := Vector2.ONE * (1.0 + (target_scale - 1.0) * effect_scale)
	scale = selected_scale
	apply_colors(base_fill_color.lerp(flash_color, clampf(effect_scale, 0.0, 1.0)), base_outline_color)

	feedback_tween = create_tween()
	feedback_tween.set_parallel(true)
	feedback_tween.tween_property(self, "scale", Vector2.ONE, duration).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	feedback_tween.tween_property(shader_material, "shader_parameter/fill_color", base_fill_color, duration)
	feedback_tween.tween_property(shader_material, "shader_parameter/outline_color", base_outline_color, duration)
	feedback_tween.finished.connect(_on_feedback_finished)

func stop_feedback() -> void:
	if feedback_tween != null and feedback_tween.is_valid():
		feedback_tween.kill()
	feedback_tween = null
	is_feedback_active = false
	modulate.a = 1.0
	heat_label.modulate.a = 1.0

func apply_colors(fill_color: Color, outline_color: Color) -> void:
	if shader_material == null:
		return
	shader_material.set_shader_parameter("fill_color", fill_color)
	shader_material.set_shader_parameter("outline_color", outline_color)

func update_visual_scale() -> void:
	var diameter := radius * 2.0
	visual.position = Vector2(-radius, -radius)
	visual.size = Vector2(diameter, diameter)
	heat_label.position = Vector2(-radius, -radius * 0.62)
	heat_label.size = Vector2(diameter, radius * 1.24)

func ensure_unique_material() -> void:
	if shader_material != null:
		return

	var source_material := visual.material as ShaderMaterial
	if source_material == null:
		return

	shader_material = source_material.duplicate() as ShaderMaterial
	visual.material = shader_material

func _on_feedback_finished() -> void:
	is_feedback_active = false
	modulate.a = 1.0
	heat_label.modulate.a = 1.0
	scale = Vector2.ONE
	set_selected(is_selected)


func update_heat_label(heat: int) -> void:
	heat_label.text = str(heat)
	if heat >= 4:
		heat_label.add_theme_color_override("font_color", Color8(42, 30, 18))
		heat_label.add_theme_color_override("font_outline_color", Color8(255, 248, 220, 180))
	else:
		heat_label.add_theme_color_override("font_color", Color8(248, 242, 232))
		heat_label.add_theme_color_override("font_outline_color", Color8(36, 32, 28, 210))


func spawn_pixel_fragments() -> void:
	var parent_node := get_parent()
	if parent_node == null:
		return

	var effect_scale := clampf(GameFeel.get_flash_scale(), 0.0, 1.0)
	var fragment_count := int(round(22.0 * effect_scale))
	if fragment_count <= 0:
		return

	for index in range(fragment_count):
		var fragment := ColorRect.new()
		var fragment_size := randf_range(4.0, 7.0)
		fragment.size = Vector2(fragment_size, fragment_size)
		fragment.pivot_offset = fragment.size * 0.5
		fragment.color = base_fill_color.lerp(base_outline_color, randf_range(0.0, 0.65))
		fragment.position = position + Vector2(randf_range(-radius * 0.42, radius * 0.42), randf_range(-radius * 0.42, radius * 0.42)) - fragment.pivot_offset
		fragment.rotation = randf_range(-0.25, 0.25)
		parent_node.add_child(fragment)

		var angle := (TAU * float(index) / float(fragment_count)) + randf_range(-0.28, 0.28)
		var distance := randf_range(radius * 0.75, radius * 1.75)
		var target_position: Vector2 = fragment.position + Vector2(cos(angle), sin(angle)) * distance
		var fragment_duration := elimination_seconds * randf_range(0.72, 1.08)

		var tween := create_tween()
		tween.set_parallel(true)
		tween.tween_property(fragment, "position", target_position, fragment_duration).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
		tween.tween_property(fragment, "rotation", fragment.rotation + randf_range(-1.4, 1.4), fragment_duration)
		tween.tween_property(fragment, "modulate:a", 0.0, fragment_duration).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		tween.finished.connect(fragment.queue_free)

func spawn_elimination_shockwave() -> void:
	var parent_node := get_parent()
	if parent_node == null:
		return

	var effect_scale := clampf(GameFeel.get_flash_scale(), 0.0, 1.0)
	if effect_scale <= 0.0:
		return

	var ring_count := 2
	for ring_index in range(ring_count):
		var ring := ColorRect.new()
		var ring_size := radius * (2.0 + float(ring_index) * 0.22)
		ring.size = Vector2(ring_size, ring_size)
		ring.pivot_offset = ring.size * 0.5
		ring.position = position - ring.pivot_offset
		ring.color = Color(1.0, 0.48 - float(ring_index) * 0.12, 0.12, 0.0)
		parent_node.add_child(ring)

		var ring_style := StyleBoxFlat.new()
		ring_style.bg_color = Color.TRANSPARENT
		ring_style.border_color = base_fill_color.lerp(Color.WHITE, 0.45)
		ring_style.set_border_width_all(3 - ring_index)
		var corner_radius := int(ring_size * 0.5)
		ring_style.corner_radius_top_left = corner_radius
		ring_style.corner_radius_top_right = corner_radius
		ring_style.corner_radius_bottom_left = corner_radius
		ring_style.corner_radius_bottom_right = corner_radius

		var panel := Panel.new()
		panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
		panel.size = ring.size
		panel.add_theme_stylebox_override("panel", ring_style)
		ring.add_child(panel)
		ring.modulate.a = (0.82 - float(ring_index) * 0.22) * effect_scale
		ring.scale = Vector2.ONE * (0.38 + float(ring_index) * 0.12)

		var delay := float(ring_index) * 0.045
		var duration := elimination_seconds * (0.42 + float(ring_index) * 0.08)
		var tween := create_tween()
		tween.tween_interval(delay)
		tween.set_parallel(true)
		tween.tween_property(ring, "scale", Vector2.ONE * (1.65 + float(ring_index) * 0.28), duration).set_trans(Tween.TRANS_EXPO).set_ease(Tween.EASE_OUT)
		tween.tween_property(ring, "modulate:a", 0.0, duration).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		tween.chain().tween_callback(ring.queue_free)
