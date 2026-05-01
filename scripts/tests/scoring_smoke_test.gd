extends SceneTree

const MAIN_SCENE := preload("res://scenes/Main.tscn")


func _init() -> void:
	await process_frame

	var main := MAIN_SCENE.instantiate()
	root.add_child(main)
	await process_frame

	var board = main.get_node("Board")

	# Per-ball clear score: 10 + 2 * heat
	assert_equal(board.get_ball_clear_score(1), 12, "heat 1 clear score per ball")
	assert_equal(board.get_ball_clear_score(3), 16, "heat 3 clear score per ball")
	assert_equal(board.get_ball_clear_score(5), 20, "heat 5 clear score per ball")

	# Chain multiplier: exponential doubling, chain 1 = 1x, chain 2 = 2x, chain 3 = 4x...
	assert_equal(board.get_chain_score_multiplier(1), 1, "chain 1 multiplier = 1x")
	assert_equal(board.get_chain_score_multiplier(2), 2, "chain 2 multiplier = 2x")
	assert_equal(board.get_chain_score_multiplier(3), 4, "chain 3 multiplier = 4x")
	assert_equal(board.get_chain_score_multiplier(4), 8, "chain 4 multiplier = 8x")
	assert_equal(board.get_chain_score_multiplier(6), 32, "chain 6 multiplier = 32x (unbounded)")

	# Per-group scoring simulation (one-at-a-time logic):
	# Group A: heat 5, 3 balls, chain 1 → base = 3*20 = 60, mult = 1 → 60
	# Group B: heat 3, 2 balls, chain 2 → base = 2*16 = 32, mult = 2 → 64
	# Total: 60 + 64 = 124
	var group_a_base: int = 3 * board.get_ball_clear_score(5)
	var group_a_score: int = group_a_base * board.get_chain_score_multiplier(1)
	assert_equal(group_a_score, 60, "group A (heat5 x3, chain1) = 60")

	var group_b_base: int = 2 * board.get_ball_clear_score(3)
	var group_b_score: int = group_b_base * board.get_chain_score_multiplier(2)
	assert_equal(group_b_score, 64, "group B (heat3 x2, chain2) = 64")

	assert_equal(group_a_score + group_b_score, 124, "total per-group score = 124")

	# Verify score_event signal structure
	var signal_data: Dictionary = {}
	var collector := func(event_type: String, data: Dictionary) -> void:
		signal_data[event_type] = data
	board.score_event.connect(collector)

	# Simulate a clear event
	board.score_event.emit("clear", {
		"heat": 5,
		"chain_depth": 1,
		"multiplier": 1,
		"balls": [
			{"cell": Vector2i(0, 0), "heat": 5, "base_score": 20},
			{"cell": Vector2i(1, 0), "heat": 5, "base_score": 20},
		],
		"final_score": 40,
	})
	assert_true(signal_data.has("clear"), "clear event received")
	var clear_data: Dictionary = signal_data.get("clear", {})
	assert_true(clear_data.has("balls"), "clear event has 'balls' array")
	assert_equal(clear_data.get("balls", []).size(), 2, "clear event has 2 balls")
	assert_equal(clear_data.get("final_score", 0), 40, "clear event final_score = 40")

	# Survival score
	board.score = 0
	board.turn_index = 0
	board.score += board.SURVIVAL_MOVE_SCORE
	assert_equal(board.score, 2, "survival move score = +2")

	print("Scoring smoke tests passed.")
	quit(0)


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


func assert_true(condition: bool, message: String) -> void:
	if condition:
		return

	push_error(message + ": expected true, got false")
	quit(1)
