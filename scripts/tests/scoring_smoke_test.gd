extends SceneTree

const MAIN_SCENE := preload("res://scenes/Main.tscn")


func _init() -> void:
	await process_frame

	var main := MAIN_SCENE.instantiate()
	root.add_child(main)
	await process_frame

	var board = main.get_node("Board")
	assert_equal(board.get_ball_clear_score(1), 12, "heat 1 score")
	assert_equal(board.get_ball_clear_score(5), 20, "heat 5 score")
	assert_equal(board.get_chain_score_multiplier(1), 1, "first clear multiplier")
	assert_equal(board.get_chain_score_multiplier(2), 2, "aftershock multiplier")
	assert_equal(board.get_chain_score_multiplier(4), 8, "unbounded doubling multiplier")

	var groups: Array[Dictionary] = [
		{
			"heat": 5,
			"threshold": 3,
			"cells": [Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0)],
		},
		{
			"heat": 3,
			"threshold": 5,
			"cells": [Vector2i(0, 1), Vector2i(1, 1)],
		},
	]
	assert_equal(board.calculate_elimination_score(groups, 2), 184, "group score with aftershock multiplier")

	board.score = 0
	board.turn_index = 0
	board.score += board.SURVIVAL_MOVE_SCORE
	assert_equal(board.score, 2, "survival move score")

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
