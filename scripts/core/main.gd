extends Node2D

@onready var board: Node = $Board
@onready var hud: CanvasLayer = $HUD
@onready var score_feedback: CanvasLayer = $ScoreFeedback


func _ready() -> void:
	print("heATomzie booted.")
	board.status_changed.connect(hud.update_status)
	hud.restart_requested.connect(board.restart_game)
	hud.debug_board_requested.connect(board.load_debug_board)
	hud.show_heat_labels_changed.connect(board.set_show_heat_labels)
	hud.move_preview_changed.connect(board.set_move_preview_enabled)
	hud.chaos_mode_changed.connect(board.set_chaos_mode_enabled)
	score_feedback.setup(board, hud)
	board.emit_status("Ready")
