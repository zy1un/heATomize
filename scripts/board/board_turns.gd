class_name BoardTurns
extends RefCounted

var rng: RandomNumberGenerator = RandomNumberGenerator.new()


func _init() -> void:
	rng.randomize()


func generate_spawn_plan(
	board_state: Array,
	grid_size: int,
	spawn_limit: int,
	min_heat: int,
	max_heat: int
) -> Array[Dictionary]:
	var heat_preview: Array[int] = generate_heat_preview(spawn_limit, min_heat, max_heat)
	return generate_spawn_plan_with_heats(board_state, grid_size, heat_preview)


func generate_heat_preview(count: int, min_heat: int, max_heat: int) -> Array[int]:
	var preview: Array[int] = []
	for _index in range(count):
		preview.append(rng.randi_range(min_heat, max_heat))
	return preview


func generate_spawn_plan_with_heats(
	board_state: Array,
	grid_size: int,
	heat_preview: Array[int]
) -> Array[Dictionary]:
	var empty_cells: Array[Vector2i] = get_empty_cells(board_state, grid_size)
	if empty_cells.is_empty() or heat_preview.is_empty():
		return []

	var spawn_count: int = mini(heat_preview.size(), empty_cells.size())
	var plan: Array[Dictionary] = []
	for index in range(spawn_count):
		var random_empty_index: int = rng.randi_range(0, empty_cells.size() - 1)
		var cell: Vector2i = empty_cells[random_empty_index]
		var heat: int = heat_preview[index]
		plan.append({
			"cell": cell,
			"heat": heat,
		})
		empty_cells.remove_at(random_empty_index)

	return plan


func get_empty_cells(board_state: Array, grid_size: int) -> Array[Vector2i]:
	var empty_cells: Array[Vector2i] = []
	for y in range(grid_size):
		for x in range(grid_size):
			if board_state[y][x] == null:
				empty_cells.append(Vector2i(x, y))
	return empty_cells
