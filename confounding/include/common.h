#pragma once

#include <string>
#include <format>

#include "exception.h"
#include "types.h"

namespace confounding {
	std::shared_ptr<char> read_file(const std::string& path);
	Date get_date(const std::string& string);
	Date get_time_from_date(Time time);
	Time get_time(const std::string& string);
	std::string get_date_string(const Date& date);
	bool operator<(const Time& time, const Date& date);
}