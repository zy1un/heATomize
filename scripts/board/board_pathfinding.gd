class_name BoardPathfinding
extends RefCounted

const ORTHOGONAL_DIRECTIONS: Array[Vector2i] = [
	Vector2i.LEFT,
	Vector2i.RIGHT,
	Vector2i.UP,
	Vector2i.DOWN,
]


func can_reach_cell(board_state: Array, grid_size: int, from_cell: Vector2i, to_cell: Vector2i) -> bool:
	return not find_path(board_state, grid_size, from_cell, to_cell).is_empty()


func find_path(board_state: Array, grid_size: int, from_cell: Vector2i, to_cell: Vector2i) -> Array[Vector2i]:
	if not is_in_grid(grid_size, from_cell):
		return []
	if not is_in_grid(grid_size, to_cell):
		return []
	if from_cell == to_cell:
		return [from_cell]
	if get_ball(board_state, grid_size, to_cell) != null:
		return []

	var frontier: Array[Vector2i] = [from_cell]
	var visited: Dictionary = {from_cell: true}
	var came_from: Dictionary = {}
	var queue_index: int = 0

	while queue_index < frontier.size():
		var current: Vector2i = frontier[queue_index]
		queue_index += 1

		if current == to_cell:
			return build_path(came_from, to_cell)

		for direction in ORTHOGONAL_DIRECTIONS:
			var next_cell: Vector2i = current + direction
			if not is_walkable_cell(board_state, grid_size, next_cell, to_cell):
				continue
			if visited.has(next_cell):
				continue

			visited[next_cell] = true
			came_from[next_cell] = current
			frontier.append(next_cell)

	return []


func has_any_reachable_move(board_state: Array, grid_size: int) -> bool:
	var empty_cells: Array[Vector2i] = get_empty_cells(board_state, grid_size)
	if empty_cells.is_empty():
		return false

	for y in range(grid_size):
		for x in range(grid_size):
			var from_cell: Vector2i = Vector2i(x, y)
			if get_ball(board_state, grid_size, from_cell) == null:
				continue

			for empty_cell in empty_cells:
				if can_reach_cell(board_state, grid_size, from_cell, empty_cell):
					return true

	return false


func get_empty_cells(board_state: Array, grid_size: int) -> Array[Vector2i]:
	var empty_cells: Array[Vector2i] = []
	for y in range(grid_size):
		for x in range(grid_size):
			var cell: Vector2i = Vector2i(x, y)
			if get_ball(board_state, grid_size, cell) == null:
				empty_cells.append(cell)
	return empty_cells


func is_in_grid(grid_size: int, grid_pos: Vector2i) -> bool:
	return (
		grid_pos.x >= 0
		and grid_pos.y >= 0
		and grid_pos.x < grid_size
		and grid_pos.y < grid_size
	)


func get_ball(board_state: Array, grid_size: int, grid_pos: Vector2i):
	if not is_in_grid(grid_size, grid_pos):
		return null
	return board_state[grid_pos.y][grid_pos.x]


func build_path(came_from: Dictionary, target_cell: Vector2i) -> Array[Vector2i]:
	var reversed_path: Array[Vector2i] = []
	var current: Vector2i = target_cell

	reversed_path.append(current)
	while came_from.has(current):
		current = came_from[current] as Vector2i
		reversed_path.append(current)

	reversed_path.reverse()
	return reversed_path


func is_walkable_cell(board_state: Array, grid_size: int, cell: Vector2i, target_cell: Vector2i) -> bool:
	if not is_in_grid(grid_size, cell):
		return false
	if cell == target_cell:
		return true
	return get_ball(board_state, grid_size, cell) == null
