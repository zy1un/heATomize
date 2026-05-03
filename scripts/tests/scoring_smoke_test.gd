extends SceneTree

const BoardScoring = preload("res://scripts/board/board_scoring.gd")

var scoring := BoardScoring.new()


func _init() -> void:
	assert_equal(scoring.get_ball_clear_score(1), 12, "heat 1 clear score per ball")
	assert_equal(scoring.get_ball_clear_score(3), 16, "heat 3 clear score per ball")
	assert_equal(scoring.get_ball_clear_score(5), 20, "heat 5 clear score per ball")

	assert_equal(scoring.get_chain_score_multiplier(1), 1, "chain 1 multiplier = 1x")
	assert_equal(scoring.get_chain_score_multiplier(2), 2, "chain 2 multiplier = 2x")
	assert_equal(scoring.get_chain_score_multiplier(3), 4, "chain 3 multiplier = 4x")
	assert_equal(scoring.get_chain_score_multiplier(4), 8, "chain 4 multiplier = 8x")
	assert_equal(scoring.get_chain_score_multiplier(6), 32, "chain 6 multiplier = 32x")

	assert_equal(scoring.get_group_clear_score(5, 3, 1), 60, "heat 5 x3 chain 1 score")
	assert_equal(scoring.get_group_clear_score(3, 2, 2), 64, "heat 3 x2 chain 2 score")
	assert_equal(
		scoring.get_group_clear_score(5, 3, 1) + scoring.get_group_clear_score(3, 2, 2),
		124,
		"total per-group score"
	)

	assert_equal(scoring.get_survival_move_score(), 2, "survival move score")

	print("Scoring smoke tests passed.")
	quit(0)


func assert_equal(actual: Variant, expected: Variant, message: String) -> void:
	if actual == expected:
		return

	push_error(message + ": expected " + str(expected) + ", got " + str(actual))
	quit(1)
