class_name ExpressionStore
extends Node

static var _pool: Dictionary = {}   # src_string -> MyExpression
static var _normalize := true

static func _norm(s: String) -> String:
	if not _normalize: return s
	var reg := RegEx.new(); reg.compile("\\s+")
	return reg.sub(s, "", true)

static func get_or_create(src: String) -> MyExpression:
	var key := _norm(src)
	if _pool.has(key):
		return _pool[key]
	var expr := MyExpression.new(src)
	_pool[key] = expr
	return expr

static func clear() -> void:
	_pool.clear()
