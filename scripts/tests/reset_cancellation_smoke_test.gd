extends SceneTree

const MAIN_SCENE := preload("res://scenes/Main.tscn")


func _init() -> void:
	await process_frame

	var main := MAIN_SCENE.instantiate()
	root.add_child(main)
	await process_frame

	var board = main.get_node("Board")
	board.load_debug_board("cascade")
	await process_frame

	board.resolve_system_turn()
	await create_timer(0.1).timeout
	board.restart_game()
	var reset_generation: int = board.get_turn_generation()

	await create_timer(1.2).timeout
	assert_equal(board.get_turn_generation(), reset_generation, "reset keeps current generation")
	assert_equal(board.turn_index, 0, "stale resolve does not advance turn")
	assert_equal(board.count_balls(), 3, "stale resolve does not mutate reset board")
	assert_true(not board.is_busy, "board is not stuck busy after reset cancellation")

	print("Reset cancellation smoke tests passed.")
	quit(0)


func assert_equal(actual: Variant, expected: Variant, message: String) -> void:
	if actual == expected:
		return

	push_error(message + ": expected " + str(expected) + ", got " + str(actual))
	quit(1)


func assert_true(condition: bool, message: String) -> void:
	if condition:
		return

	push_error(message + ": expected true, got false")
	quit(1)
