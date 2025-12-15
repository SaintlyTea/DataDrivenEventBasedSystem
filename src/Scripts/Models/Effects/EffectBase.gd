class_name EffectBase
extends Resource

var id: String
var name: String

var trigger: String
var conditions: MyExpression 

var host: Unit
var source: Unit

var enabled_stacks: bool = false
var max_stacks: int = -1 # -1 means infinite use
var current_stacks: int = 0
var persists_combat: bool = true
var resetable: bool = false

var duration_turns_left: int = -1 # means no duration
var cd_turns_left: int = 0
var cd_base: int = 0 # 0 means no cd

var tags: Array[String] = []

func _init(efName: String, efTrigger: String, efConditionString: String,
efHost: Unit, efSource: Unit, efTags: Array[String] = [], efMaxStacks: int = -1, efCurrentStacks: int = 0, 
efPersistsCombat: bool = true, efResetable: bool = false, efDurationTurnsLeft: int = -1,
efCdTurnsLeft: int = 0, efCdBase: int = 0) -> void:
	name = efName
	trigger = efTrigger
	host = efHost
	source = efSource
	max_stacks = efMaxStacks
	current_stacks = efCurrentStacks
	persists_combat = efPersistsCombat
	resetable = efResetable
	duration_turns_left = efDurationTurnsLeft
	cd_turns_left = efCdTurnsLeft
	cd_base = efCdBase
	tags = efTags
	
	conditions = ExpressionStore.get_or_create(efConditionString)

func enqueue(ctx: EventContext, ev_op_queue: EventOpQueue) -> void:
	if conditions.evaluate(ctx):
		self._enqueue(ctx, ev_op_queue)

func _enqueue(ctx: EventContext, ev_op_queue: EventOpQueue):
	pass

func execute(ctx: EventContext, ev_op_queue: EventOpQueue, op: EventOp)->void:
	if conditions.evaluate(ctx):
		self._execute(ctx, ev_op_queue, op)

func _execute(ctx: EventContext, ev_op_queue: EventOpQueue, op: EventOp)->void:
	pass

func eval_conditions(context: EventContext)->bool:
	return conditions.evaluate(context)
