extends SceneTree

const BoardRules = preload("res://scripts/board/board_rules.gd")


func _init() -> void:
	var rules := BoardRules.new()

	var tactical_board := make_two_hot_balls_board()
	var pre_spawn_updates: Array[Dictionary] = rules.compute_heat_updates(tactical_board, 3)
	apply_updates(tactical_board, pre_spawn_updates)
	var pre_spawn_groups: Array[Dictionary] = rules.find_elimination_groups(tactical_board, 3)
	if not pre_spawn_groups.is_empty():
		push_error("Tactical pre-spawn board should not clear before the new ball appears.")
		quit(1)
		return

	tactical_board[0][2] = {"heat": 5}
	var delayed_groups: Array[Dictionary] = rules.find_elimination_groups(tactical_board, 3)
	if delayed_groups.is_empty():
		push_error("The delayed spawn should create a group that waits until next turn.")
		quit(1)
		return

	var chaos_board := make_two_hot_balls_board()
	chaos_board[0][2] = {"heat": 5}
	var chaos_groups: Array[Dictionary] = rules.find_elimination_groups(chaos_board, 3)
	if chaos_groups.is_empty():
		push_error("Chaos mode should allow the spawned ball to clear immediately.")
		quit(1)
		return
	if (chaos_groups[0]["cells"] as Array).size() != 3:
		push_error("Chaos mode should clear the three heat 5 balls.")
		quit(1)
		return

	print("Turn order smoke tests passed.")
	quit(0)


func make_two_hot_balls_board() -> Array:
	return [
		[{"heat": 5}, {"heat": 5}, null],
		[null, null, null],
		[null, null, null],
	]


func apply_updates(board_state: Array, updates: Array[Dictionary]) -> void:
	for update in updates:
		var cell: Vector2i = update["cell"]
		board_state[cell.y][cell.x]["heat"] = int(update["new_heat"])
