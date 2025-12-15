class_name Applier
extends RefCounted

static func commit(event_queue: EventOpQueue, parent_context: EventContext) -> void:
	var fifo_index := 0
	var processed_count := 0
	const MAX_OPS := 10000

	while processed_count < MAX_OPS and (event_queue.lifo.size() > 0 or fifo_index < event_queue.fifo.size()):
		var operation: EventOp
		if event_queue.lifo.size() > 0:
			operation = event_queue.lifo.pop_back()        # barrier (depth-first)
		else:
			operation = event_queue.fifo[fifo_index]       # normal FIFO without shifting
			fifo_index += 1

		var handler: Callable = GetEventHandler.get_handler_for(operation)
		if handler == null:
			push_warning("Applier: no handler for op class=%s kind=%s" % [operation.get_class(), operation.kind])
		else:
			# Handlers can enqueue follow-ups through event_queue
			handler.call(operation, parent_context, event_queue)

		# Post-apply triggers (keyed by op.kind; builders accept op objects)
		var post_defs: Array = EventPostTriggers._POST_TRIGGERS.get(operation.kind, [])
		for def_item in post_defs:
			var builder: Callable = def_item.get("builder")
			var event_type: String = def_item.get("event")
			var is_barrier: bool = bool(def_item.get("barrier", false))

			var emit_ctx: EventContext = builder.call(operation, parent_context)
			emit_ctx.event_type = event_type

			var follow_queue := EventOpQueue.new()
			GameMaster.emit_trigger(emit_ctx, follow_queue)

			if is_barrier:
				event_queue.enqueue_barrier_ops(follow_queue)
			else:
				event_queue.enqueue_normal_ops(follow_queue)

		processed_count += 1

	if processed_count >= MAX_OPS:
		push_error("Applier: exceeded MAX_OPS; possible infinite loop.")
