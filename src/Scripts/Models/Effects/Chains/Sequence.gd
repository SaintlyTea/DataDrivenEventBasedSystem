class_name Sequence
extends EffectBase

var steps: Array[EffectBase] = []
var atomic: bool = true  # guarantees no interleaving if your scheduler honors it

func apply(ctx: EventContext, out: EventOpQueue) -> void:
	for e in steps:
		if e.eval(ctx): e.apply(ctx, out)
