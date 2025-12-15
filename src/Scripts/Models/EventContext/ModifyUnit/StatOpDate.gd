class_name StatOpData
extends ContextPayload

var name: String = ""
var mode: String = "add"  # "add" | "set"
var delta: float = 0.0

func merge_with(other: ContextPayload) -> ContextPayload:
	if other is StatOpData:
		var o := other as StatOpData
		var out := StatOpData.new()
		out.name = (name != "") if name else o.name
		# merging a stat op is usually undefined; keep first and add deltas
		out.mode = mode
		out.delta = delta + o.delta
		return out
	return self
