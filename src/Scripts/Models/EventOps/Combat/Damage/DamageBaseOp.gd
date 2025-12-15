class_name DamageBaseOp
extends EventOp

var src: Unit
var tgts: Array[Unit] = []     # ALWAYS an array
var dtype: String = "phys"
var tags: Array[String] = []
func _init():
	kind = "damage"
