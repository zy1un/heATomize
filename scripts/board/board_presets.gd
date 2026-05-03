class_name BoardPresets
extends RefCounted


func get_initial_balls() -> Array[Dictionary]:
	return [
		make_ball(Vector2i(2, 2), 1),
		make_ball(Vector2i(4, 4), 3),
		make_ball(Vector2i(6, 5), 5),
	]


func get_debug_balls(board_name: String) -> Array[Dictionary]:
	match board_name:
		"cascade":
			return get_cascade_balls()
		"blocked":
			return get_blocked_balls()
		"chain":
			return get_chain_balls()
		_:
			return get_initial_balls()


func get_cascade_balls() -> Array[Dictionary]:
	return [
		make_ball(Vector2i(1, 1), 5),
		make_ball(Vector2i(2, 1), 5),
		make_ball(Vector2i(3, 1), 5),
		make_ball(Vector2i(2, 2), 4),
		make_ball(Vector2i(2, 3), 4),
		make_ball(Vector2i(1, 3), 4),
		make_ball(Vector2i(3, 3), 4),
		make_ball(Vector2i(7, 7), 2),
	]


func get_blocked_balls() -> Array[Dictionary]:
	var balls: Array[Dictionary] = []
	for x in range(9):
		balls.append(make_ball(Vector2i(x, 4), 2))
	balls.append(make_ball(Vector2i(1, 1), 3))
	balls.append(make_ball(Vector2i(7, 7), 1))
	return balls


func get_chain_balls() -> Array[Dictionary]:
	var balls: Array[Dictionary] = []
	for x in range(2, 7):
		balls.append(make_ball(Vector2i(x, 0), 5))
	for x in range(2, 7):
		balls.append(make_ball(Vector2i(x, 1), 3))
	for x in range(2, 7):
		balls.append(make_ball(Vector2i(x, 2), 1))
	for x in range(1, 8):
		balls.append(make_ball(Vector2i(x, 3), 1))
	balls.append(make_ball(Vector2i(0, 8), 5))
	balls.append(make_ball(Vector2i(8, 8), 4))
	return balls


func make_ball(cell: Vector2i, heat: int) -> Dictionary:
	return {
		"cell": cell,
		"heat": heat,
	}
