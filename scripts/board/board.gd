extends Node2D

signal status_changed(snapshot: Dictionary)
signal score_event(event_type: String, data: Dictionary)
signal score_feedback_completed(event_id: int)

const BoardPathfinding = preload("res://scripts/board/board_pathfinding.gd")
const BoardEffects = preload("res://scripts/board/board_effects.gd")
const BoardPresets = preload("res://scripts/board/board_presets.gd")
const BoardRules = preload("res://scripts/board/board_rules.gd")
const BoardScoring = preload("res://scripts/board/board_scoring.gd")
const BoardTurns = preload("res://scripts/board/board_turns.gd")
const GameFeel = preload("res://scripts/core/game_feel.gd")

const GRID_SIZE := 9
const GRID_SIDE := 66
const GRID_BORDER := 8
const GRID_LENGTH := GRID_SIDE + GRID_BORDER * 2
const WINDOW_BORDER := 30
const BALL_RADIUS := 27.0
const NEW_BALLS_PER_TURN := 3
const NEW_BALL_MIN_HEAT := 1
const NEW_BALL_MAX_HEAT := 3
const MAX_SYSTEM_CYCLES := 64
const ELIMINATION_FEEDBACK_SECONDS := 0.72
const SYSTEM_PHASE_PAUSE_SECONDS := 0.18
const SPAWN_STAGGER_SECONDS := 0.09
const SCORE_FEEDBACK_TIMEOUT_SECONDS := 5.0
const SCORE_FEEDBACK_POLL_SECONDS := 0.05

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
var next_score_event_id: int = 1
var completed_score_event_ids: Dictionary = {}
var next_spawn_heats: Array[int] = []
var game_over := false
var is_busy := false
var show_heat_labels := true
var move_preview_enabled := true
var chaos_mode_enabled := false
var score_feedback_sync_enabled := false
var move_preview_target := Vector2i(-1, -1)
var move_preview_nodes: Array[Node] = []
var _preview_hidden_cell := Vector2i(-1, -1)
var _last_move_target := Vector2i(-1, -1)
var _turn_generation: int = 0
var effects: BoardEffects
var pathfinding: BoardPathfinding
var presets: BoardPresets
var rules: BoardRules
var scoring: BoardScoring
var turns: BoardTurns

func _ready() -> void:
	print("Board is ready")
	effects = BoardEffects.new()
	pathfinding = BoardPathfinding.new()
	presets = BoardPresets.new()
	rules = BoardRules.new()
	scoring = BoardScoring.new()
	turns = BoardTurns.new()
	setup_balls_layer()
	setup_effects_layer()
	initialize_board()
	place_balls(presets.get_initial_balls())
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

func place_balls(ball_entries: Array[Dictionary]) -> void:
	for entry in ball_entries:
		set_ball(entry["cell"], int(entry["heat"]))

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
	var action_generation := _turn_generation
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
		await effects.animate_ball_along_path(self, moving_node, move_path, grid_to_pixel_center)
		if not is_turn_generation_current(action_generation):
			return false

	selected_cell = Vector2i(-1, -1)
	preview_path.clear()
	clear_move_heat_preview()
	_last_move_target = target_cell
	print("Moved: ", from_cell, " -> ", target_cell)
	return true

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
	var turn_generation := _turn_generation
	is_busy = true
	current_chain = 0
	turn_index += 1
	var survival_score := scoring.get_survival_move_score()
	score += survival_score
	score_event.emit("survival", {
		"amount": survival_score,
		"cell": _last_move_target,
		"target_score": score,
		"turn_generation": turn_generation,
	})
	print("Turn ", turn_index, ": player moved")
	if chaos_mode_enabled:
		if not await complete_chaos_turn(turn_generation):
			return
	else:
		if not await complete_tactical_turn(turn_generation):
			return
	if not is_turn_generation_current(turn_generation):
		return
	check_game_over()
	if not game_over:
		emit_status("Ready")
	is_busy = false

func complete_tactical_turn(turn_generation: int) -> bool:
	emit_status("Resolving move")
	if not await resolve_system_turn(turn_generation):
		return false
	await get_tree().create_timer(SYSTEM_PHASE_PAUSE_SECONDS).timeout
	if not is_turn_generation_current(turn_generation):
		return false
	emit_status("Spawning next turn")
	return await spawn_turn_balls(turn_generation)

func complete_chaos_turn(turn_generation: int) -> bool:
	emit_status("Chaos spawn")
	if not await spawn_turn_balls(turn_generation):
		return false
	await get_tree().create_timer(SYSTEM_PHASE_PAUSE_SECONDS).timeout
	if not is_turn_generation_current(turn_generation):
		return false
	emit_status("Chaos resolve")
	return await resolve_system_turn(turn_generation)

func spawn_turn_balls(turn_generation: int = -1) -> bool:
	if turn_generation < 0:
		turn_generation = _turn_generation
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
		return true

	for entry in spawn_plan:
		if not is_turn_generation_current(turn_generation):
			return false
		var cell: Vector2i = entry["cell"]
		var heat: int = entry["heat"]
		set_ball(cell, heat, true)
		print("Spawned heat ", heat, " at ", cell)
		await get_tree().create_timer(SPAWN_STAGGER_SECONDS).timeout
		if not is_turn_generation_current(turn_generation):
			return false

	prepare_next_spawn_preview()
	return true

func prepare_next_spawn_preview() -> void:
	next_spawn_heats = turns.generate_heat_preview(
		NEW_BALLS_PER_TURN,
		NEW_BALL_MIN_HEAT,
		NEW_BALL_MAX_HEAT
	)

func resolve_system_turn(turn_generation: int = -1) -> bool:
	if turn_generation < 0:
		turn_generation = _turn_generation
	var did_anything := false

	for cycle_index in range(MAX_SYSTEM_CYCLES):
		if not is_turn_generation_current(turn_generation):
			return false
		# Phase 1: heat update
		print("System cycle ", cycle_index + 1, ": Phase 1 heat update")
		emit_status("Heating cycle " + str(cycle_index + 1))
		var heat_updates: Array[Dictionary] = rules.compute_heat_updates(board_state, GRID_SIZE)
		var had_heat_change := not heat_updates.is_empty()
		if had_heat_change:
			did_anything = true
			await effects.animate_heat_transfers(
				self,
				effects_layer,
				heat_updates,
				get_hotter_neighbor_cells_by_target(heat_updates),
				grid_to_pixel_center
			)
			if not is_turn_generation_current(turn_generation):
				return false
			apply_heat_updates(heat_updates, "Heat")
			await get_tree().create_timer(SYSTEM_PHASE_PAUSE_SECONDS).timeout
			if not is_turn_generation_current(turn_generation):
				return false
		else:
			print("Phase 1: no heat changes")

		# Phase 2+3: process elimination groups one at a time for proper chaining.
		# Each group gets its own chain increment and aftershock,
		# then we re-check for new/remaining groups before continuing.
		var had_elimination := false

		while true:
			if not is_turn_generation_current(turn_generation):
				return false
			print("Phase 2: elimination check")
			emit_status("Checking clusters")
			var elimination_groups: Array[Dictionary] = rules.find_elimination_groups(board_state, GRID_SIZE)
			if elimination_groups.is_empty():
				break

			had_elimination = true
			did_anything = true

			# Process first elimination group only; this keeps chains readable.
			var group: Dictionary = elimination_groups[0]
			var group_heat: int = group["heat"]
			var group_cells: Array = group["cells"]
			var eliminated_cells: Array[Vector2i] = []
			for c in group_cells:
				eliminated_cells.append(c as Vector2i)

			current_chain += 1
			max_chain = maxi(max_chain, current_chain)
			total_eliminated += eliminated_cells.size()

			# Score: per-group base times per-chain multiplier.
			var group_base := eliminated_cells.size() * scoring.get_ball_clear_score(group_heat)
			var chain_mult := scoring.get_chain_score_multiplier(current_chain)
			var final_score: int = scoring.get_group_clear_score(
				group_heat,
				eliminated_cells.size(),
				current_chain
			)
			score += final_score
			log_elimination_groups([group])
			var score_event_id := next_score_event_id
			next_score_event_id += 1

			# Emit per-ball score data for the visual feedback pipeline.
			var balls_data: Array[Dictionary] = []
			for c in eliminated_cells:
				balls_data.append({
					"cell": c,
					"heat": group_heat,
					"base_score": scoring.get_ball_clear_score(group_heat),
				})
			score_event.emit("clear", {
				"heat": group_heat,
				"chain_depth": current_chain,
				"multiplier": chain_mult,
				"balls": balls_data,
				"final_score": final_score,
				"target_score": score,
				"event_id": score_event_id,
				"turn_generation": turn_generation,
			})
			emit_status("Chain " + str(current_chain) + ": cleared " + str(eliminated_cells.size()))
			effects.play_elimination_feedback(eliminated_cells, ball_nodes)
			if group_heat >= 4:
				GameFeel.hitstop(0.08 if group_heat >= 5 else 0.05)
			await get_tree().create_timer(ELIMINATION_FEEDBACK_SECONDS).timeout
			if not is_turn_generation_current(turn_generation):
				return false
			clear_cells(eliminated_cells)
			if not await wait_for_score_feedback(score_event_id, turn_generation):
				return false

			# Phase 3: aftershock for this group only
			print("Phase 3: aftershock")
			emit_status("Aftershock")
			var aftershock_updates: Array[Dictionary] = rules.compute_aftershock_updates(
				board_state,
				GRID_SIZE,
				eliminated_cells
			)
			if not aftershock_updates.is_empty():
				await effects.animate_aftershock_transfers(
					self,
					effects_layer,
					eliminated_cells,
					aftershock_updates,
					grid_to_pixel_center
				)
				if not is_turn_generation_current(turn_generation):
					return false
				apply_heat_updates(aftershock_updates, "Aftershock")
				GameFeel.screen_shake(self, 4.0, 0.18)
				await get_tree().create_timer(SYSTEM_PHASE_PAUSE_SECONDS).timeout
				if not is_turn_generation_current(turn_generation):
					return false
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
			return true

	print("System phase stopped after max cycle limit: ", MAX_SYSTEM_CYCLES)
	return true

func apply_heat_updates(heat_updates: Array[Dictionary], label: String) -> void:
	for update in heat_updates:
		var cell: Vector2i = update["cell"]
		var old_heat: int = update["old_heat"]
		var new_heat: int = update["new_heat"]
		var ball: Variant = get_ball(cell)
		if ball == null:
			print(label, " skipped at ", cell, ": ball no longer exists")
			continue
		ball["heat"] = new_heat
		upsert_ball_node(cell, new_heat, true)
		var ball_node = ball_nodes.get(cell)
		if ball_node != null:
			if label == "Aftershock":
				ball_node.play_aftershock_feedback()
			else:
				ball_node.play_heat_feedback()
		print(label, " updated at ", cell, ": ", old_heat, " -> ", new_heat)

func get_hotter_neighbor_cells_by_target(heat_updates: Array[Dictionary]) -> Dictionary:
	var source_cells_by_target: Dictionary = {}
	for update in heat_updates:
		var old_heat: int = update["old_heat"]
		var new_heat: int = update["new_heat"]
		if new_heat <= old_heat:
			continue

		var target_cell: Vector2i = update["cell"]
		source_cells_by_target[target_cell] = get_hotter_neighbor_cells(target_cell, old_heat)

	return source_cells_by_target

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
	return scoring.get_ball_clear_score(heat)

func get_chain_score_multiplier(chain_depth: int) -> int:
	return scoring.get_chain_score_multiplier(chain_depth)

func notify_score_feedback_complete(event_id: int, turn_generation: int = -1) -> void:
	if turn_generation >= 0 and not is_turn_generation_current(turn_generation):
		return
	completed_score_event_ids[event_id] = true
	score_feedback_completed.emit(event_id)

func register_score_feedback_sync() -> void:
	score_feedback_sync_enabled = true

func wait_for_score_feedback(event_id: int, turn_generation: int = -1) -> bool:
	if turn_generation < 0:
		turn_generation = _turn_generation
	if not score_feedback_sync_enabled:
		return true
	if completed_score_event_ids.has(event_id):
		completed_score_event_ids.erase(event_id)
		return true

	var waited_seconds := 0.0
	while waited_seconds < SCORE_FEEDBACK_TIMEOUT_SECONDS:
		if not is_turn_generation_current(turn_generation):
			return false
		if completed_score_event_ids.has(event_id):
			completed_score_event_ids.erase(event_id)
			return true
		await get_tree().create_timer(SCORE_FEEDBACK_POLL_SECONDS).timeout
		waited_seconds += SCORE_FEEDBACK_POLL_SECONDS

	push_warning("Score feedback timed out for event " + str(event_id) + ". Continuing turn resolution.")
	return is_turn_generation_current(turn_generation)

func is_turn_generation_current(turn_generation: int) -> bool:
	return turn_generation == _turn_generation

func get_turn_generation() -> int:
	return _turn_generation

func advance_turn_generation() -> void:
	_turn_generation += 1
	completed_score_event_ids.clear()

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
	_play_game_over_effects()
	emit_status("Game over")
	print("Game over: no reachable moves remain.")

func _play_game_over_effects() -> void:
	GameFeel.hitstop(0.35, 0.05)
	GameFeel.screen_shake(self, 8.0, 0.6)
	var tween := create_tween()
	tween.tween_property(self, "modulate", Color(0.4, 0.4, 0.4, 1.0), 0.8).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)

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
	place_balls(presets.get_debug_balls(board_name))

	prepare_next_spawn_preview()
	emit_status("Debug: " + board_name)
	queue_redraw()

func restart_game() -> void:
	print("Restarting game")
	reset_runtime_state()
	clear_ball_nodes()
	initialize_board()
	place_balls(presets.get_initial_balls())
	prepare_next_spawn_preview()
	emit_status("Ready")
	queue_redraw()

func reset_runtime_state() -> void:
	modulate = Color.WHITE
	advance_turn_generation()
	score_event.emit("reset", {
		"turn_generation": _turn_generation,
	})
	selected_cell = Vector2i(-1, -1)
	hovered_cell = Vector2i(-1, -1)
	preview_path.clear()
	clear_move_heat_preview()
	_last_move_target = Vector2i(-1, -1)
	total_eliminated = 0
	score = 0
	current_chain = 0
	max_chain = 0
	next_score_event_id = 1
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
