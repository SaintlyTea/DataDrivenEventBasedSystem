class_name EventOpBuilder
extends Resource

static func make_damage_gather_attack_mods(
	event: EffectBase,
	source: Unit,
	targets: Array[Unit],
	base_stat_key: String = "base_phys_dmg",
	damage_type: String = "phys",
	tags: Array[String] = [],
) -> DamageGatherAttackModsOp:
	var op := DamageGatherAttackModsOp.new()
	op.event = event
	op.src = source
	op.tgts = targets.duplicate()
	op.base_dmg_key = base_stat_key
	op.dtype = damage_type
	op.tags = tags.duplicate()
	return op

static func make_damage_finalize(
	event: EffectBase,
	targets: Array[Unit],
	mods: DamageCalcData,
	_base_dmg_key: String,
	_damage_type: String
) -> DamageFinalizeOp:
	var op := DamageFinalizeOp.new()
	op.event = event
	op.src = event.host
	op.tgts = targets
	op.base_dmg_key = _base_dmg_key
	op.dtype = _damage_type
	op.tags = event.tags
	op.mods = mods
	return op

static func make_damage_gather_defense_mods(
	event: EffectBase,
	target: Unit,
	amount: int,
	damage_type: String,
) -> DamageGatherDefenseModsOp:
	var op := DamageGatherDefenseModsOp.new()
	op.event = event
	op.src = event.host
	op.tgts = [target]
	op.amount = amount
	op.dtype = damage_type
	op.tags = event.tags
	return op

static func make_damage_apply(
	event: EffectBase,
	source: Unit,
	target: Unit,
	amount: int,
	damage_type: String,
	defense_mods: DamageTakenData,
	tags: Array[String] = []
) -> DamageApplyOp:
	var op := DamageApplyOp.new()
	op.event = event
	op.src = source
	op.tgts = [target]
	op.amount = amount
	op.dtype = damage_type
	op.def_mods = _clone_damage_taken(defense_mods)
	op.tags = tags.duplicate()
	return op

static func make_damage_direct(
	event: EffectBase,
	source: Unit,
	targets: Array[Unit],
	amount: int,
	damage_type: String = "phys",
	tags: Array[String] = []
) -> DamageDirectOp:
	var op := DamageDirectOp.new()
	op.event = event
	op.src = source
	op.tgts = targets.duplicate()
	op.amount = amount
	op.dtype = damage_type
	op.tags = tags.duplicate()
	return op


# ---------- Non-damage ----------

static func make_mod_stat(
	event: EffectBase,
	target: Unit,
	stat: String,
	mod_type: String,
	delta: float,
	mode: String = "add"
) -> ModStatOp:
	var op := ModStatOp.new()
	op.event = event
	op.target = target
	op.stat = stat
	op.mod_type = mod_type
	op.delta = delta
	op.mode = mode
	return op

static func make_apply_status(
	event: EffectBase,
	target: Unit,
	status_id: String,
	stacks: int,
	duration: int
) -> ApplyStatusOp:
	var op := ApplyStatusOp.new()
	op.event = event
	op.target = target
	op.status_id = status_id
	op.stacks = stacks
	op.duration = duration
	return op


# ---------- Private cloning helpers (avoid shared mutable payloads) ----------

# TODO: Move these to the classes individually
static func _clone_damage_calc(src: DamageCalcData) -> DamageCalcData:
	if src == null:
		return null
	var out := DamageCalcData.new()
	out.base = src.base
	out.flat = src.flat
	out.percent = src.percent
	out.dtype = src.dtype
	out.tags = src.tags.duplicate()
	return out

static func _clone_damage_taken(src: DamageTakenData) -> DamageTakenData:
	if src == null:
		return null
	var out := DamageTakenData.new()
	out.base_def = src.base_def
	out.flat = src.flat
	out.percent = src.percent
	out.dtype = src.dtype
	out.tags = src.tags.duplicate()
	return out
