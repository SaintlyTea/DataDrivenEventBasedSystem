# Scripts/Models/Unit.gd
class_name Unit
extends Resource

var id: String
var stats: StatBlock = StatBlock.new()
var effects: Array[EffectBase] = []

func change_base_stat(stat: String, new_amount: int) -> void:
	stats.set(stat, new_amount)

func get_base_stat(stat: String) -> int:
	return stats.get(stat)

func take_damage(amount: int) -> bool:
	var dmg:int = max(0, amount)

	if dmg <= 0:
		return false

	stats.set_hp(max(0, stats.get_hp() - max(0, amount)))
	return true
