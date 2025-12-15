class_name EventOpQueue
extends RefCounted

var fifo: Array[EventOp] = []
var lifo: Array[EventOp] = []

var ops: Array[EventOp] : get = _get_ops
func _get_ops() -> Array[EventOp]: return fifo

# -------- low-level enqueue ----------

func push_normal_op(operation: EventOp) -> void:
	fifo.append(operation)

func push_barrier_op(operation: EventOp) -> void:
	lifo.append(operation)

func enqueue_normal_ops(batch: EventOpQueue) -> void:
	if batch and batch.fifo.size() > 0:
		fifo.append_array(batch.fifo)

func enqueue_barrier_ops(batch: EventOpQueue) -> void:
	if batch and batch.fifo.size() > 0:
		for i in range(batch.fifo.size() - 1, -1, -1):
			lifo.append(batch.fifo[i])
