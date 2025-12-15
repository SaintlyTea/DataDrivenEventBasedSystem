class_name EventBuilder
extends Resource

# Attacker calc
static func build_damage_calc_ctx_obj(op: DamageGatherAttackModsOp) -> EventContext:
	var ctx := EventContext.new()
	ctx.event_type = "ModifyMyDamage"
	ctx.trigger_source = op.src
	ctx.trigger_targets = op.tgts

	var data := DamageCalcData.new()
	data.base = float(op.src.get_base_stat(op.base_dmg_key))
	data.flat = 0.0
	data.percent = 0.0
	data.dtype = op.dtype
	data.tags = op.tags.duplicate()
	ctx.set_damage_calc(data)

	return ctx

# Defender calc (single target)
static func build_damage_taken_calc_ctx_obj(op: DamageGatherDefenseModsOp, _parent_ctx: EventContext) -> EventContext:
	var ctx := EventContext.new()
	var tgt: Unit = (op.tgts.size() > 0) if op.tgts[0] else null
	ctx.event_type = "ModifyMyDamageTaken"
	ctx.trigger_source = op.src
	ctx.trigger_targets = op.tgts

	var data := DamageTakenData.new()
	data.base_def = float(tgt.get_defense(op.dtype))
	data.flat = 0.0
	data.percent = 0.0
	data.dtype = op.dtype
	data.tags = op.tags.duplicate()
	ctx.set_damage_taken(data)

	return ctx
