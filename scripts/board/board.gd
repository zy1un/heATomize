extends Node2D

const GRID_SIZE := 9
const GRID_SIDE := 44
const GRID_BORDER := 5
const GRID_LENGTH := GRID_SIDE + GRID_BORDER * 2
const WINDOW_BORDER := 20
const BALL_RADIUS := 16.0

const BOARD_FILL_COLOR := Color8(48, 44, 40)
const CELL_FILL_COLOR := Color8(245, 240, 235)
const CELL_BORDER_COLOR := Color8(58, 58, 58, 128)
const HOVER_FILL_COLOR := Color8(58, 58, 58, 128)
const HOVER_BORDER_COLOR := Color8(160, 130, 90)
const SELECTED_FILL_COLOR := Color8(192, 192, 192, 180)
const SELECTED_BORDER_COLOR := Color8(160, 130, 90)

const HEAT_COLORS := {
	1: Color8(100, 149, 237),
	2: Color8(46, 180, 100),
	3: Color8(255, 191, 0),
	4: Color8(230, 140, 60),
	5: Color8(220, 60, 80),
}

var board_state: Array = []

var hovered_cell := Vector2i(-1, -1)
var selected_cell := Vector2i(-1, -1)

func _ready() -> void:
	print("Board is ready")
	initialize_board()
	place_test_balls()
	queue_redraw()

func _process(_delta: float) -> void:
	var new_hover := pixel_to_grid(get_local_mouse_position())
	if new_hover != hovered_cell:
		hovered_cell = new_hover
		queue_redraw()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		var clicked_cell := pixel_to_grid(get_local_mouse_position())
		if not is_in_grid(clicked_cell):
			return

		print("Clicked cell: ", clicked_cell)
		handle_cell_click(clicked_cell)
		queue_redraw()

func _draw() -> void:
	draw_board_background()
	draw_cells()
	draw_hover_cell()
	draw_selected_cell()
	draw_balls()

func initialize_board() -> void:
	board_state.clear()
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
	var ball := get_ball(cell)
	if ball != null:
		if selected_cell == cell:
			selected_cell = Vector2i(-1, -1)
			print("Deselected: ", cell)
		else:
			selected_cell = cell
			print("Selected: ", cell, " heat=", ball["heat"])
	else:
		if is_in_grid(selected_cell):
			print("Empty cell clicked: ", cell, " (movement comes next)")
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

func draw_balls() -> void:
	for y in range(GRID_SIZE):
		for x in range(GRID_SIZE):
			var ball := board_state[y][x]
			if ball == null:
				continue

			var grid_pos := Vector2i(x, y)
			var center := grid_to_pixel_center(grid_pos)
			var fill_color: Color = HEAT_COLORS.get(ball["heat"], Color.WHITE)
			var outline_color := Color(fill_color.r * 0.65, fill_color.g * 0.65, fill_color.b * 0.65, 1.0)

			draw_circle(center, BALL_RADIUS, fill_color)
			draw_arc(center, BALL_RADIUS, 0.0, TAU, 24, outline_color, 4.0)

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

func set_ball(grid_pos: Vector2i, heat: int) -> void:
	if not is_in_grid(grid_pos):
		return
	board_state[grid_pos.y][grid_pos.x] = {
		"heat": heat
	}
