# systems/MatchState.gd
class_name SaveState
extends RefCounted

var day: int = 1
var weekday: String = TimeConstants.MONDAY
var time_of_day: String = TimeConstants.MORNING
var flags: Dictionary = {} # FlagId -> bool

var units: Array[Unit] = []   # live runtime units (save-bound)

func init_from_save(save_data: Dictionary) -> void:
	day = int(save_data.get("day", 1))
	time_of_day = String(save_data.get("time_of_day", "MORNING"))
	flags = save_data.get("flags", {})
	# TODO: load player units


func eval(cond_type: String, cond_value: String, ctx: EventContext) -> bool:
	# TODO: Add all checks.
	match cond_type:
		"EVENTCONTEXT":
			return ctx.additional_data.has("event_context") and ctx.additional_data.get("event_context").get(cond_type) == cond_value
		"WEEKDAY":
			return weekday == cond_value
		"TIMERANGE":
			return  time_of_day == cond_value
		_:
			push_warning("Unknown condition type: %s" % cond_type)
			return false
