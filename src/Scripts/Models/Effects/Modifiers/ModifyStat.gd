# Scripts/Models/Effects/Modifiers/ModifyStat.gd
class_name StatModEffect
extends EffectBase

var stat_type: String
var amount: float
var change_type: String    # "flat" or "percent" 

func apply(ctx: EventContext, out: EventOpQueue) -> void:
	for unit: Unit in ctx.trigger_targets:
		out.mod_stat(unit, stat_type, change_type, amount)
