extends CanvasLayer

signal restart_requested
signal debug_board_requested(board_name: String)
signal show_heat_labels_changed(enabled: bool)
signal move_preview_changed(enabled: bool)
signal chaos_mode_changed(enabled: bool)

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

@onready var turn_label: Label = %TurnLabel
@onready var balls_label: Label = %BallsLabel
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
@onready var preset_button: MenuButton = %PresetButton

var rules_overlay: Control
var rules_panel: PanelContainer
var rules_close_button: Button


func _ready() -> void:
	apply_panel_style()
	apply_button_style(restart_button)
	apply_button_style(rules_button)
	apply_menu_button_style(preset_button)
	apply_toggle_style(show_heat_toggle)
	apply_toggle_style(move_preview_toggle)
	apply_toggle_style(chaos_mode_toggle)

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
	score_label.text = "Score  " + str(snapshot.get("score", 0))
	cleared_label.text = "Cleared  " + str(snapshot.get("cleared", 0))
	chain_label.text = "Chain  " + str(snapshot.get("chain", 0)) + " / Best " + str(snapshot.get("max_chain", 0))
	next_label.text = "Next"
	update_next_preview(snapshot.get("next_heats", []))
	state_label.text = str(snapshot.get("state", "Ready"))

	if bool(snapshot.get("game_over", false)):
		state_label.add_theme_color_override("font_color", Color8(255, 106, 78))
	else:
		state_label.add_theme_color_override("font_color", Color8(255, 226, 130))


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


func apply_menu_button_style(menu_btn: MenuButton) -> void:
	apply_button_style(menu_btn)


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
