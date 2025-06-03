#pragma once

#include <string>
#include <format>

#include "exception.h"
#include "types.h"

namespace confounding {
	class Money {
	public:
		Money();
		Money(int64_t amount);
		Money(std::string amount_string);

		int64_t to_int() const;
		double to_double() const;
		int32_t operator-(const Money& other) const;

	private:
		// Encodes both the integer and the fractional portion of the price
		int64_t _amount;

		static constexpr int64_t get_base_10_factor(int exponent) {
			int64_t output = 1;
			for (int i = 0; i < exponent; i++)
				output *= 10;
			return output;
		}
	};

	std::shared_ptr<char> read_file(const std::string& path);
	Date get_date(const std::string& string);
	Date get_date(Time time);
	void add_day(Date& date);
	Time get_time(const std::string& string);
	Time get_time(Date date);
	TimeOfDay get_time_of_day(const std::string& string);
	TimeOfDay get_time_of_day(Time time);
	std::string get_date_string(const Date& date);
	std::string get_time_string(const Time& time);
	double get_rate_of_change(double a, double b);
	bool operator<(const Time& time, const Date& date);
	Money operator*(unsigned factor, const Money& money);

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