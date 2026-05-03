extends SceneTree

const BoardPresets = preload("res://scripts/board/board_presets.gd")

var presets := BoardPresets.new()


func _init() -> void:
	assert_equal(presets.get_initial_balls().size(), 3, "initial preset ball count")
	assert_has_ball(presets.get_initial_balls(), Vector2i(2, 2), 1, "initial heat 1 ball")
	assert_has_ball(presets.get_initial_balls(), Vector2i(4, 4), 3, "initial heat 3 ball")
	assert_has_ball(presets.get_initial_balls(), Vector2i(6, 5), 5, "initial heat 5 ball")

	assert_equal(presets.get_debug_balls("cascade").size(), 8, "cascade preset ball count")
	assert_has_ball(presets.get_debug_balls("cascade"), Vector2i(7, 7), 2, "cascade anchor ball")

	assert_equal(presets.get_debug_balls("blocked").size(), 11, "blocked preset ball count")
	assert_has_ball(presets.get_debug_balls("blocked"), Vector2i(0, 4), 2, "blocked wall start")
	assert_has_ball(presets.get_debug_balls("blocked"), Vector2i(8, 4), 2, "blocked wall end")

	assert_equal(presets.get_debug_balls("chain").size(), 24, "chain preset ball count")
	assert_has_ball(presets.get_debug_balls("chain"), Vector2i(0, 8), 5, "chain movable corner")
	assert_has_ball(presets.get_debug_balls("chain"), Vector2i(8, 8), 4, "chain movable corner")

	print("BoardPresets smoke tests passed.")
	quit(0)


func assert_has_ball(entries: Array[Dictionary], cell: Vector2i, heat: int, message: String) -> void:
	for entry in entries:
		if entry["cell"] == cell and int(entry["heat"]) == heat:
			return

	push_error(message + ": missing " + str(cell) + " heat " + str(heat))
	quit(1)


func assert_equal(actual: Variant, expected: Variant, message: String) -> void:
	if actual == expected:
		return

	push_error(message + ": expected " + str(expected) + ", got " + str(actual))
	quit(1)
