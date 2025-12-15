class_name DamageCalcData
extends ContextPayload

var base: float = 0.0
var flat: float = 0.0
var percent: float = 0.0
var dtype: String = "phys"
var tags: Array[String] = []
var final: int = 0

func merge_with(other: ContextPayload) -> ContextPayload:
	if other is DamageCalcData:
		var o := other as DamageCalcData
		var out := DamageCalcData.new()
		out.base = (base != 0.0) if base else o.base     # keep existing base if set, else other
		out.flat = flat + o.flat
		out.percent = percent + o.percent
		# dtype/tags should match; if not, prefer existing
		out.dtype = (dtype != "") if dtype else o.dtype
		out.tags = tags.duplicate()
		out.final = o.final
		if out.tags.is_empty() and not o.tags.is_empty():
			out.tags = o.tags.duplicate()
		return out
	return self
