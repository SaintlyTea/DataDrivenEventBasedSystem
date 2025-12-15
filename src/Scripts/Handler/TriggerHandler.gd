# systems/TriggerHandler.gd
class_name TriggerHandler
extends RefCounted

var _by_type: Dictionary = {}  # trigger_type -> Array[EffectBase]

func register(trigger_type: String, effect: EffectBase) -> void:
	if not _by_type.has(trigger_type):
		_by_type[trigger_type] = []
	_by_type[trigger_type].append(effect)

func unregister(trigger_type: String, effect: EffectBase) -> void:
	var arr: Array = _by_type.get(trigger_type, [])
	arr.erase(effect)
	if arr.is_empty():
		_by_type.erase(trigger_type)

# Scripts/handler/TriggerHandler.gd
func exe_trigger(ctx: EventContext, evOpQueue: EventOpQueue) -> void:
	var effects: Array = _by_type.get(ctx.event_type, [])
	if effects.is_empty(): return
	var host: Unit = ctx.trigger_source

	for ef: EffectBase in effects:
		if ef.host == host and ef.eval_conditions(ctx):
			ef.apply(ctx, evOpQueue)
	for ef: EffectBase in effects:
		if ef.host != host and ef.eval_conditions(ctx):
			ef.apply(ctx, evOpQueue)
