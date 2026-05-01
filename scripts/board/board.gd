extends Node2D

signal status_changed(snapshot: Dictionary)
signal score_event(event_type: String, data: Dictionary)

const BoardPathfinding = preload("res://scripts/board/board_pathfinding.gd")
const BoardRules = preload("res://scripts/board/board_rules.gd")
const BoardTurns = preload("res://scripts/board/board_turns.gd")

const GRID_SIZE := 9
const GRID_SIDE := 66
const GRID_BORDER := 8
const GRID_LENGTH := GRID_SIDE + GRID_BORDER * 2
const WINDOW_BORDER := 30
const BALL_RADIUS := 27.0
const NEW_BALLS_PER_TURN := 3
const NEW_BALL_MIN_HEAT := 1
const NEW_BALL_MAX_HEAT := 3
const SURVIVAL_MOVE_SCORE := 2
const CLEAR_BASE_SCORE := 10
const CLEAR_HEAT_SCORE := 2
const MAX_SYSTEM_CYCLES := 64
const MOVE_ANIMATION_SECONDS := 0.16
const MOVE_ANIMATION_MIN_STEP_SECONDS := 0.045
const MOVE_ANIMATION_MAX_TOTAL_SECONDS := 0.48
const ELIMINATION_FEEDBACK_SECONDS := 0.72
const SYSTEM_PHASE_PAUSE_SECONDS := 0.18
const SPAWN_STAGGER_SECONDS := 0.09
const HEAT_TRANSFER_SECONDS := 0.48
const AFTERSHOCK_TRANSFER_SECONDS := 0.54
const TRANSFER_STAGGER_SECONDS := 0.035

const BOARD_FILL_COLOR := Color8(48, 44, 40)
const CELL_FILL_COLOR := Color8(245, 240, 235)
const CELL_BORDER_COLOR := Color8(58, 58, 58, 128)
const HOVER_FILL_COLOR := Color8(58, 58, 58, 128)
const HOVER_BORDER_COLOR := Color8(160, 130, 90)
const SELECTED_FILL_COLOR := Color8(255, 210, 58, 38)
const SELECTED_BORDER_COLOR := Color8(255, 210, 58, 120)
const PATH_FILL_COLOR := Color8(245, 188, 76, 72)
const PATH_BORDER_COLOR := Color8(245, 188, 76, 156)
const BLOCKED_FILL_COLOR := Color8(190, 48, 38, 72)
const BLOCKED_BORDER_COLOR := Color8(255, 86, 66, 180)
const MOVE_PREVIEW_BORDER_COLOR := Color8(120, 214, 255, 210)
const MOVE_PREVIEW_GLOW_COLOR := Color8(120, 214, 255, 72)

const HEAT_FILL_COLORS := {
	1: Color8(201, 205, 209),
	2: Color8(141, 35, 28),
	3: Color8(240, 90, 36),
	4: Color8(236, 158, 20),
	5: Color8(255, 255, 250),
}

const HEAT_OUTLINE_COLORS := {
	1: Color8(61, 67, 74),
	2: Color8(217, 221, 226),
	3: Color8(166, 30, 22),
	4: Color8(94, 46, 0),
	5: Color8(255, 76, 32),
}

const HEAT_BALL_SCENE := preload("res://scenes/pieces/HeatBall.tscn")
const ORTHOGONAL_DIRECTIONS: Array[Vector2i] = [
	Vector2i.LEFT,
	Vector2i.RIGHT,
	Vector2i.UP,
	Vector2i.DOWN,
]

var board_state: Array = []
var ball_nodes: Dictionary = {}
var balls_layer: Node2D
var effects_layer: Node2D

var hovered_cell := Vector2i(-1, -1)
var selected_cell := Vector2i(-1, -1)
var preview_path: Array[Vector2i] = []
var turn_index: int = 0
var total_eliminated: int = 0
var score: int = 0
var current_chain: int = 0
var max_chain: int = 0
var next_spawn_heats: Array[int] = []
var game_over := false
var is_busy := false
var show_heat_labels := true
var move_preview_enabled := true
var chaos_mode_enabled := false
var move_preview_target := Vector2i(-1, -1)
var move_preview_nodes: Array[Node] = []
var _preview_hidden_cell := Vector2i(-1, -1)
var _last_move_target := Vector2i(-1, -1)
var pathfinding: BoardPathfinding
var rules: BoardRules
var turns: BoardTurns

func _ready() -> void:
	print("Board is ready")
	pathfinding = BoardPathfinding.new()
	rules = BoardRules.new()
	turns = BoardTurns.new()
	setup_balls_layer()
	setup_effects_layer()
	initialize_board()
	place_test_balls()
	prepare_next_spawn_preview()
	emit_status("Ready")
	queue_redraw()

func _process(_delta: float) -> void:
	var new_hover := pixel_to_grid(get_local_mouse_position())
	if new_hover != hovered_cell:
		hovered_cell = new_hover
		if is_in_grid(move_preview_target) and hovered_cell != move_preview_target:
			clear_move_heat_preview()
		update_preview_path()
		queue_redraw()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		if is_busy:
			return
		if event.keycode == KEY_F5:
			load_debug_board("cascade")
			return
		elif event.keycode == KEY_F6:
			load_debug_board("blocked")
			return
		elif event.keycode == KEY_F7:
			load_debug_board("chain")
			return
		elif event.keycode == KEY_R:
			restart_game()
			return

	if game_over or is_busy:
		return
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		clear_move_heat_preview()
		var clicked_cell := pixel_to_grid(get_local_mouse_position())
		if not is_in_grid(clicked_cell):
			return

		print("Clicked cell: ", clicked_cell)
		await handle_cell_click(clicked_cell)
		queue_redraw()
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_RIGHT:
		var clicked_cell := pixel_to_grid(get_local_mouse_position())
		if not is_in_grid(clicked_cell):
			return
		preview_move_heat_update(clicked_cell)
		queue_redraw()

func _draw() -> void:
	draw_board_background()
	draw_cells()
	draw_path_preview()
	draw_blocked_target()
	draw_hover_cell()
	draw_selected_cell()

func setup_balls_layer() -> void:
	balls_layer = get_node_or_null("Balls")
	if balls_layer == null:
		balls_layer = Node2D.new()
		balls_layer.name = "Balls"
		add_child(balls_layer)

func setup_effects_layer() -> void:
	effects_layer = get_node_or_null("Effects")
	if effects_layer == null:
		effects_layer = Node2D.new()
		effects_layer.name = "Effects"
		add_child(effects_layer)
	effects_layer.z_index = 10

func initialize_board() -> void:
	board_state.clear()
	ball_nodes.clear()
	for y in range(GRID_SIZE):
		var row: Array = []
		for x in range(GRID_SIZE):
			row.append(null)
		board_state.append(row)

func place_test_balls() -> void:
	set_ball(Vector2i(2, 2), 1)
	set_ball(Vector2i(4, 4), 3)
	set_ball(Vector2i(6, 5), 5)

func handle_cell_click(cell: Vector2i) -> void:
	var ball: Variant = get_ball(cell)
	if ball != null:
		if selected_cell == cell:
			set_ball_selected(selected_cell, false)
			selected_cell = Vector2i(-1, -1)
			preview_path.clear()
			clear_move_heat_preview()
			print("Deselected: ", cell)
		else:
			if is_in_grid(selected_cell):
				set_ball_selected(selected_cell, false)
			selected_cell = cell
			set_ball_selected(selected_cell, true)
			clear_move_heat_preview()
			update_preview_path()
			print("Selected: ", cell, " heat=", ball["heat"])
	else:
		if is_in_grid(selected_cell):
			var moved: bool = await move_selected_ball_to(cell)
			if moved:
				await complete_player_turn()
		else:
			print("Empty cell clicked: ", cell)

func draw_board_background() -> void:
	var total_board_size := GRID_LENGTH * GRID_SIZE
	var bg_rect := Rect2(
		Vector2(WINDOW_BORDER, WINDOW_BORDER),
		Vector2(total_board_size, total_board_size)
	)
	draw_rect(bg_rect, BOARD_FILL_COLOR)

func draw_cells() -> void:
	for y in range(GRID_SIZE):
		for x in range(GRID_SIZE):
			var top_left := Vector2(
				WINDOW_BORDER + x * GRID_LENGTH,
				WINDOW_BORDER + y * GRID_LENGTH
			)

			var outer_rect := Rect2(
				top_left,
				Vector2(GRID_LENGTH, GRID_LENGTH)
			)
			draw_rect(outer_rect, CELL_BORDER_COLOR)

			var inner_rect := Rect2(
				top_left + Vector2(GRID_BORDER, GRID_BORDER),
				Vector2(GRID_SIDE, GRID_SIDE)
			)
			draw_rect(inner_rect, CELL_FILL_COLOR)

func draw_hover_cell() -> void:
	if not is_in_grid(hovered_cell):
		return

	var top_left := Vector2(
		WINDOW_BORDER + hovered_cell.x * GRID_LENGTH,
		WINDOW_BORDER + hovered_cell.y * GRID_LENGTH
	)

	var outer_rect := Rect2(top_left, Vector2(GRID_LENGTH, GRID_LENGTH))
	draw_rect(outer_rect, HOVER_BORDER_COLOR)

	var inner_rect := Rect2(
		top_left + Vector2(GRID_BORDER, GRID_BORDER),
		Vector2(GRID_SIDE, GRID_SIDE)
	)
	draw_rect(inner_rect, HOVER_FILL_COLOR)

func draw_selected_cell() -> void:
	if not is_in_grid(selected_cell):
		return

	var top_left := Vector2(
		WINDOW_BORDER + selected_cell.x * GRID_LENGTH,
		WINDOW_BORDER + selected_cell.y * GRID_LENGTH
	)

	var outer_rect := Rect2(top_left, Vector2(GRID_LENGTH, GRID_LENGTH))
	draw_rect(outer_rect, SELECTED_BORDER_COLOR)

	var inner_rect := Rect2(
		top_left + Vector2(GRID_BORDER, GRID_BORDER),
		Vector2(GRID_SIDE, GRID_SIDE)
	)
	draw_rect(inner_rect, SELECTED_FILL_COLOR)

func draw_path_preview() -> void:
	if preview_path.size() <= 1:
		return

	for index in range(1, preview_path.size()):
		var cell: Vector2i = preview_path[index]
		var top_left := Vector2(
			WINDOW_BORDER + cell.x * GRID_LENGTH,
			WINDOW_BORDER + cell.y * GRID_LENGTH
		)

		var outer_rect := Rect2(top_left, Vector2(GRID_LENGTH, GRID_LENGTH))
		draw_rect(outer_rect, PATH_BORDER_COLOR)

		var inner_rect := Rect2(
			top_left + Vector2(GRID_BORDER, GRID_BORDER),
			Vector2(GRID_SIDE, GRID_SIDE)
		)
		draw_rect(inner_rect, PATH_FILL_COLOR)

func draw_blocked_target() -> void:
	if not has_blocked_target_preview():
		return

	var top_left := Vector2(
		WINDOW_BORDER + hovered_cell.x * GRID_LENGTH,
		WINDOW_BORDER + hovered_cell.y * GRID_LENGTH
	)

	var outer_rect := Rect2(top_left, Vector2(GRID_LENGTH, GRID_LENGTH))
	draw_rect(outer_rect, BLOCKED_BORDER_COLOR)

	var inner_rect := Rect2(
		top_left + Vector2(GRID_BORDER, GRID_BORDER),
		Vector2(GRID_SIDE, GRID_SIDE)
	)
	draw_rect(inner_rect, BLOCKED_FILL_COLOR)

func pixel_to_grid(pixel_pos: Vector2) -> Vector2i:
	if not is_in_board_pixels(pixel_pos):
		return Vector2i(-1, -1)

	return Vector2i(
		int((pixel_pos.x - WINDOW_BORDER) / GRID_LENGTH),
		int((pixel_pos.y - WINDOW_BORDER) / GRID_LENGTH)
	)

func is_in_board_pixels(pixel_pos: Vector2) -> bool:
	var total_board_size := GRID_LENGTH * GRID_SIZE
	return (
		pixel_pos.x >= WINDOW_BORDER
		and pixel_pos.y >= WINDOW_BORDER
		and pixel_pos.x < WINDOW_BORDER + total_board_size
		and pixel_pos.y < WINDOW_BORDER + total_board_size
	)

func is_in_grid(grid_pos: Vector2i) -> bool:
	return (
		grid_pos.x >= 0
		and grid_pos.y >= 0
		and grid_pos.x < GRID_SIZE
		and grid_pos.y < GRID_SIZE
	)

func grid_to_pixel_center(grid_pos: Vector2i) -> Vector2:
	return Vector2(
		WINDOW_BORDER + GRID_BORDER + GRID_SIDE / 2.0 + grid_pos.x * GRID_LENGTH,
		WINDOW_BORDER + GRID_BORDER + GRID_SIDE / 2.0 + grid_pos.y * GRID_LENGTH
	)

func get_ball(grid_pos: Vector2i):
	if not is_in_grid(grid_pos):
		return null
	return board_state[grid_pos.y][grid_pos.x]

func set_ball(grid_pos: Vector2i, heat: int, play_feedback := false) -> void:
	if not is_in_grid(grid_pos):
		return
	board_state[grid_pos.y][grid_pos.x] = {
		"heat": heat
	}
	upsert_ball_node(grid_pos, heat)
	if play_feedback:
		var ball_node = ball_nodes.get(grid_pos)
		if ball_node != null:
			ball_node.play_spawn_feedback()

func clear_ball(grid_pos: Vector2i) -> void:
	if not is_in_grid(grid_pos):
		return

	board_state[grid_pos.y][grid_pos.x] = null
	var ball_node = ball_nodes.get(grid_pos)
	if ball_node != null:
		ball_node.queue_free()
		ball_nodes.erase(grid_pos)

func move_selected_ball_to(target_cell: Vector2i) -> bool:
	if not is_in_grid(selected_cell):
		return false
	if not is_in_grid(target_cell):
		return false
	if get_ball(target_cell) != null:
		return false

	var moving_ball: Variant = get_ball(selected_cell)
	if moving_ball == null:
		selected_cell = Vector2i(-1, -1)
		return false

	if not pathfinding.can_reach_cell(board_state, GRID_SIZE, selected_cell, target_cell):
		print("Blocked: ", selected_cell, " -> ", target_cell)
		emit_status("Blocked path")
		return false

	var from_cell := selected_cell
	var moving_node = ball_nodes.get(from_cell)
	var move_path: Array[Vector2i] = pathfinding.find_path(board_state, GRID_SIZE, selected_cell, target_cell)

	board_state[from_cell.y][from_cell.x] = null
	board_state[target_cell.y][target_cell.x] = moving_ball

	if moving_node != null:
		ball_nodes.erase(from_cell)
		ball_nodes[target_cell] = moving_node
		moving_node.set_selected(false)
		preview_path.clear()
		queue_redraw()
		await animate_ball_along_path(moving_node, move_path)

	selected_cell = Vector2i(-1, -1)
	preview_path.clear()
	clear_move_heat_preview()
	_last_move_target = target_cell
	print("Moved: ", from_cell, " -> ", target_cell)
	return true

func animate_ball_along_path(ball_node: Node2D, move_path: Array[Vector2i]) -> void:
	if move_path.size() <= 1:
		ball_node.position = grid_to_pixel_center(move_path.back())
		return

	var tween := create_tween()
	var step_duration := get_move_step_duration(move_path)
	for index in range(1, move_path.size()):
		tween.tween_property(
			ball_node,
			"position",
			grid_to_pixel_center(move_path[index]),
			step_duration
		).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	await tween.finished

func get_move_step_duration(move_path: Array[Vector2i]) -> float:
	var step_count: int = maxi(1, move_path.size() - 1)
	var capped_duration: float = MOVE_ANIMATION_MAX_TOTAL_SECONDS / float(step_count)
	return maxf(MOVE_ANIMATION_MIN_STEP_SECONDS, minf(MOVE_ANIMATION_SECONDS, capped_duration))

func update_preview_path() -> void:
	preview_path.clear()
	if not is_in_grid(selected_cell):
		return
	if not is_in_grid(hovered_cell):
		return
	if get_ball(hovered_cell) != null:
		return

	preview_path = pathfinding.find_path(board_state, GRID_SIZE, selected_cell, hovered_cell)

func has_blocked_target_preview() -> bool:
	if not is_in_grid(selected_cell):
		return false
	if not is_in_grid(hovered_cell):
		return false
	if selected_cell == hovered_cell:
		return false
	if get_ball(hovered_cell) != null:
		return false
	return preview_path.is_empty()

func preview_move_heat_update(target_cell: Vector2i) -> void:
	clear_move_heat_preview()
	if not move_preview_enabled:
		return
	if not is_in_grid(selected_cell):
		return
	if not is_in_grid(target_cell):
		return
	if get_ball(target_cell) != null:
		return
	if not pathfinding.can_reach_cell(board_state, GRID_SIZE, selected_cell, target_cell):
		emit_status("Preview blocked")
		return

	var preview_board := duplicate_board_state()
	var moving_ball: Variant = preview_board[selected_cell.y][selected_cell.x]
	if moving_ball == null:
		return

	preview_board[selected_cell.y][selected_cell.x] = null
	preview_board[target_cell.y][target_cell.x] = {"heat": int(moving_ball["heat"])}
	var heat_updates: Array[Dictionary] = rules.compute_heat_updates(preview_board, GRID_SIZE)
	apply_preview_heat_updates(preview_board, heat_updates)
	move_preview_target = target_cell
	render_move_preview_board(preview_board, target_cell)
	if heat_updates.is_empty():
		emit_status("Preview: no heat change")
	else:
		emit_status("Preview: heat only")

func apply_preview_heat_updates(preview_board: Array, heat_updates: Array[Dictionary]) -> void:
	for update in heat_updates:
		var cell: Vector2i = update["cell"]
		preview_board[cell.y][cell.x]["heat"] = int(update["new_heat"])

func render_move_preview_board(preview_board: Array, target_cell: Vector2i) -> void:
	clear_move_preview_nodes()
	# Hide the real ball at selected_cell so preview version replaces it visually
	if is_in_grid(selected_cell):
		var real_ball = ball_nodes.get(selected_cell)
		if real_ball != null:
			real_ball.visible = false
		_preview_hidden_cell = selected_cell
	add_move_preview_glow(target_cell)
	for y in range(GRID_SIZE):
		for x in range(GRID_SIZE):
			var cell := Vector2i(x, y)
			var ball: Variant = preview_board[y][x]
			if ball == null:
				continue
			add_move_preview_ball(cell, int(ball["heat"]))

func add_move_preview_ball(cell: Vector2i, heat: int) -> void:
	var ball_node = HEAT_BALL_SCENE.instantiate()
	effects_layer.add_child(ball_node)
	move_preview_nodes.append(ball_node)
	ball_node.radius = BALL_RADIUS
	ball_node.position = grid_to_pixel_center(cell)
	ball_node.modulate.a = 0.74
	ball_node.configure(
		heat,
		HEAT_FILL_COLORS.get(heat, Color.WHITE),
		HEAT_OUTLINE_COLORS.get(heat, Color.BLACK)
	)
	ball_node.set_heat_label_visible(show_heat_labels)

func add_move_preview_glow(target_cell: Vector2i) -> void:
	var glow := Panel.new()
	glow.mouse_filter = Control.MOUSE_FILTER_IGNORE
	glow.custom_minimum_size = Vector2(BALL_RADIUS * 2.55, BALL_RADIUS * 2.55)
	glow.position = grid_to_pixel_center(target_cell) - glow.custom_minimum_size * 0.5
	var style := StyleBoxFlat.new()
	style.bg_color = MOVE_PREVIEW_GLOW_COLOR
	style.border_color = MOVE_PREVIEW_BORDER_COLOR
	style.set_border_width_all(4)
	var radius := int(glow.custom_minimum_size.x * 0.5)
	style.corner_radius_top_left = radius
	style.corner_radius_top_right = radius
	style.corner_radius_bottom_left = radius
	style.corner_radius_bottom_right = radius
	glow.add_theme_stylebox_override("panel", style)
	effects_layer.add_child(glow)
	move_preview_nodes.append(glow)

func duplicate_board_state() -> Array:
	var clone: Array = []
	for y in range(GRID_SIZE):
		var row: Array = []
		for x in range(GRID_SIZE):
			var ball: Variant = board_state[y][x]
			if ball == null:
				row.append(null)
			else:
				row.append({"heat": int(ball["heat"])})
		clone.append(row)
	return clone

func clear_move_heat_preview() -> void:
	if move_preview_nodes.is_empty() and not is_in_grid(move_preview_target) and not is_in_grid(_preview_hidden_cell):
		return
	# Restore the real ball that was hidden during preview
	if is_in_grid(_preview_hidden_cell):
		var hidden_ball = ball_nodes.get(_preview_hidden_cell)
		if hidden_ball != null and is_instance_valid(hidden_ball):
			hidden_ball.visible = true
		_preview_hidden_cell = Vector2i(-1, -1)
	clear_move_preview_nodes()
	move_preview_target = Vector2i(-1, -1)
	queue_redraw()

func clear_move_preview_nodes() -> void:
	for node in move_preview_nodes:
		if is_instance_valid(node):
			node.queue_free()
	move_preview_nodes.clear()

func complete_player_turn() -> void:
	is_busy = true
	current_chain = 0
	turn_index += 1
	score += SURVIVAL_MOVE_SCORE
	score_event.emit("survival", {
		"amount": SURVIVAL_MOVE_SCORE,
		"cell": _last_move_target,
	})
	print("Turn ", turn_index, ": player moved")
	if chaos_mode_enabled:
		await complete_chaos_turn()
	else:
		await complete_tactical_turn()
	check_game_over()
	if not game_over:
		emit_status("Ready")
	is_busy = false

func complete_tactical_turn() -> void:
	emit_status("Resolving move")
	await resolve_system_turn()
	await get_tree().create_timer(SYSTEM_PHASE_PAUSE_SECONDS).timeout
	emit_status("Spawning next turn")
	await spawn_turn_balls()

func complete_chaos_turn() -> void:
	emit_status("Chaos spawn")
	await spawn_turn_balls()
	await get_tree().create_timer(SYSTEM_PHASE_PAUSE_SECONDS).timeout
	emit_status("Chaos resolve")
	await resolve_system_turn()

func spawn_turn_balls() -> void:
	if next_spawn_heats.is_empty():
		prepare_next_spawn_preview()

	var spawn_plan: Array[Dictionary] = turns.generate_spawn_plan_with_heats(
		board_state,
		GRID_SIZE,
		next_spawn_heats
	)
	if spawn_plan.is_empty():
		print("No empty cells left. Skipping spawn.")
		prepare_next_spawn_preview()
		emit_status("No spawn space")
		return

	for entry in spawn_plan:
		var cell: Vector2i = entry["cell"]
		var heat: int = entry["heat"]
		set_ball(cell, heat, true)
		print("Spawned heat ", heat, " at ", cell)
		await get_tree().create_timer(SPAWN_STAGGER_SECONDS).timeout

	prepare_next_spawn_preview()

func prepare_next_spawn_preview() -> void:
	next_spawn_heats = turns.generate_heat_preview(
		NEW_BALLS_PER_TURN,
		NEW_BALL_MIN_HEAT,
		NEW_BALL_MAX_HEAT
	)

func resolve_system_turn() -> void:
	var did_anything := false

	for cycle_index in range(MAX_SYSTEM_CYCLES):
		# Phase 1: heat update
		print("System cycle ", cycle_index + 1, ": Phase 1 heat update")
		emit_status("Heating cycle " + str(cycle_index + 1))
		var heat_updates: Array[Dictionary] = rules.compute_heat_updates(board_state, GRID_SIZE)
		var had_heat_change := not heat_updates.is_empty()
		if had_heat_change:
			did_anything = true
			await animate_heat_transfers(heat_updates)
			apply_heat_updates(heat_updates, "Heat")
			await get_tree().create_timer(SYSTEM_PHASE_PAUSE_SECONDS).timeout
		else:
			print("Phase 1: no heat changes")

		# Phase 2+3: process elimination groups one at a time for proper chaining.
		# Each group gets its own chain increment and aftershock,
		# then we re-check for new/remaining groups before continuing.
		var had_elimination := false

		while true:
			print("Phase 2: elimination check")
			emit_status("Checking clusters")
			var elimination_groups: Array[Dictionary] = rules.find_elimination_groups(board_state, GRID_SIZE)
			if elimination_groups.is_empty():
				break

			had_elimination = true
			did_anything = true

			# Process first elimination group only — enables proper chain feel
			var group: Dictionary = elimination_groups[0]
			var group_heat: int = group["heat"]
			var group_cells: Array = group["cells"]
			var eliminated_cells: Array[Vector2i] = []
			for c in group_cells:
				eliminated_cells.append(c as Vector2i)

			current_chain += 1
			max_chain = maxi(max_chain, current_chain)
			total_eliminated += eliminated_cells.size()

			# Score: per-group base × per-chain multiplier
			var group_base := eliminated_cells.size() * get_ball_clear_score(group_heat)
			var chain_mult := get_chain_score_multiplier(current_chain)
			score += group_base * chain_mult
			log_elimination_groups([group])

			# Emit score events for feedback
			score_event.emit("clear", {
				"heat": group_heat,
				"cell_count": eliminated_cells.size(),
				"base_score": group_base,
				"cells": group_cells,
			})
			if current_chain >= 2:
				score_event.emit("multiplier", {
					"chain_depth": current_chain,
					"multiplier": chain_mult,
				})
			emit_status("Chain " + str(current_chain) + ": cleared " + str(eliminated_cells.size()))
			play_elimination_feedback(eliminated_cells)
			await get_tree().create_timer(ELIMINATION_FEEDBACK_SECONDS).timeout
			clear_cells(eliminated_cells)

			# Phase 3: aftershock for this group only
			print("Phase 3: aftershock")
			emit_status("Aftershock")
			var aftershock_updates: Array[Dictionary] = rules.compute_aftershock_updates(
				board_state,
				GRID_SIZE,
				eliminated_cells
			)
			if not aftershock_updates.is_empty():
				await animate_aftershock_transfers(eliminated_cells, aftershock_updates)
				apply_heat_updates(aftershock_updates, "Aftershock")
				await get_tree().create_timer(SYSTEM_PHASE_PAUSE_SECONDS).timeout
			else:
				print("Phase 3: no aftershock heat changes")

			# Loop back: re-find groups (aftershock may have created new ones,
			# or remaining original groups may still qualify)

		# If no eliminations this cycle, system is resolved.
		# Heat oscillation without elimination is irrelevant to gameplay.
		if not had_elimination:
			if did_anything:
				print("System resolved after ", cycle_index + 1, " cycle(s)")
			else:
				print("System phase: no heat changes or eliminations")
			return

	print("System phase stopped after max cycle limit: ", MAX_SYSTEM_CYCLES)

func apply_heat_updates(heat_updates: Array[Dictionary], label: String) -> void:
	for update in heat_updates:
		var cell: Vector2i = update["cell"]
		var old_heat: int = update["old_heat"]
		var new_heat: int = update["new_heat"]
		board_state[cell.y][cell.x]["heat"] = new_heat
		upsert_ball_node(cell, new_heat, true)
		var ball_node = ball_nodes.get(cell)
		if ball_node != null:
			if label == "Aftershock":
				ball_node.play_aftershock_feedback()
			else:
				ball_node.play_heat_feedback()
		print(label, " updated at ", cell, ": ", old_heat, " -> ", new_heat)

func play_elimination_feedback(cells: Array[Vector2i]) -> void:
	for cell in cells:
		var ball_node = ball_nodes.get(cell)
		if ball_node != null:
			ball_node.play_elimination_feedback()

func animate_heat_transfers(heat_updates: Array[Dictionary]) -> void:
	var effect_count := 0
	for update in heat_updates:
		var old_heat: int = update["old_heat"]
		var new_heat: int = update["new_heat"]
		if new_heat <= old_heat:
			continue

		var target_cell: Vector2i = update["cell"]
		var source_cells := get_hotter_neighbor_cells(target_cell, old_heat)
		for source_cell in source_cells:
			var delay := effect_count * TRANSFER_STAGGER_SECONDS
			spawn_transfer_pixel(
				grid_to_pixel_center(source_cell),
				grid_to_pixel_center(target_cell),
				Color8(255, 208, 82),
				HEAT_TRANSFER_SECONDS,
				delay,
				8.0
			)
			effect_count += 1

	if effect_count > 0:
		var total_seconds := HEAT_TRANSFER_SECONDS + (effect_count - 1) * TRANSFER_STAGGER_SECONDS
		await get_tree().create_timer(total_seconds).timeout

func animate_aftershock_transfers(eliminated_cells: Array[Vector2i], heat_updates: Array[Dictionary]) -> void:
	var effect_count := 0
	for update in heat_updates:
		var target_cell: Vector2i = update["cell"]
		for source_cell in get_adjacent_cells_from(target_cell, eliminated_cells):
			var delay := effect_count * TRANSFER_STAGGER_SECONDS
			spawn_transfer_pixel(
				grid_to_pixel_center(source_cell),
				grid_to_pixel_center(target_cell),
				Color8(255, 86, 42),
				AFTERSHOCK_TRANSFER_SECONDS,
				delay,
				10.0
			)
			effect_count += 1

	if effect_count > 0:
		var total_seconds := AFTERSHOCK_TRANSFER_SECONDS + (effect_count - 1) * TRANSFER_STAGGER_SECONDS
		await get_tree().create_timer(total_seconds).timeout

func spawn_transfer_pixel(
	start_pos: Vector2,
	end_pos: Vector2,
	color: Color,
	duration: float,
	delay: float,
	size: float
) -> void:
	var pixel := ColorRect.new()
	pixel.color = color
	pixel.size = Vector2(size, size)
	pixel.pivot_offset = pixel.size * 0.5
	pixel.position = start_pos - pixel.pivot_offset
	pixel.modulate.a = 0.0
	effects_layer.add_child(pixel)

	var tween := create_tween()
	tween.tween_interval(delay)
	tween.set_parallel(true)
	tween.tween_property(pixel, "position", end_pos - pixel.pivot_offset, duration).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.tween_property(pixel, "modulate:a", 1.0, duration * 0.25).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.tween_property(pixel, "scale", Vector2.ONE * 1.35, duration * 0.5).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.chain().tween_property(pixel, "modulate:a", 0.0, duration * 0.28).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
	tween.finished.connect(pixel.queue_free)

func get_hotter_neighbor_cells(cell: Vector2i, heat: int) -> Array[Vector2i]:
	var source_cells: Array[Vector2i] = []
	for direction in ORTHOGONAL_DIRECTIONS:
		var neighbor_cell := cell + direction
		var neighbor_ball: Variant = get_ball(neighbor_cell)
		if neighbor_ball == null:
			continue
		if int(neighbor_ball["heat"]) > heat:
			source_cells.append(neighbor_cell)
	return source_cells

func get_adjacent_cells_from(cell: Vector2i, candidates: Array[Vector2i]) -> Array[Vector2i]:
	var source_cells: Array[Vector2i] = []
	for candidate in candidates:
		if abs(candidate.x - cell.x) + abs(candidate.y - cell.y) == 1:
			source_cells.append(candidate)
	return source_cells

func log_elimination_groups(elimination_groups: Array[Dictionary]) -> void:
	for group in elimination_groups:
		var heat: int = group["heat"]
		var threshold: int = group["threshold"]
		var cells: Array = group["cells"]
		print(
			"Eliminated heat ",
			heat,
			" group, size=",
			cells.size(),
			", threshold=",
			threshold,
			", cells=",
			cells
		)

func get_ball_clear_score(heat: int) -> int:
	return CLEAR_BASE_SCORE + CLEAR_HEAT_SCORE * heat

func get_chain_score_multiplier(chain_depth: int) -> int:
	var multiplier := 1
	for _index in range(maxi(0, chain_depth - 1)):
		multiplier *= 2
	return multiplier

func clear_cells(cells: Array[Vector2i]) -> void:
	for cell in cells:
		if cell == selected_cell:
			selected_cell = Vector2i(-1, -1)
		clear_ball(cell)

func emit_status(state_text: String) -> void:
	status_changed.emit({
		"turn": turn_index,
		"balls": count_balls(),
		"cleared": total_eliminated,
		"score": score,
		"chain": current_chain,
		"max_chain": max_chain,
		"next_heats": next_spawn_heats.duplicate(),
		"state": state_text,
		"game_over": game_over,
	})

func count_balls() -> int:
	var count := 0
	for y in range(GRID_SIZE):
		for x in range(GRID_SIZE):
			if board_state[y][x] != null:
				count += 1
	return count

func check_game_over() -> void:
	if pathfinding.has_any_reachable_move(board_state, GRID_SIZE):
		return

	game_over = true
	selected_cell = Vector2i(-1, -1)
	preview_path.clear()
	emit_status("Game over")
	print("Game over: no reachable moves remain.")

func set_ball_selected(grid_pos: Vector2i, selected: bool) -> void:
	var ball_node = ball_nodes.get(grid_pos)
	if ball_node != null:
		ball_node.set_selected(selected)

func upsert_ball_node(grid_pos: Vector2i, heat: int, animate_change := false) -> void:
	var ball_node = ball_nodes.get(grid_pos)
	if ball_node == null:
		ball_node = HEAT_BALL_SCENE.instantiate()
		balls_layer.add_child(ball_node)
		ball_nodes[grid_pos] = ball_node

	ball_node.radius = BALL_RADIUS
	ball_node.position = grid_to_pixel_center(grid_pos)
	ball_node.configure(
		heat,
		HEAT_FILL_COLORS.get(heat, Color.WHITE),
		HEAT_OUTLINE_COLORS.get(heat, Color.BLACK),
		not animate_change
	)
	ball_node.set_heat_label_visible(show_heat_labels)

func set_show_heat_labels(enabled: bool) -> void:
	show_heat_labels = enabled
	for ball_node in ball_nodes.values():
		if ball_node != null:
			ball_node.set_heat_label_visible(show_heat_labels)

func set_move_preview_enabled(enabled: bool) -> void:
	move_preview_enabled = enabled
	if not enabled:
		clear_move_heat_preview()

func set_chaos_mode_enabled(enabled: bool) -> void:
	chaos_mode_enabled = enabled
	clear_move_heat_preview()
	if chaos_mode_enabled:
		emit_status("Chaos mode")
	else:
		emit_status("Tactical mode")

func load_debug_board(board_name: String) -> void:
	print("Loading debug board: ", board_name)
	reset_runtime_state()
	clear_ball_nodes()
	initialize_board()

	if board_name == "cascade":
		place_debug_cascade_board()
	elif board_name == "blocked":
		place_debug_blocked_board()
	elif board_name == "chain":
		place_debug_chain_board()
	else:
		place_test_balls()

	prepare_next_spawn_preview()
	emit_status("Debug: " + board_name)
	queue_redraw()

func restart_game() -> void:
	print("Restarting game")
	reset_runtime_state()
	clear_ball_nodes()
	initialize_board()
	place_test_balls()
	prepare_next_spawn_preview()
	emit_status("Ready")
	queue_redraw()

func reset_runtime_state() -> void:
	selected_cell = Vector2i(-1, -1)
	hovered_cell = Vector2i(-1, -1)
	preview_path.clear()
	clear_move_heat_preview()
	_last_move_target = Vector2i(-1, -1)
	total_eliminated = 0
	score = 0
	current_chain = 0
	max_chain = 0
	next_spawn_heats.clear()
	turn_index = 0
	game_over = false
	is_busy = false

func clear_ball_nodes() -> void:
	for child in balls_layer.get_children():
		child.queue_free()
	ball_nodes.clear()
	if effects_layer != null:
		for child in effects_layer.get_children():
			child.queue_free()

func place_debug_cascade_board() -> void:
	set_ball(Vector2i(1, 1), 5)
	set_ball(Vector2i(2, 1), 5)
	set_ball(Vector2i(3, 1), 5)
	set_ball(Vector2i(2, 2), 4)
	set_ball(Vector2i(2, 3), 4)
	set_ball(Vector2i(1, 3), 4)
	set_ball(Vector2i(3, 3), 4)
	set_ball(Vector2i(7, 7), 2)

func place_debug_chain_board() -> void:
	# Multi-chain test board: 2-chain cascade with large elimination groups.
	# Row 0: heat 5 (5 balls) → chain 1 (threshold 3)
	# Row 1: heat 3 (5 balls) → Phase 1 raises to 4, aftershock to 5 → chain 2
	# Row 2: heat 1 (5 balls) → aftershock → heat 3, chain 2 collateral
	# Row 3: heat 1 (7 balls) → chain 1 collateral (threshold 7)
	# Corners: movable balls for the player
	set_ball(Vector2i(2, 0), 5)
	set_ball(Vector2i(3, 0), 5)
	set_ball(Vector2i(4, 0), 5)
	set_ball(Vector2i(5, 0), 5)
	set_ball(Vector2i(6, 0), 5)
	set_ball(Vector2i(2, 1), 3)
	set_ball(Vector2i(3, 1), 3)
	set_ball(Vector2i(4, 1), 3)
	set_ball(Vector2i(5, 1), 3)
	set_ball(Vector2i(6, 1), 3)
	set_ball(Vector2i(2, 2), 1)
	set_ball(Vector2i(3, 2), 1)
	set_ball(Vector2i(4, 2), 1)
	set_ball(Vector2i(5, 2), 1)
	set_ball(Vector2i(6, 2), 1)
	set_ball(Vector2i(1, 3), 1)
	set_ball(Vector2i(2, 3), 1)
	set_ball(Vector2i(3, 3), 1)
	set_ball(Vector2i(4, 3), 1)
	set_ball(Vector2i(5, 3), 1)
	set_ball(Vector2i(6, 3), 1)
	set_ball(Vector2i(7, 3), 1)
	set_ball(Vector2i(0, 8), 5)
	set_ball(Vector2i(8, 8), 4)

func place_debug_blocked_board() -> void:
	for x in range(GRID_SIZE):
		set_ball(Vector2i(x, 4), 2)
	set_ball(Vector2i(1, 1), 3)
	set_ball(Vector2i(7, 7), 1)
