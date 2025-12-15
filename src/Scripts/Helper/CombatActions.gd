class_name CombatActions
extends Resource

func run_ability(targets: Array[Unit], ability: EffectBase) -> void:
	var ctx := EventContext.new()
	ctx.event_type = "OnAbilityCastAnnounced"
	ctx.trigger_source = ability.host
	ctx.trigger_targets = targets
	var evOpQueue := EventOpQueue.new()

	GameMaster.emit_trigger(ctx, evOpQueue) 

	ctx.event_type = "OnAbilityCast"
	ability.enqueue(ctx, evOpQueue)

	Applier.commit(evOpQueue, ctx)
