#pragma once

#include <string>
#include <format>

#include "confounding/exports.h"
#include "confounding/exception.h"
#include "confounding/types.h"

namespace confounding {
	class CONFOUNDING_API Money {
	public:
		Money();
		Money(int64_t amount);
		Money(std::string amount_string);

		int64_t to_int() const;
		double to_double() const;
		int32_t operator-(Money other) const;

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

	std::shared_ptr<char> CONFOUNDING_API read_file(const std::string& path);
	Date CONFOUNDING_API get_date(const std::string& string);
	Date CONFOUNDING_API get_date(Time time);
	void CONFOUNDING_API add_day(Date& date);
	Time CONFOUNDING_API get_time(const std::string& string);
	Time CONFOUNDING_API get_time(Date date);
	TimeOfDay CONFOUNDING_API get_time_of_day(const std::string& string);
	TimeOfDay CONFOUNDING_API get_time_of_day(Time time);
	std::string CONFOUNDING_API get_date_string(Date date);
	std::string CONFOUNDING_API get_time_string(Time time);
	double CONFOUNDING_API get_rate_of_change(double a, double b);
	bool CONFOUNDING_API operator<(Time time, Date date);
	Money CONFOUNDING_API operator*(unsigned factor, Money money);

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