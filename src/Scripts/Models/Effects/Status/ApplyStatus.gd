class_name ApplyStatus
extends EffectBase

var status_id: String
var stacks: int = 1
var duration: int = 2

func apply(ctx: EventContext, out: EventOpQueue) -> void:
	for tgt: Unit in ctx.trigger_targets:
		out.apply_status(tgt, status_id, stacks, duration)
