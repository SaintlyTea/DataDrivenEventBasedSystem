class_name DealDamage
extends EffectBase

var base_key := "base_phys_dmg"  # read from attacker
var dtype := "phys"            # or "phys", etc.

func _enqueue(ctx: EventContext, out: EventOpQueue) -> void:
	# TODO: create operation
	out.dmg(ctx, self)

func _execute(ctx: EventContext, out: EventOpQueue, _op: EventOp) -> void:
	var op = _op as DamageBaseOp
	# Gather damage mods
	var ev_dmg_ctx = EventBuilder.build_damage_calc_ctx_obj(op)
	var temp_queue = EventOpQueue.new()
	GameMaster.emit_trigger(ev_dmg_ctx, temp_queue)
	
	# Finalise damage
	var final_dmg_payload = ev_dmg_ctx.get_damage_calc()
	var finalise_dmg_event = EventOpBuilder.make_damage_finalize(self, ctx.trigger_targets, final_dmg_payload, base_key, dtype)
	temp_queue.push_normal_op(finalise_dmg_event)
	
	# Gather defense
	for unit in ctx.trigger_targets:
		var def_gather_op = EventOpBuilder.make_damage_gather_defense_mods(self, unit, final_dmg_payload.final, dtype)
		var ev_def_ctx = EventBuilder.build_damage_taken_calc_ctx_obj(def_gather_op, ctx)
		GameMaster.emit_trigger(ctx, temp_queue)
		
		# deal actual damage TODO: Split into two, one calc and one apply
		var def_gather_payload = ev_def_ctx.get_damage_taken()
		var deal_final_damage = EventOpBuilder.make_damage_apply(self, source, unit, final_dmg_payload.final, dtype, def_gather_payload)
		temp_queue.push_normal_op(deal_final_damage)
	
	# Flip and add to real queue
	out.enqueue_barrier_ops(temp_queue.fifo.reverse())
