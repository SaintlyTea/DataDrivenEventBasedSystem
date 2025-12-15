# autoload/Game.gd
class_name Game
extends Node

var state: SaveState
var _trigger_handler: TriggerHandler

func _ready() -> void:
	_trigger_handler = TriggerHandler.new()

# ---------- Match lifecycle ----------
func start_match(save_data: Dictionary) -> void:
	# reset per-match stuff
	DataRepository.clear_per_match()
	state = SaveState.new()
	state.init_from_save(save_data)

# ---------- Triggers (callable from anywhere) ----------
func emit_trigger(ctx: EventContext, evOpQueue: EventOpQueue) -> void:
	_trigger_handler.exe_trigger(ctx, evOpQueue)

func register_trigger(trigger_type: String, effect: EffectBase) -> void:
	_trigger_handler.register(trigger_type, effect)

func unregister_trigger(trigger_type: String, effect: EffectBase) -> void:
	_trigger_handler.unregister(trigger_type, effect)

# ---------- Units ----------
func list_units() -> Array[Unit]:
	return state.units
