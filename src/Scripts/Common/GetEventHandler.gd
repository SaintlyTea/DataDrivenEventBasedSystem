class_name GetEventHandler
extends Resource

static var _HANDLERS_BY_CLASS := {
	"DamageDirectOp":               Callable(EventHandlers, "_run_damage_flow"),
	"DamageGatherAttackModsOp":     Callable(EventHandlers, "_run_damage_flow"),
	"DamageFinalizeOp":             Callable(EventHandlers, "_run_damage_flow"),
	"DamageGatherDefenseModsOp":    Callable(EventHandlers, "_run_damage_flow"),
	"DamageApplyOp":                Callable(EventHandlers, "_run_damage_flow"),

	"ModStatOp":                    Callable(EventHandlers, "_apply_mod_stat_op"),
	"ApplyStatusOp":                Callable(EventHandlers, "_apply_status_op"),
}

static var _HANDLERS_BY_KIND := {
	"damage":       Callable(EventHandlers, "_run_damage_flow"),
	"mod_stat":     Callable(EventHandlers, "_apply_mod_stat_op"),
	"apply_status": Callable(EventHandlers, "_apply_status_op"),
}

static func get_handler_for(operation: EventOp) -> Callable:
	var op_class_name := operation.get_class()
	var by_class: Callable = _HANDLERS_BY_CLASS.get(op_class_name, null)
	if by_class != null:
		return by_class
	return _HANDLERS_BY_KIND.get(operation.kind, null)
