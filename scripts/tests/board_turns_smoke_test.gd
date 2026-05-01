extends SceneTree

const BoardTurns = preload("res://scripts/board/board_turns.gd")

var turns := BoardTurns.new()


func _init() -> void:
	test_heat_preview_count()
	test_spawn_plan_uses_preview_heats()
	test_spawn_plan_limits_to_empty_cells()
	print("BoardTurns smoke tests passed.")
	quit(0)


func test_heat_preview_count() -> void:
	var preview: Array[int] = turns.generate_heat_preview(3, 1, 3)
	assert_equal(preview.size(), 3, "preview has requested count")
	for heat in preview:
		if heat < 1 or heat > 3:
			push_error("preview heat out of range: " + str(heat))
			quit(1)


func test_spawn_plan_uses_preview_heats() -> void:
	var board := create_board(3)
	var preview: Array[int] = [1, 2, 3]
	var plan: Array[Dictionary] = turns.generate_spawn_plan_with_heats(board, 3, preview)

	assert_equal(plan.size(), 3, "spawn plan uses all preview heats when space exists")
	assert_equal(plan[0]["heat"], 1, "first preview heat is preserved")
	assert_equal(plan[1]["heat"], 2, "second preview heat is preserved")
	assert_equal(plan[2]["heat"], 3, "third preview heat is preserved")


func test_spawn_plan_limits_to_empty_cells() -> void:
	var board := create_board(2)
	set_ball(board, Vector2i(0, 0), 1)
	set_ball(board, Vector2i(1, 0), 1)
	set_ball(board, Vector2i(0, 1), 1)

	var preview: Array[int] = [1, 2, 3]
	var plan: Array[Dictionary] = turns.generate_spawn_plan_with_heats(board, 2, preview)
	assert_equal(plan.size(), 1, "spawn plan is capped by empty cell count")
	assert_equal(plan[0]["heat"], 1, "capped plan consumes preview order")


func create_board(grid_size: int) -> Array:
	var board: Array = []
	for _y in range(grid_size):
		var row: Array = []
		for _x in range(grid_size):
			row.append(null)
		board.append(row)
	return board


func set_ball(board: Array, cell: Vector2i, heat: int) -> void:
	board[cell.y][cell.x] = {
		"heat": heat,
	}


func assert_equal(actual: Variant, expected: Variant, message: String) -> void:
	if actual == expected:
		return

	push_error(message + ": expected " + str(expected) + ", got " + str(actual))
	quit(1)
