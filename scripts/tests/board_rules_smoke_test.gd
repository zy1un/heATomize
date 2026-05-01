extends SceneTree

const BoardRules = preload("res://scripts/board/board_rules.gd")

var rules := BoardRules.new()


func _init() -> void:
	test_heat_phase()
	test_simultaneous_heat_updates()
	test_elimination_groups()
	test_aftershock_updates()
	test_aftershock_deduplicates_neighbors()
	print("BoardRules smoke tests passed.")
	quit(0)


func test_heat_phase() -> void:
	var board := create_board(3)
	set_ball(board, Vector2i(1, 1), 3)
	set_ball(board, Vector2i(2, 1), 4)
	assert_equal(
		rules.compute_next_heat(board, 3, Vector2i(1, 1), 3),
		4,
		"hotter neighbor raises heat"
	)

	board = create_board(3)
	set_ball(board, Vector2i(1, 1), 3)
	assert_equal(
		rules.compute_next_heat(board, 3, Vector2i(1, 1), 3),
		2,
		"isolated ball cools down"
	)

	board = create_board(3)
	set_ball(board, Vector2i(1, 1), 3)
	set_ball(board, Vector2i(2, 1), 3)
	assert_equal(
		rules.compute_next_heat(board, 3, Vector2i(1, 1), 3),
		3,
		"same heat neighbor keeps heat stable"
	)

	board = create_board(3)
	set_ball(board, Vector2i(1, 1), 3)
	set_ball(board, Vector2i(2, 1), 2)
	assert_equal(
		rules.compute_next_heat(board, 3, Vector2i(1, 1), 3),
		2,
		"all colder neighbors cool the ball"
	)

	board = create_board(3)
	set_ball(board, Vector2i(1, 1), 3)
	set_ball(board, Vector2i(2, 1), 2)
	set_ball(board, Vector2i(0, 1), 3)
	assert_equal(
		rules.compute_next_heat(board, 3, Vector2i(1, 1), 3),
		3,
		"same and colder neighbors keep heat stable"
	)


func test_simultaneous_heat_updates() -> void:
	var board := create_board(3)
	set_ball(board, Vector2i(0, 1), 4)
	set_ball(board, Vector2i(1, 1), 3)
	set_ball(board, Vector2i(2, 1), 2)

	var updates: Array[Dictionary] = rules.compute_heat_updates(board, 3)
	assert_equal(updates.size(), 3, "three balls update from old values")
	assert_update(updates, Vector2i(0, 1), 4, 3)
	assert_update(updates, Vector2i(1, 1), 3, 4)
	assert_update(updates, Vector2i(2, 1), 2, 3)


func test_elimination_groups() -> void:
	var board := create_board(3)
	set_ball(board, Vector2i(0, 0), 5)
	set_ball(board, Vector2i(1, 0), 5)
	set_ball(board, Vector2i(2, 0), 5)

	var groups: Array[Dictionary] = rules.find_elimination_groups(board, 3)
	assert_equal(groups.size(), 1, "three heat-5 balls eliminate")
	assert_equal(groups[0]["heat"], 5, "elimination group keeps heat")
	assert_equal(groups[0]["cells"].size(), 3, "elimination group keeps cells")

	board = create_board(3)
	set_ball(board, Vector2i(0, 0), 4)
	set_ball(board, Vector2i(1, 0), 4)
	set_ball(board, Vector2i(2, 0), 4)
	groups = rules.find_elimination_groups(board, 3)
	assert_equal(groups.size(), 0, "three heat-4 balls stay below threshold")


func test_aftershock_updates() -> void:
	var board := create_board(3)
	set_ball(board, Vector2i(1, 0), 4)
	set_ball(board, Vector2i(0, 1), 5)

	var removed_cells: Array[Vector2i] = [Vector2i(1, 1)]
	var updates: Array[Dictionary] = rules.compute_aftershock_updates(board, 3, removed_cells)

	assert_equal(updates.size(), 1, "aftershock skips capped heat-5 balls")
	assert_equal(updates[0]["cell"], Vector2i(1, 0), "aftershock targets orthogonal neighbor")
	assert_equal(updates[0]["new_heat"], 5, "aftershock raises heat by one")


func test_aftershock_deduplicates_neighbors() -> void:
	var board := create_board(3)
	set_ball(board, Vector2i(1, 0), 3)

	var removed_cells: Array[Vector2i] = [Vector2i(0, 0), Vector2i(2, 0), Vector2i(1, 1)]
	var updates: Array[Dictionary] = rules.compute_aftershock_updates(board, 3, removed_cells)
	assert_equal(updates.size(), 1, "aftershock applies once per affected ball")
	assert_equal(updates[0]["old_heat"], 3, "deduplicated aftershock keeps old heat")
	assert_equal(updates[0]["new_heat"], 4, "deduplicated aftershock raises once")


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

	push_error(
		message
		+ ": expected "
		+ str(expected)
		+ ", got "
		+ str(actual)
	)
	quit(1)


func assert_update(updates: Array[Dictionary], cell: Vector2i, old_heat: int, new_heat: int) -> void:
	for update in updates:
		if update["cell"] != cell:
			continue
		assert_equal(update["old_heat"], old_heat, "old heat for " + str(cell))
		assert_equal(update["new_heat"], new_heat, "new heat for " + str(cell))
		return

	push_error("missing update for " + str(cell))
	quit(1)
