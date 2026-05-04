extends SceneTree

const MAIN_SCENE := preload("res://scenes/Main.tscn")
const GameFeel = preload("res://scripts/core/game_feel.gd")
const ShockwaveEffect = preload("res://scripts/effects/shockwave_effect.gd")


func _init() -> void:
	await process_frame

	var main := MAIN_SCENE.instantiate()
	root.add_child(main)
	await process_frame

	var board = main.get_node("Board")
	board.load_debug_board("cascade")
	await process_frame
	await board.resolve_system_turn()
	await process_frame

	var ball_node = board.ball_nodes.get(Vector2i(7, 7))
	if ball_node == null:
		push_error("Expected debug ball at (7, 7).")
		quit(1)
		return

	ball_node.play_spawn_feedback()
	await create_timer(0.28).timeout
	ball_node.play_heat_feedback()
	await create_timer(0.2).timeout
	ball_node.play_aftershock_feedback()
	await process_frame
	assert_true(count_shockwaves(board) > 0, "aftershock feedback spawns a shockwave effect")
	await create_timer(0.24).timeout
	GameFeel.set_effect_scales(0.0, 0.0, 0.0)
	ball_node.play_heat_feedback()
	await create_timer(0.05).timeout
	GameFeel.reset_effect_scales()

	print("Animation smoke tests passed.")
	quit(0)


func count_shockwaves(root: Node) -> int:
	var count := 0
	for child in root.find_children("*", "ShockwaveEffect", true, false):
		if child is ShockwaveEffect:
			count += 1
	return count


func assert_true(value: bool, message: String) -> void:
	if value:
		return
	push_error(message)
	quit(1)
