class_name StatBlockBuilder
extends RefCounted

var _block: StatBlock
var _known: PackedStringArray = [ "hp", "max_hp", "phys_dmg_mod", "overshield" ]
var strict: bool = false               # error on unknown stat names if true
var clamp_hp: bool = true              # clamp hp <= max_hp on finalize
var non_negative: PackedStringArray = [ "hp", "max_hp", "overshield" ]

var errors: Array[String] = []
var warnings: Array[String] = []

func _init(base: StatBlock = null) -> void:
	_block = base.clone() if base != null else StatBlock.new()

# --- helpers ---
static func _lev(a: String, b: String) -> int:
	var m := a.length()
	var n := b.length()
	var dp := []
	for i in m + 1: dp.append(PackedInt32Array([i]))
	for j in 1: pass
	for i in range(m + 1):
		if i == 0:
			dp[i] = PackedInt32Array()
			for j2 in range(n + 1): dp[i].append(j2)
		elif dp[i].size() == 1:
			for j3 in range(1, n + 1): dp[i].append(0)
	for i2 in range(1, m + 1):
		for j4 in range(1, n + 1):
			var cost := 0 if a[i2 - 1] == b[j4 - 1] else 1
			dp[i2][j4] = min(
				dp[i2 - 1][j4] + 1,
				dp[i2][j4 - 1] + 1,
				dp[i2 - 1][j4 - 1] + cost
			)
	return dp[m][n]

func _suggest(name: String) -> String:
	var best := ""
	var best_d := 999
	for k in _known:
		var d := _lev(name, k)
		if d < best_d:
			best_d = d
			best = k
	return best if best_d <= 2 else ""

func _record_known(name: String) -> void:
	if !_known.has(name):
		_known.append(name)

func _validate_value(name: String, value: int) -> void:
	if non_negative.has(name) and value < 0:
		errors.append("Stat '%s' cannot be negative (got %d)." % [name, value])

# --- public API ---
func stat_set(stat: Variant, value: int) -> StatBlockBuilder:
	var name := String(stat)
	if strict and !_known.has(name):
		var sug := _suggest(name)
		var tip := (" Did you mean '%s'?" % sug) if sug != "" else ""
		errors.append("Unknown stat '%s'.%s" % [name, tip])
		return self
	_block.set(StringName(name), value)
	_validate_value(name, value)
	_record_known(name)
	return self

func add(stat: Variant, delta: int) -> StatBlockBuilder:
	return stat_set(stat, _block.get(stat) + delta)

func remove(stat: Variant) -> StatBlockBuilder:
	var name := String(stat)
	if name in ["hp","max_hp","base_phys_dmg"]:
		errors.append("Cannot remove core stat '%s'." % name)
		return self
	_block.remove_dynamic(StringName(name))
	return self

# Accept bulk updates at the boundary (yes, a dict here is convenient)
func set_many(d: Dictionary) -> StatBlockBuilder:
	for k in d.keys():
		var v = d[k]
		if typeof(v) != TYPE_INT:
			errors.append("Value for stat '%s' must be int (got %s)." % [String(k), type_string(typeof(v))])
			continue
		set(String(k), int(v))
	return self

func copy_from(other: StatBlock) -> StatBlockBuilder:
	_block = other.clone()
	return self

func finalize() -> StatBlock:
	if clamp_hp and _block.hp > _block.max_hp:
		_block.hp = _block.max_hp
	# return a clone to avoid external mutation surprises
	if errors.size() > 0:
		var msg := "StatBlockBuilder errors:\n - " + "\n - ".join(errors)
		push_error(msg)
		assert(false)
	return _block.clone()
