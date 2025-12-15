# autoload/DataRepo.gd
class_name DataRepo
extends Node

@export var base_path := "res://data"
@export var max_cache_items := 256

var _cache: Dictionary = {}       # id -> Resource
var _lru: Array[String] = []      # id order, least-recent at front

func _path_for(kind: String, id: String) -> String:
	return "%s/%s/%s.res" % [base_path, kind, id]

func clear_per_match() -> void:
	_cache.clear()
	_lru.clear()

func _touch(id: String) -> void:
	var i := _lru.find(id)
	if i != -1: _lru.remove_at(i)
	_lru.append(id)

func _evict() -> void:
	while _lru.size() > max_cache_items:
		var victim :String = _lru.pop_front()
		_cache.erase(victim)

func _get_res(kind: String, id: String) -> Resource:
	if _cache.has(id):
		_touch(id)
		return _cache[id]
	var path := _path_for(kind, id)
	var res := ResourceLoader.load(path)
	if res == null:
		push_error("Resource not found: %s" % path)
		return null
	_cache[id] = res
	_touch(id)
	_evict()
	return res
