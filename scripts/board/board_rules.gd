class_name BoardRules
extends RefCounted

const MIN_HEAT := 1
const MAX_HEAT := 5
const ELIMINATION_THRESHOLDS := {
	1: 7,
	2: 6,
	3: 5,
	4: 4,
	5: 3,
}
const ORTHOGONAL_DIRECTIONS: Array[Vector2i] = [
	Vector2i.LEFT,
	Vector2i.RIGHT,
	Vector2i.UP,
	Vector2i.DOWN,
]


func compute_heat_updates(board_state: Array, grid_size: int) -> Array[Dictionary]:
	var updates: Array[Dictionary] = []

	for y in range(grid_size):
		for x in range(grid_size):
			var cell: Vector2i = Vector2i(x, y)
			var ball: Variant = get_ball(board_state, grid_size, cell)
			if ball == null:
				continue

			var old_heat: int = int(ball["heat"])
			var new_heat: int = compute_next_heat(board_state, grid_size, cell, old_heat)
			if new_heat == old_heat:
				continue

			updates.append({
				"cell": cell,
				"old_heat": old_heat,
				"new_heat": new_heat,
			})

	return updates


func find_elimination_groups(board_state: Array, grid_size: int) -> Array[Dictionary]:
	var groups: Array[Dictionary] = []
	var visited: Dictionary = {}

	for y in range(grid_size):
		for x in range(grid_size):
			var start_cell: Vector2i = Vector2i(x, y)
			if visited.has(start_cell):
				continue

			var ball: Variant = get_ball(board_state, grid_size, start_cell)
			if ball == null:
				continue

			var heat: int = int(ball["heat"])
			var connected_cells: Array[Vector2i] = collect_same_heat_group(
				board_state,
				grid_size,
				start_cell,
				heat,
				visited
			)
			var threshold: int = ELIMINATION_THRESHOLDS.get(heat, 999)
			if connected_cells.size() < threshold:
				continue

			groups.append({
				"heat": heat,
				"threshold": threshold,
				"cells": connected_cells,
			})

	return groups


func collect_same_heat_group(
	board_state: Array,
	grid_size: int,
	start_cell: Vector2i,
	heat: int,
	visited: Dictionary
) -> Array[Vector2i]:
	var group: Array[Vector2i] = []
	var frontier: Array[Vector2i] = [start_cell]
	var queue_index: int = 0
	visited[start_cell] = true

	while queue_index < frontier.size():
		var current: Vector2i = frontier[queue_index]
		queue_index += 1
		group.append(current)

		for direction in ORTHOGONAL_DIRECTIONS:
			var next_cell: Vector2i = current + direction
			if visited.has(next_cell):
				continue
			var next_ball: Variant = get_ball(board_state, grid_size, next_cell)
			if next_ball == null:
				continue
			if int(next_ball["heat"]) != heat:
				continue

			visited[next_cell] = true
			frontier.append(next_cell)

	return group


func get_cells_from_groups(groups: Array[Dictionary]) -> Array[Vector2i]:
	var cells: Array[Vector2i] = []
	for group in groups:
		var group_cells: Array = group["cells"]
		for cell in group_cells:
			cells.append(cell as Vector2i)
	return cells


func compute_aftershock_updates(
	board_state: Array,
	grid_size: int,
	removed_cells: Array[Vector2i]
) -> Array[Dictionary]:
	var affected_cells: Dictionary = {}
	for removed_cell in removed_cells:
		for direction in ORTHOGONAL_DIRECTIONS:
			var neighbor_cell: Vector2i = removed_cell + direction
			var neighbor_ball: Variant = get_ball(board_state, grid_size, neighbor_cell)
			if neighbor_ball == null:
				continue
			affected_cells[neighbor_cell] = true

	var updates: Array[Dictionary] = []
	for raw_cell in affected_cells.keys():
		var cell: Vector2i = raw_cell as Vector2i
		var ball: Variant = get_ball(board_state, grid_size, cell)
		if ball == null:
			continue

		var old_heat: int = int(ball["heat"])
		var new_heat: int = clampi(old_heat + 1, MIN_HEAT, MAX_HEAT)
		if new_heat == old_heat:
			continue

		updates.append({
			"cell": cell,
			"old_heat": old_heat,
			"new_heat": new_heat,
		})

	return updates


func compute_next_heat(board_state: Array, grid_size: int, cell: Vector2i, current_heat: int) -> int:
	var neighbor_heats: Array[int] = get_neighbor_heats(board_state, grid_size, cell)
	if neighbor_heats.is_empty():
		return clampi(current_heat - 1, MIN_HEAT, MAX_HEAT)

	var has_hotter_neighbor: bool = false
	var all_neighbors_colder: bool = true

	for neighbor_heat in neighbor_heats:
		if neighbor_heat > current_heat:
			has_hotter_neighbor = true
		if neighbor_heat >= current_heat:
			all_neighbors_colder = false

	if has_hotter_neighbor:
		return clampi(current_heat + 1, MIN_HEAT, MAX_HEAT)
	if all_neighbors_colder:
		return clampi(current_heat - 1, MIN_HEAT, MAX_HEAT)
	return current_heat


func get_neighbor_heats(board_state: Array, grid_size: int, cell: Vector2i) -> Array[int]:
	var neighbor_heats: Array[int] = []

	for direction in ORTHOGONAL_DIRECTIONS:
		var neighbor_cell: Vector2i = cell + direction
		var neighbor_ball: Variant = get_ball(board_state, grid_size, neighbor_cell)
		if neighbor_ball == null:
			continue
		neighbor_heats.append(int(neighbor_ball["heat"]))

	return neighbor_heats


func get_ball(board_state: Array, grid_size: int, grid_pos: Vector2i):
	if not is_in_grid(grid_size, grid_pos):
		return null
	return board_state[grid_pos.y][grid_pos.x]


func is_in_grid(grid_size: int, grid_pos: Vector2i) -> bool:
	return (
		grid_pos.x >= 0
		and grid_pos.y >= 0
		and grid_pos.x < grid_size
		and grid_pos.y < grid_size
	)
