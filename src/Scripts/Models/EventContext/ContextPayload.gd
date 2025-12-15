class_name ContextPayload
extends RefCounted

# Override in subclasses to define how two payloads of the *same type* merge.
func merge_with(other: ContextPayload) -> ContextPayload:
	return self  # default: no-op
