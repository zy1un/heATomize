class_name BoardScoring
extends RefCounted

const SURVIVAL_MOVE_SCORE := 2
const CLEAR_BASE_SCORE := 10
const CLEAR_HEAT_SCORE := 2


func get_survival_move_score() -> int:
	return SURVIVAL_MOVE_SCORE


func get_ball_clear_score(heat: int) -> int:
	return CLEAR_BASE_SCORE + CLEAR_HEAT_SCORE * heat


func get_chain_score_multiplier(chain_depth: int) -> int:
	var multiplier := 1
	for _index in range(maxi(0, chain_depth - 1)):
		multiplier *= 2
	return multiplier


func get_group_clear_score(group_heat: int, cell_count: int, chain_depth: int) -> int:
	return cell_count * get_ball_clear_score(group_heat) * get_chain_score_multiplier(chain_depth)
