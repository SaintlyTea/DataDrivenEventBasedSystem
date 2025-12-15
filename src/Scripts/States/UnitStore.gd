class_name UnitStore
extends Resource

static var dicOfUnits: Dictionary # Id -> Unit

static func get_unit(id: String)->Unit:
	return dicOfUnits.get(id)

static func get_units(ids: Array[String])->Array[Unit]:
	var units: Array[Unit] = []
	for id in ids:
		units.append(dicOfUnits.get(id))
	return units
