class_name EventPostTriggers
extends Resource

# After committing an op, which events should fire, and are they barriers?
# "barrier: true" means process these triggers' produced ops BEFORE continuing.
# Replace method name with a function that makes the method name into a string, or make const
static var _POST_TRIGGERS := {
	"damage": [
		{"event": "OnDamaged", "builder": Callable(Applier, "_build_ctx_damage"), "barrier": false},
	],
	"mod_stat": [
		{"event": "OnStatModified", "builder": Callable(Applier, "_build_ctx_stat"), "barrier": true}, # your requirement
	],
	"apply_status": [
		{"event": "OnStatusApplied", "builder": Callable(Applier, "_build_ctx_status"), "barrier": false},
	],
}
