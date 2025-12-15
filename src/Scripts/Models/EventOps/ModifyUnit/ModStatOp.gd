class_name ModStatOp
extends EventOp

var target: Unit
var stat: String
var mod_type: String
var delta: float
var mode: String = "add"  # "add" | "set"

func _init():
	kind = "mod_stat"
