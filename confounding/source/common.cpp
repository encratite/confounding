#include <memory>
#include <chrono>
#include <cstdio>
#include <format>
#include <regex>
#include <cmath>

#include "common.h"
#include "exception.h"

namespace confounding {
	// The required minimum for 6J
	constexpr int money_precision = 7;

	Money::Money()
		: _amount(0) {
	}

	Money::Money(int64_t amount)
		: _amount(amount) {
	}

	Money::Money(std::string amount_string) {
		static std::regex pattern(R"(^-?(0|[1-9]\d*)(\.(\d+))?$)");
		std::smatch match;
		if (std::regex_search(amount_string, match, pattern)) {
			int64_t integer_part = get_number<int64_t>(match[1]);
			constexpr int64_t integer_factor = get_base_10_factor(money_precision);
			std::string fractional_string = match[3];
			if (fractional_string.size() > money_precision)
				throw Exception("Fractional part of money string too long: {}", amount_string);
			int64_t fractional_part = get_number<int64_t>(fractional_string);
			int delta = money_precision - static_cast<int>(fractional_string.size());
			int64_t fractional_factor = get_base_10_factor(delta);
			_amount = integer_factor * integer_part + fractional_factor + fractional_part;
		} else {
			throw Exception("Unable to parse money string: {}", amount_string);
		}
	}

	int64_t Money::to_int() const {
		return _amount;
	}

	double Money::to_double() const {
		return static_cast<double>(_amount) * std::pow(1.0, -money_precision);
	}

	std::shared_ptr<char> read_file(const std::string& path) {
		FILE* file = std::fopen(path.c_str(), "rb");
		if (file == nullptr) {
			std::string error = std::format("Failed to open file ({})", strerror(errno));
			throw Exception(error);
		}
		std::fseek(file, 0, SEEK_END);
		std::size_t file_size = std::ftell(file);
		std::shared_ptr<char> buffer(new char[file_size], std::default_delete<char[]>());
		std::size_t bytes_read = std::fread(buffer.get(), 1, file_size, file);
		std::fclose(file);
		return buffer;
	}

	Date get_date(const std::string& string) {
		std::istringstream stream(string);
		Date date;
		stream >> std::chrono::parse("%F", date);
		if (stream.fail())
			throw Exception("Failed to parse date: {}", string);
		return date;
	}

	Date get_date(Time time) {
		std::chrono::local_days local_days = std::chrono::floor<std::chrono::days>(time);
		Date date = Date{local_days};
		return date;
	}

	void add_day(Date& date) {
		auto days = std::chrono::sys_days{date} + std::chrono::days{1};
		date = Date{days};
	}

	Time get_time(const std::string& string) {
		std::istringstream stream(string);
		Time time;
		stream >> std::chrono::parse("%F %R", time);
		if (stream.fail())
			throw Exception("Failed to parse time: {}", string);
		return time;
	}

	Time get_time(Date date) {
		Time output = std::chrono::local_days{date};
		return output;
	}

	TimeOfDay get_time_of_day(const std::string& string) {
		static std::regex pattern(R"(^(\d{2}):(\d{2})$)");
		std::smatch match;
		if (std::regex_search(string, match, pattern)) {
			unsigned hours = get_number<unsigned>(match[1]);
			unsigned minutes = get_number<unsigned>(match[2]);
			TimeOfDay time_of_day{std::chrono::hours(hours) + std::chrono::minutes(minutes)};
			return time_of_day;
		} else {
			throw Exception("Unable to parse time of day string: {}", string);
		}
	}

	TimeOfDay get_time_of_day(Time time) {
		std::chrono::local_days local_days = std::chrono::floor<std::chrono::days>(time);
		auto hours = time - local_days;
		TimeOfDay time_of_day{hours};
		return time_of_day;
	}

	std::string get_date_string(const Date& date) {
		std::string output = std::format(
			"{:04}-{:02}-{:02}",
			static_cast<int>(date.year()),
			static_cast<unsigned>(date.month()),
			static_cast<unsigned>(date.day())
		);
		return output;
	}

	double get_rate_of_change(double a, double b) {
		if (a < 0 || b <= 0)
			throw Exception("Invalid parameters for get_rate_of_change");
		return a / b - 1.0;
	}

	bool operator<(const Time& time, const Date& date) {
		using namespace std::chrono;
		auto days = local_days{date};
		return time < days;
	}
}