class_name StatusApplyData
extends ContextPayload

var id: String = ""
var stacks: int = 1
var duration: int = 1

func merge_with(other: ContextPayload) -> ContextPayload:
	if other is StatusApplyData and other.id == id:
		var out := StatusApplyData.new()
		out.id = id
		out.stacks = stacks + other.stacks
		out.duration = max(duration, other.duration)
		return out
	return self
