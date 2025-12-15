# Scripts/Models/Effects/Modifiers/ModifyDmg.gd
class_name ModifyDmg
extends EffectBase

var amount: float
var change_type: String    # "flat" | "percent"

func apply(ctx: EventContext, out: EventOpQueue) -> void:
	match change_type:
		"flat":
			ctx.add_data("damage", {"flat": amount})
		"percent":
			ctx.add_data("damage", {"percent": amount})
