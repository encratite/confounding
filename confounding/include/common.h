#pragma once

#include <string>
#include <format>

#include "exception.h"
#include "types.h"

namespace confounding {
	std::shared_ptr<char> read_file(const std::string& path);
	Date get_date(const std::string& string);
	Date get_date(Time time);
	void add_day(Date& date);
	Time get_time(const std::string& string);
	Time get_time(Date date);
	TimeOfDay get_time_of_day(const std::string& string);
	TimeOfDay get_time_of_day(Time time);
	std::string get_date_string(const Date& date);
	double get_rate_of_change(double a, double b);
	bool operator<(const Time& time, const Date& date);

	template<typename T>
	T get_number(const std::string& string) {
		T result;
		auto start = string.c_str();
		auto end = start + string.length();
		auto [ptr, ec] = std::from_chars(start, end, result);
		if (ec != std::errc())
			throw Exception("Invalid numeric string: {}", string);
		return result;
	}
}