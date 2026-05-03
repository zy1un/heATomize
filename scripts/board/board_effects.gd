class_name BoardEffects
extends RefCounted

const MOVE_ANIMATION_SECONDS := 0.16
const MOVE_ANIMATION_MIN_STEP_SECONDS := 0.045
const MOVE_ANIMATION_MAX_TOTAL_SECONDS := 0.48
const HEAT_TRANSFER_SECONDS := 0.48
const AFTERSHOCK_TRANSFER_SECONDS := 0.54
const TRANSFER_STAGGER_SECONDS := 0.035


func animate_ball_along_path(
	owner: Node,
	ball_node: Node2D,
	move_path: Array[Vector2i],
	grid_to_pixel_center: Callable
) -> void:
	if move_path.size() <= 1:
		ball_node.position = grid_to_pixel_center.call(move_path.back())
		return

	var tween := owner.create_tween()
	var step_duration := get_move_step_duration(move_path)
	for index in range(1, move_path.size()):
		tween.tween_property(
			ball_node,
			"position",
			grid_to_pixel_center.call(move_path[index]),
			step_duration
		).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	await tween.finished


func get_move_step_duration(move_path: Array[Vector2i]) -> float:
	var step_count: int = maxi(1, move_path.size() - 1)
	var capped_duration: float = MOVE_ANIMATION_MAX_TOTAL_SECONDS / float(step_count)
	return maxf(MOVE_ANIMATION_MIN_STEP_SECONDS, minf(MOVE_ANIMATION_SECONDS, capped_duration))


func play_elimination_feedback(cells: Array[Vector2i], ball_nodes: Dictionary) -> void:
	for cell in cells:
		var ball_node = ball_nodes.get(cell)
		if ball_node != null:
			ball_node.play_elimination_feedback()


func animate_heat_transfers(
	owner: Node,
	effects_layer: Node,
	heat_updates: Array[Dictionary],
	source_cells_by_target: Dictionary,
	grid_to_pixel_center: Callable
) -> void:
	var effect_count := 0
	for update in heat_updates:
		var old_heat: int = update["old_heat"]
		var new_heat: int = update["new_heat"]
		if new_heat <= old_heat:
			continue

		var target_cell: Vector2i = update["cell"]
		var source_cells: Array = source_cells_by_target.get(target_cell, [])
		for source_cell in source_cells:
			var delay := effect_count * TRANSFER_STAGGER_SECONDS
			spawn_transfer_pixel(
				owner,
				effects_layer,
				grid_to_pixel_center.call(source_cell),
				grid_to_pixel_center.call(target_cell),
				Color8(255, 208, 82),
				HEAT_TRANSFER_SECONDS,
				delay,
				8.0
			)
			effect_count += 1

	if effect_count > 0:
		var total_seconds := HEAT_TRANSFER_SECONDS + (effect_count - 1) * TRANSFER_STAGGER_SECONDS
		await owner.get_tree().create_timer(total_seconds).timeout


func animate_aftershock_transfers(
	owner: Node,
	effects_layer: Node,
	eliminated_cells: Array[Vector2i],
	heat_updates: Array[Dictionary],
	grid_to_pixel_center: Callable
) -> void:
	var effect_count := 0
	for update in heat_updates:
		var target_cell: Vector2i = update["cell"]
		for source_cell in get_adjacent_cells_from(target_cell, eliminated_cells):
			var delay := effect_count * TRANSFER_STAGGER_SECONDS
			spawn_transfer_pixel(
				owner,
				effects_layer,
				grid_to_pixel_center.call(source_cell),
				grid_to_pixel_center.call(target_cell),
				Color8(255, 86, 42),
				AFTERSHOCK_TRANSFER_SECONDS,
				delay,
				10.0
			)
			effect_count += 1

	if effect_count > 0:
		var total_seconds := AFTERSHOCK_TRANSFER_SECONDS + (effect_count - 1) * TRANSFER_STAGGER_SECONDS
		await owner.get_tree().create_timer(total_seconds).timeout


func spawn_transfer_pixel(
	owner: Node,
	effects_layer: Node,
	start_pos: Vector2,
	end_pos: Vector2,
	color: Color,
	duration: float,
	delay: float,
	size: float
) -> void:
	var pixel := ColorRect.new()
	pixel.color = color
	pixel.size = Vector2(size, size)
	pixel.pivot_offset = pixel.size * 0.5
	pixel.position = start_pos - pixel.pivot_offset
	pixel.modulate.a = 0.0
	effects_layer.add_child(pixel)

	var tween := owner.create_tween()
	tween.tween_interval(delay)
	tween.set_parallel(true)
	tween.tween_property(pixel, "position", end_pos - pixel.pivot_offset, duration).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.tween_property(pixel, "modulate:a", 1.0, duration * 0.25).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.tween_property(pixel, "scale", Vector2.ONE * 1.35, duration * 0.5).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	tween.chain().tween_property(pixel, "modulate:a", 0.0, duration * 0.28).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
	tween.finished.connect(pixel.queue_free)


func get_adjacent_cells_from(cell: Vector2i, candidates: Array[Vector2i]) -> Array[Vector2i]:
	var source_cells: Array[Vector2i] = []
	for candidate in candidates:
		if abs(candidate.x - cell.x) + abs(candidate.y - cell.y) == 1:
			source_cells.append(candidate)
	return source_cells
