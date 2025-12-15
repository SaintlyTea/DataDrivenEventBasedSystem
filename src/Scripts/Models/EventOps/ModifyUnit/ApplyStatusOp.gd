class_name ApplyStatusOp
extends EventOp

var target: Unit
var status_id: String
var stacks: int = 1
var duration: int = 1

func _init():
	kind = "apply_status"
