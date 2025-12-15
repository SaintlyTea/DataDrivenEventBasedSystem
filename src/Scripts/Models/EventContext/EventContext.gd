class_name EventContext
extends RefCounted

var event_type: String
var trigger_source: Unit
var trigger_targets: Array[Unit] = []

# key -> ContextPayload
var _payloads: Dictionary = {}

# -------- Public API (typed) --------

func set_payload(key: String, payload: ContextPayload) -> void:
	if payload == null:
		return
	if _payloads.has(key):
		var merged := (_payloads[key] as ContextPayload).merge_with(payload)
		_payloads[key] = merged
	else:
		_payloads[key] = payload

func get_payload(key: String) -> ContextPayload:
	return _payloads.get(key, null)

func has_payload(key: String) -> bool:
	return _payloads.has(key)

# Convenience typed accessors
func set_damage_calc(data: DamageCalcData) -> void:
	set_payload("damage", data)

func get_damage_calc() -> DamageCalcData:
	return get_payload("damage") as DamageCalcData

func set_damage_taken(data: DamageTakenData) -> void:
	set_payload("defense", data)

func get_damage_taken() -> DamageTakenData:
	return get_payload("defense") as DamageTakenData

# Optional generics for future payloads:
func set_stat_op(data: StatOpData) -> void:
	set_payload("stat", data)

func get_stat_op() -> StatOpData:
	return get_payload("stat") as StatOpData

func set_status_apply(data: StatusApplyData) -> void:
	set_payload("status", data)

func get_status_apply() -> StatusApplyData:
	return get_payload("status") as StatusApplyData
