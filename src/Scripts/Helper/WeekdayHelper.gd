class_name WeekdayHelper
extends Resource

var day: int = 1

func calc_weekday()-> String:
	var number = day%7
	match number:
		0:
			return TimeConstants.MONDAY 
		1:
			return TimeConstants.TUESDAY
		2:
			return TimeConstants.WEDNESDAY
		3:
			return TimeConstants.THURSDAY
		4:
			return TimeConstants.FRIDAY
		5:
			return TimeConstants.SATURDAY
		_:
			return TimeConstants.SUNDAY
