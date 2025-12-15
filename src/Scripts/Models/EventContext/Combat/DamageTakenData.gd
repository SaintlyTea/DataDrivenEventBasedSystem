class_name DamageTakenData
extends ContextPayload

var base_def: float = 0.0
var flat: float = 0.0
var percent: float = 0.0
var dtype: String = "phys"
var tags: Array[String] = []

func merge_with(other: ContextPayload) -> ContextPayload:
	if other is DamageTakenData:
		var o := other as DamageTakenData
		var out := DamageTakenData.new()
		out.base_def = (base_def != 0.0) if base_def else o.base_def
		out.flat = flat + o.flat
		out.percent = percent + o.percent
		out.dtype = (dtype != "") if dtype else o.dtype
		if out.tags.is_empty() and not o.tags.is_empty():
			out.tags = o.tags.duplicate()
		return out
	return self
