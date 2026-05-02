extends CanvasLayer

signal restart_requested
signal debug_board_requested(board_name: String)
signal show_heat_labels_changed(enabled: bool)
signal move_preview_changed(enabled: bool)
signal chaos_mode_changed(enabled: bool)

const PIXEL_FONT: FontFile = preload("res://assets/fonts/PressStart2P-Regular.ttf")

const PREVIEW_COLORS := {
	1: Color8(201, 205, 209),
	2: Color8(141, 35, 28),
	3: Color8(240, 90, 36),
	4: Color8(236, 158, 20),
	5: Color8(255, 255, 250),
}
const PREVIEW_OUTLINES := {
	1: Color8(61, 67, 74),
	2: Color8(217, 221, 226),
	3: Color8(166, 30, 22),
	4: Color8(94, 46, 0),
	5: Color8(255, 76, 32),
}
const SCOREBOARD_BASE_COLOR := Color8(13, 12, 11, 248)
const SCOREBOARD_BORDER_COLOR := Color8(255, 143, 34, 190)
const SCOREBOARD_TEXT_COLOR := Color8(255, 174, 54)
const SCOREBOARD_GLOW_COLOR := Color8(255, 96, 24, 145)
const SCOREBOARD_OUTLINE_COLOR := Color8(74, 24, 2)
const SCOREBOARD_HEAT_COLORS := {
	1: Color8(180, 196, 210),
	2: Color8(230, 60, 40),
	3: Color8(255, 130, 30),
	4: Color8(255, 210, 50),
	5: Color8(255, 255, 240),
}
const SCOREBOARD_HEAT_OUTLINES := {
	1: Color8(30, 36, 50),
	2: Color8(80, 10, 5),
	3: Color8(110, 40, 0),
	4: Color8(90, 60, 0),
	5: Color8(180, 50, 20),
}

@onready var turn_label: Label = %TurnLabel
@onready var balls_label: Label = %BallsLabel
@onready var score_panel: PanelContainer = %ScorePanel
@onready var score_caption: Label = %ScoreCaption
@onready var score_readout: Control = %ScoreReadout
@onready var score_glow_label: Label = %ScoreGlowLabel
@onready var score_label: Label = %ScoreLabel
@onready var cleared_label: Label = %ClearedLabel
@onready var chain_label: Label = %ChainLabel
@onready var next_label: Label = %NextLabel
@onready var next_preview_bar: HBoxContainer = %NextPreviewBar
@onready var state_label: Label = %StateLabel
@onready var panel: PanelContainer = %Panel
@onready var restart_button: Button = %RestartButton
@onready var rules_button: Button = %RulesButton
@onready var show_heat_toggle: CheckButton = %ShowHeatToggle
@onready var move_preview_toggle: CheckButton = %MovePreviewToggle
@onready var chaos_mode_toggle: CheckButton = %ChaosModeToggle
@onready var preset_panel: PanelContainer = %PresetPanel
@onready var preset_button: MenuButton = %PresetButton

var rules_overlay: Control
var rules_panel: PanelContainer
var rules_close_button: Button
var displayed_score: int = 0
var actual_score: int = 0
var score_tween: Tween
var score_flash_tween: Tween
var score_scanline_nodes: Array[ColorRect] = []


func _ready() -> void:
	apply_panel_style()
	apply_button_style(restart_button)
	apply_button_style(rules_button)
	apply_menu_button_style(preset_button)
	apply_toggle_style(show_heat_toggle)
	apply_toggle_style(move_preview_toggle)
	apply_toggle_style(chaos_mode_toggle)
	apply_score_label_style()
	apply_preset_panel_style()

	restart_button.pressed.connect(func() -> void: restart_requested.emit())
	rules_button.pressed.connect(show_rules_overlay)
	show_heat_toggle.toggled.connect(func(enabled: bool) -> void: show_heat_labels_changed.emit(enabled))
	move_preview_toggle.toggled.connect(func(enabled: bool) -> void: move_preview_changed.emit(enabled))
	chaos_mode_toggle.toggled.connect(func(enabled: bool) -> void: chaos_mode_changed.emit(enabled))

	# Preset submenu
	var popup: PopupMenu = preset_button.get_popup()
	popup.add_item("Cascade (F5)", 0)
	popup.add_item("Blocked (F6)", 1)
	popup.add_item("Chain (F7)", 2)
	apply_popup_style(popup)
	popup.id_pressed.connect(_on_preset_selected)

	build_rules_overlay()


func update_status(snapshot: Dictionary) -> void:
	turn_label.text = "Turn  " + str(snapshot.get("turn", 0))
	balls_label.text = "Balls  " + str(snapshot.get("balls", 0))
	var incoming_score := int(snapshot.get("score", 0))
	actual_score = incoming_score
	if incoming_score <= displayed_score or incoming_score == 0:
		set_displayed_score(incoming_score)
	cleared_label.text = "Cleared  " + str(snapshot.get("cleared", 0))
	chain_label.text = "Chain  " + str(snapshot.get("chain", 0)) + " / Best " + str(snapshot.get("max_chain", 0))
	next_label.text = "Next"
	update_next_preview(snapshot.get("next_heats", []))
	state_label.text = str(snapshot.get("state", "Ready"))

	if bool(snapshot.get("game_over", false)):
		state_label.add_theme_color_override("font_color", Color8(255, 106, 78))
	else:
		state_label.add_theme_color_override("font_color", Color8(255, 226, 130))


func set_displayed_score(value: int) -> void:
	displayed_score = value
	var text := str(displayed_score).pad_zeros(6)
	score_label.text = text
	score_glow_label.text = text


func get_score_target_position() -> Vector2:
	var rect := score_panel.get_global_rect()
	return rect.position + rect.size * 0.5


func inject_score_to(target_score: int, heat: int, intensity: int = 1) -> void:
	actual_score = max(actual_score, target_score)
	var target: int = maxi(displayed_score, target_score)
	animate_score_to(target, heat, intensity)


func animate_score_to(target_score: int, heat: int, intensity: int = 1) -> void:
	if score_tween != null and score_tween.is_valid():
		score_tween.kill()

	var start_score: int = displayed_score
	var score_delta: int = maxi(1, target_score - start_score)
	var duration: float = clampf(0.18 + float(score_delta) / 260.0, 0.22, 0.8)
	var driver: Tween = create_tween()
	score_tween = driver
	score_panel.pivot_offset = score_panel.size * 0.5
	driver.tween_method(
		func(value: float) -> void:
			set_displayed_score(int(round(value))),
		float(start_score),
		float(target_score),
		duration
	).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	driver.parallel().tween_property(
		score_panel,
		"scale",
		Vector2.ONE * (1.0 + 0.025 * float(clampi(intensity, 1, 4))),
		0.09
	).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	driver.tween_property(score_panel, "scale", Vector2.ONE, 0.16).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN_OUT)
	flash_scoreboard(heat, intensity)


func flash_scoreboard(heat: int, intensity: int = 1) -> void:
	if score_flash_tween != null and score_flash_tween.is_valid():
		score_flash_tween.kill()

	var flash_color: Color = SCOREBOARD_HEAT_COLORS.get(heat, SCOREBOARD_TEXT_COLOR)
	var outline_color: Color = SCOREBOARD_HEAT_OUTLINES.get(heat, SCOREBOARD_OUTLINE_COLOR)
	var style := make_score_panel_style(
		SCOREBOARD_BASE_COLOR.lerp(flash_color, 0.16),
		flash_color,
		2 + clampi(intensity, 1, 4)
	)
	score_panel.add_theme_stylebox_override("panel", style)
	score_label.add_theme_color_override("font_color", flash_color)
	score_label.add_theme_color_override("font_outline_color", outline_color)
	score_label.add_theme_constant_override("outline_size", 2 + clampi(intensity, 1, 4))
	score_glow_label.add_theme_color_override("font_color", flash_color)
	score_glow_label.add_theme_color_override("font_outline_color", flash_color)
	score_glow_label.modulate.a = 0.72

	score_flash_tween = create_tween()
	score_flash_tween.tween_interval(0.18 + 0.05 * float(clampi(intensity, 1, 4)))
	score_flash_tween.tween_callback(func() -> void:
		apply_score_panel_style()
	)


func update_next_preview(next_heats: Variant) -> void:
	for child in next_preview_bar.get_children():
		child.queue_free()

	for heat_value in next_heats:
		var heat := int(heat_value)
		next_preview_bar.add_child(create_preview_token(heat))


func create_preview_token(heat: int) -> Control:
	var token := PanelContainer.new()
	token.custom_minimum_size = Vector2(52, 52)
	var style := StyleBoxFlat.new()
	style.bg_color = PREVIEW_COLORS.get(heat, Color.WHITE)
	style.border_color = PREVIEW_OUTLINES.get(heat, Color.BLACK)
	style.set_border_width_all(3)
	style.corner_radius_top_left = 26
	style.corner_radius_top_right = 26
	style.corner_radius_bottom_left = 26
	style.corner_radius_bottom_right = 26
	token.add_theme_stylebox_override("panel", style)

	var label := Label.new()
	label.text = str(heat)
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	label.add_theme_font_size_override("font_size", 22)
	if heat >= 4:
		label.add_theme_color_override("font_color", Color8(42, 30, 18))
	else:
		label.add_theme_color_override("font_color", Color8(248, 242, 232))
	token.add_child(label)
	return token


func apply_panel_style() -> void:
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color8(34, 31, 29, 232)
	panel_style.border_color = Color8(255, 210, 92, 150)
	panel_style.set_border_width_all(2)
	panel_style.corner_radius_top_left = 8
	panel_style.corner_radius_top_right = 8
	panel_style.corner_radius_bottom_left = 8
	panel_style.corner_radius_bottom_right = 8
	panel.add_theme_stylebox_override("panel", panel_style)


func make_score_panel_style(bg_color: Color, border_color: Color, border_width: int) -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = bg_color
	style.border_color = border_color
	style.set_border_width_all(border_width)
	style.corner_radius_top_left = 6
	style.corner_radius_top_right = 6
	style.corner_radius_bottom_left = 6
	style.corner_radius_bottom_right = 6
	style.shadow_color = Color(border_color.r, border_color.g, border_color.b, 0.30)
	style.shadow_size = 12 + border_width * 2
	style.shadow_offset = Vector2.ZERO
	style.content_margin_left = 0
	style.content_margin_right = 0
	return style


func apply_score_panel_style() -> void:
	score_panel.add_theme_stylebox_override(
		"panel",
		make_score_panel_style(SCOREBOARD_BASE_COLOR, SCOREBOARD_BORDER_COLOR, 2)
	)
	score_label.add_theme_color_override("font_color", SCOREBOARD_TEXT_COLOR)
	score_label.add_theme_color_override("font_outline_color", SCOREBOARD_OUTLINE_COLOR)
	score_label.add_theme_constant_override("outline_size", 2)
	score_glow_label.add_theme_color_override("font_color", SCOREBOARD_GLOW_COLOR)
	score_glow_label.add_theme_color_override("font_outline_color", SCOREBOARD_GLOW_COLOR)
	score_glow_label.add_theme_constant_override("outline_size", 8)
	score_glow_label.modulate.a = 0.46


func apply_button_style(button: Button) -> void:
	var normal_style := StyleBoxFlat.new()
	normal_style.bg_color = Color8(72, 62, 48)
	normal_style.border_color = Color8(176, 128, 58)
	normal_style.set_border_width_all(1)
	normal_style.corner_radius_top_left = 6
	normal_style.corner_radius_top_right = 6
	normal_style.corner_radius_bottom_left = 6
	normal_style.corner_radius_bottom_right = 6

	var hover_style := normal_style.duplicate() as StyleBoxFlat
	hover_style.bg_color = Color8(96, 78, 52)

	button.add_theme_stylebox_override("normal", normal_style)
	button.add_theme_stylebox_override("hover", hover_style)
	button.add_theme_color_override("font_color", Color8(245, 240, 235))


func apply_toggle_style(toggle: CheckButton) -> void:
	toggle.add_theme_color_override("font_color", Color8(245, 240, 235))
	toggle.add_theme_font_size_override("font_size", 15)


func apply_score_label_style() -> void:
	score_panel.custom_minimum_size = Vector2(0, 86)
	score_readout.custom_minimum_size = Vector2(260, 42)
	score_caption.add_theme_font_override("font", PIXEL_FONT)
	score_caption.add_theme_font_size_override("font_size", 10)
	score_caption.add_theme_color_override("font_color", Color8(255, 154, 42))
	score_label.add_theme_font_override("font", PIXEL_FONT)
	score_glow_label.add_theme_font_override("font", PIXEL_FONT)
	score_label.add_theme_font_size_override("font_size", 26)
	score_glow_label.add_theme_font_size_override("font_size", 26)
	score_label.custom_minimum_size = Vector2(260, 42)
	score_glow_label.custom_minimum_size = Vector2(260, 42)
	score_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	score_glow_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	score_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	score_glow_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	apply_score_panel_style()
	score_panel.scale = Vector2.ONE
	score_label.scale = Vector2.ONE
	score_glow_label.scale = Vector2.ONE
	build_score_scanlines()
	set_displayed_score(displayed_score)


func build_score_scanlines() -> void:
	for line in score_scanline_nodes:
		if is_instance_valid(line):
			line.queue_free()
	score_scanline_nodes.clear()

	var readout_height := 42
	for index in range(0, readout_height, 6):
		var scanline := ColorRect.new()
		scanline.name = "ScoreScanline"
		scanline.mouse_filter = Control.MOUSE_FILTER_IGNORE
		scanline.color = Color8(255, 118, 28, 34)
		scanline.anchor_left = 0.0
		scanline.anchor_right = 1.0
		scanline.anchor_top = 0.0
		scanline.anchor_bottom = 0.0
		scanline.offset_left = 0.0
		scanline.offset_right = 0.0
		scanline.offset_top = float(index)
		scanline.offset_bottom = float(index + 1)
		score_readout.add_child(scanline)
		score_scanline_nodes.append(scanline)


func apply_menu_button_style(menu_btn: MenuButton) -> void:
	apply_button_style(menu_btn)


func apply_preset_panel_style() -> void:
	var style := StyleBoxFlat.new()
	style.bg_color = Color8(28, 25, 22, 210)
	style.border_color = Color8(176, 128, 58, 145)
	style.set_border_width_all(1)
	style.corner_radius_top_left = 6
	style.corner_radius_top_right = 6
	style.corner_radius_bottom_left = 6
	style.corner_radius_bottom_right = 6
	preset_panel.add_theme_stylebox_override("panel", style)


func apply_popup_style(popup: PopupMenu) -> void:
	popup.add_theme_color_override("font_color", Color8(245, 240, 235))
	popup.add_theme_color_override("font_hover_color", Color8(255, 226, 130))
	popup.add_theme_font_size_override("font_size", 16)
	popup.add_theme_constant_override("v_separation", 8)
	popup.add_theme_constant_override("h_separation", 16)

	var bg_style := StyleBoxFlat.new()
	bg_style.bg_color = Color8(34, 31, 29, 246)
	bg_style.border_color = Color8(176, 128, 58)
	bg_style.set_border_width_all(1)
	bg_style.corner_radius_top_left = 6
	bg_style.corner_radius_top_right = 6
	bg_style.corner_radius_bottom_left = 6
	bg_style.corner_radius_bottom_right = 6
	popup.add_theme_stylebox_override("panel", bg_style)

	var hover_style := StyleBoxFlat.new()
	hover_style.bg_color = Color8(72, 62, 48)
	hover_style.border_color = Color8(176, 128, 58, 120)
	hover_style.set_border_width_all(1)
	hover_style.corner_radius_top_left = 4
	hover_style.corner_radius_top_right = 4
	hover_style.corner_radius_bottom_left = 4
	hover_style.corner_radius_bottom_right = 4
	popup.add_theme_stylebox_override("hover", hover_style)


func _on_preset_selected(id: int) -> void:
	match id:
		0: debug_board_requested.emit("cascade")
		1: debug_board_requested.emit("blocked")
		2: debug_board_requested.emit("chain")


func build_rules_overlay() -> void:
	rules_overlay = Control.new()
	rules_overlay.name = "RulesOverlay"
	rules_overlay.visible = false
	rules_overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(rules_overlay)

	var dim := ColorRect.new()
	dim.color = Color8(11, 10, 9, 190)
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	rules_overlay.add_child(dim)

	rules_panel = PanelContainer.new()
	rules_panel.name = "RulesPanel"
	rules_panel.anchor_left = 0.5
	rules_panel.anchor_top = 0.5
	rules_panel.anchor_right = 0.5
	rules_panel.anchor_bottom = 0.5
	rules_panel.custom_minimum_size = Vector2(720, 520)
	rules_panel.offset_left = -360
	rules_panel.offset_top = -260
	rules_panel.offset_right = 360
	rules_panel.offset_bottom = 260
	rules_overlay.add_child(rules_panel)
	apply_rules_panel_style()

	var margin := MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 24)
	margin.add_theme_constant_override("margin_top", 22)
	margin.add_theme_constant_override("margin_right", 24)
	margin.add_theme_constant_override("margin_bottom", 22)
	rules_panel.add_child(margin)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 14)
	margin.add_child(vbox)

	var header_row := HBoxContainer.new()
	header_row.add_theme_constant_override("separation", 12)
	vbox.add_child(header_row)

	var title := Label.new()
	title.text = "Rules"
	title.add_theme_font_size_override("font_size", 30)
	title.add_theme_color_override("font_color", Color8(255, 210, 92))
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header_row.add_child(title)

	rules_close_button = Button.new()
	rules_close_button.text = "Close"
	apply_button_style(rules_close_button)
	rules_close_button.pressed.connect(hide_rules_overlay)
	header_row.add_child(rules_close_button)

	var body := RichTextLabel.new()
	body.bbcode_enabled = true
	body.fit_content = false
	body.scroll_active = true
	body.size_flags_vertical = Control.SIZE_EXPAND_FILL
	body.add_theme_font_size_override("normal_font_size", 18)
	body.add_theme_color_override("default_color", Color8(245, 240, 235))
	body.text = get_rules_text()
	vbox.add_child(body)


func apply_rules_panel_style() -> void:
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color8(30, 27, 25, 246)
	panel_style.border_color = Color8(255, 210, 92, 190)
	panel_style.set_border_width_all(2)
	panel_style.corner_radius_top_left = 8
	panel_style.corner_radius_top_right = 8
	panel_style.corner_radius_bottom_left = 8
	panel_style.corner_radius_bottom_right = 8
	rules_panel.add_theme_stylebox_override("panel", panel_style)


func show_rules_overlay() -> void:
	rules_overlay.visible = true


func hide_rules_overlay() -> void:
	rules_overlay.visible = false


func _unhandled_input(event: InputEvent) -> void:
	if rules_overlay == null or not rules_overlay.visible:
		return
	if event is InputEventKey and event.pressed and not event.echo and event.keycode == KEY_ESCAPE:
		hide_rules_overlay()
		get_viewport().set_input_as_handled()


func get_rules_text() -> String:
	var lines := PackedStringArray([
		"[b]Goal[/b]",
		"Move heat balls into same-heat clusters, clear space, and survive as the board fills.",
		"",
		"[b]Turn[/b]",
		"1. Select one ball and move it through empty cells.",
		"2. The moved board resolves heat, clears clusters, then applies aftershock.",
		"3. Up to 3 previewed balls spawn after the system settles.",
		"4. Newly spawned balls start participating on the next turn.",
		"",
		"[b]Heat[/b]",
		"- A hotter orthogonal neighbor raises a ball by +1.",
		"- If every neighbor is colder, or there are no neighbors, it cools by -1.",
		"- Otherwise it stays stable. Heat is always 1 to 5.",
		"",
		"[b]Clear Thresholds[/b]",
		"Heat 1 clears at 7 connected balls.",
		"Heat 2 clears at 6 connected balls.",
		"Heat 3 clears at 5 connected balls.",
		"Heat 4 clears at 4 connected balls.",
		"Heat 5 clears at 3 connected balls.",
		"",
		"[b]Right-click Preview[/b]",
		"When a ball is selected, hover an empty reachable cell and right-click to preview only the immediate heat update after that move.",
		"",
		"[b]Chaos Mode[/b]",
		"Chaos mode uses the older order: spawn first, then let new balls immediately participate in the same system resolve.",
	])
	return "\n".join(lines)
