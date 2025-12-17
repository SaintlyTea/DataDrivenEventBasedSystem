# Deprecated

#class_name EventHandlers
#extends Resource
#
## ---------------- Controller ----------------
#
#static func _run_damage_flow(operation: EventOp, parent_context: EventContext, event_queue: EventOpQueue) -> void:
	## Fast path (direct): apply and enqueue OnDamaged
	#if operation is DamageDirectOp:
		#var direct_op: DamageDirectOp = operation
		#var after_queue := _apply_damage_only(direct_op, parent_context)   # helper returns follow-ups
		#event_queue.enqueue_normal_ops(after_queue)
		#return
#
	## Phase 1: Attacker-side modifiers
	#if operation is DamageGatherAttackModsOp:
		#var gather_op: DamageGatherAttackModsOp = operation
		#var calc_payload: DamageCalcData = _gather_dmg_mods(gather_op, parent_context, event_queue)  
#
		## Orchestrate next phase: finalize (barrier). Build via EventOpBuilder with typed payload.
		#var finalize_op := EventOpBuilder.make_damage_finalize(
			#gather_op.src,
			#gather_op.tgts,
			#gather_op.base_dmg_key,
			#gather_op.dtype,
			#gather_op.tags,
			#calc_payload
		#)
		#event_queue.push_barrier_op(finalize_op)
		#return
#
	## Phase 2: Finalize damage number per hit (no enqueues inside helper)
	#if operation is DamageFinalizeOp:
		#var finalize_op: DamageFinalizeOp = operation
		#var final_amount := _finalize_damage(finalize_op, parent_context)  # pure calc from typed payload
#
		## Orchestrate next phase per target: defender modifiers (barrier).
		#for target_unit in parent_context.trigger_targets:
			#var defense_mod_op = EventOpBuilder.make_damage_gather_defense_mods(
				#finalize_op.src,
				#target_unit,
				#final_amount,
				#finalize_op.dtype,
				#finalize_op.tags
			#)
			#event_queue.push_barrier_op(defense_mod_op)
		#return
#
	## Phase 3: Defender-side modifiers
	#if operation is DamageGatherDefenseModsOp:
		#var defense_op: DamageGatherDefenseModsOp = operation
		#var taken_payload: DamageTakenData = _gather_def_mods(defense_op, parent_context, event_queue)  # helper enqueues ONLY its own follow-ups (barrier)
#
		## Orchestrate final phase: apply HP (barrier). Queue helper with typed payload.
		#var apply_damage_op = EventOpBuilder.make_damage_apply(
			#defense_op.src,
			#defense_op.tgts[0],  # single target phase
			#defense_op.amount,
			#defense_op.dtype,
			#taken_payload,
			#defense_op.tags
		#)
		#event_queue.push_barrier_op(apply_damage_op)
		#return
#
	## Phase 4: Apply HP
	#if operation is DamageApplyOp:
		#var apply_op_typed: DamageApplyOp = operation
		#var after_queue := _apply_damage_with_def(apply_op_typed, parent_context)  # helper returns follow-ups (OnDamaged)
		#event_queue.enqueue_normal_ops(after_queue)
		#return
#
	#push_warning("Damage controller: unknown op type %s" % operation.get_class())
#
#
## ---------------- Helpers (standalone-safe) ----------------
## Helpers enqueue ONLY their own trigger follow-ups for their step (barrier where relevant),
## do NOT advance the phase, and return typed payloads (or queues) to the controller.
#
## 1) Attacker-side: gather modifiers → returns DamageCalcData
#static func _gather_dmg_mods(operation: DamageGatherAttackModsOp, parent_context: EventContext, event_queue: EventOpQueue) -> DamageCalcData:
	#var calc_context := EventBuilder.build_damage_calc_ctx_obj(operation, parent_context)
	#var follow_queue := EventOpQueue.new()
	#GameMaster.emit_trigger(calc_context, follow_queue)
	## enqueue ONLY the calc follow-ups as barrier (they affect this step)
	#event_queue.enqueue_barrier_ops(follow_queue)
#
	#var payload: DamageCalcData = calc_context.get_damage_calc()
	#return payload
#
## 2) Attacker-side: compute final amount from DamageCalcData (pure)
#static func _finalize_damage(operation: DamageFinalizeOp, parent_context: EventContext) -> int:
	#var mods: DamageCalcData = operation.mods  
	## Fallback: if base was not filled for some reason, derive from stat
	#var base_value := (mods.base != 0.0) if mods.base else float(parent_context.trigger_source.get_base_stat(operation.base_dmg_key))
	#var flat_bonus := float(mods.flat)
	#var percent_bonus := float(mods.percent)
	#return int(round(max(0.0, (base_value + flat_bonus) * (1.0 + percent_bonus))))
#
## 3) Defender-side: gather modifiers for ONE target → returns DamageTakenData
#static func _gather_def_mods(operation: DamageGatherDefenseModsOp, parent_context: EventContext, event_queue: EventOpQueue) -> DamageTakenData:
	#var calc_def_context := EventBuilder.build_damage_taken_calc_ctx_obj(operation, parent_context)
	#var follow_queue := EventOpQueue.new()
	#GameMaster.emit_trigger(calc_def_context, follow_queue)
	## enqueue ONLY the defense-calc follow-ups as barrier
	#event_queue.enqueue_barrier_ops(follow_queue)
#
	#var payload: DamageTakenData = calc_def_context.get_damage_taken()
	#return payload
#
## 4) Apply with defender mods (returns follow-ups; pure except for HP write)
#static func _apply_damage_with_def(operation: DamageApplyOp, _parent_context: EventContext) -> EventOpQueue:
	#var target_unit: Unit = operation.tgts[0]
	#var def_mods: DamageTakenData = operation.def_mods  # typed payload
#
	#var base_defense := float(target_unit.get_base_stat("base_def"))
	#var flat_reduction := float(def_mods.flat)
	#var percent_reduction := float(def_mods.percent) + base_defense
	#var actual_damage := int(round(((1.0 - percent_reduction) * operation.amount) - flat_reduction))
	#target_unit.take_damage(actual_damage)
#
	#var after_queue := EventOpQueue.new()
	#var on_damaged_context := EventBuilder.build_on_damaged_ctx_obj(operation.src, target_unit, actual_damage, operation.dtype, operation.tags)
	#GameMaster.emit_trigger(on_damaged_context, after_queue)  # non-barrier
	#return after_queue
#
## Fast path: direct apply (returns follow-ups; pure except for HP write)
#static func _apply_damage_only(operation: DamageDirectOp, _parent_context: EventContext) -> EventOpQueue:
	#var after_queue := EventOpQueue.new()
	#for target_unit: Unit in operation.tgts:
		#target_unit.take_damage(operation.amount)
		#var on_damaged_context := EventBuilder.build_on_damaged_ctx_obj(operation.src, target_unit, operation.amount, operation.dtype, operation.tags)
		#GameMaster.emit_trigger(on_damaged_context, after_queue)
	#return after_queue
#
#
## ---------------- Non-damage ops ----------------
#
#static func _apply_mod_stat_op(operation: ModStatOp, _parent_context: EventContext, _event_queue: EventOpQueue) -> void:
	#var current_value := operation.target.get_base_stat(operation.stat)
	#match operation.mode:
		#"add":
			#operation.target.change_base_stat(operation.stat, int(current_value + operation.delta))
		#"set":
			#operation.target.change_base_stat(operation.stat, int(operation.delta))
		#_:
			#push_warning("Applier:_apply_mod_stat unknown mode: %s" % operation.mode)
#
#static func _apply_status_op(_operation: ApplyStatusOp, _parent_context: EventContext, _event_queue: EventOpQueue) -> void:
	## TODO: integrate your status system
	#pass
