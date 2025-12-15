# Scripts/Models/Stats/StatStore.gd
class_name StatBlock
extends Resource

@export var hp: int = 1
@export var max_hp: int = 1
@export var phys_dmg_mod: int = 0

var _dyn: Dictionary = {}  # keys: StringName, values: int

# --- Core convenience ---
func get_hp() -> int: return hp
func set_hp(v: int) -> void: hp = max(0, v)

func get_max_hp() -> int: return max_hp
func set_max_hp(v: int) -> void: max_hp = max(0, v)

func get_phys_dmg_mod() -> int: return phys_dmg_mod
func set_phys_dmg_mod(v: int) -> void: phys_dmg_mod = v

func has(stat: StringName) -> bool:
	match stat:
		&"hp", &"max_hp", &"phys_dmg_mod":
			return true
		_:
			return _dyn.has(stat)

func stat_get(stat: StringName) -> int:
	match stat:
		&"hp": return hp
		&"max_hp": return max_hp
		&"phys_dmg_mod": return phys_dmg_mod
		_:
			return int(_dyn.get(stat, 0))

func stat_set(stat: StringName, value: int) -> void:
	match stat:
		&"hp": hp = max(0, value)
		&"max_hp": max_hp = max(0, value)
		&"phys_dmg_mod": phys_dmg_mod = value
		_:
			_dyn[stat] = value

func add(stat: StringName, delta: int) -> void:
	set(stat, get(stat) + delta)

func remove_dynamic(stat: StringName) -> void:
	match stat:
		&"hp", &"max_hp", &"base_phys_dmg":
			push_warning("StatStore: refusing to remove core stat '%s'." % stat)
		_:
			_dyn.erase(stat)

func clamp_hp_to_max() -> void:
	if hp > max_hp:
		hp = max_hp

# Utility
func to_dict() -> Dictionary:
	var out: Dictionary = {
		"hp": hp,
		"max_hp": max_hp,
		"phys_dmg_mod": phys_dmg_mod,
	}
	for k in _dyn.keys():
		out[String(k)] = _dyn[k]
	return out.duplicate(true)

func clone() -> StatBlock:
	var c := StatBlock.new()
	c.hp = hp
	c.max_hp = max_hp
	c.phys_dmg_mod = phys_dmg_mod
	c._dyn = _dyn.duplicate(true)
	return c
