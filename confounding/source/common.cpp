#include <memory>
#include <chrono>
#include <cstdio>
#include <format>
#include <regex>

#include "common.h"
#include "exception.h"

namespace confounding {
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

	Date get_time_from_date(Time time) {
		std::chrono::local_days local_days = std::chrono::floor<std::chrono::days>(time);
		Date date = Date{ local_days };
		return date;
	}

	Time get_time(const std::string& string) {
		std::istringstream stream(string);
		Time time;
		stream >> std::chrono::parse("%F %R", time);
		if (stream.fail())
			throw Exception("Failed to parse time: {}", string);
		return time;
	}

	TimeOfDay get_time_of_day(const std::string& string) {
		static std::regex pattern(R"(^(\d{2}):(\d{2})$)");
		std::smatch match;
		if (std::regex_search(string, match, pattern)) {
			unsigned hours = get_number<unsigned>(match[1]);
			unsigned minutes = get_number<unsigned>(match[2]);
			TimeOfDay time_of_day{ std::chrono::hours(hours) + std::chrono::minutes(minutes) };
			return time_of_day;
		} else {
			throw Exception("Unable to parse time of day string: {}", string);
		}
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

	bool operator<(const Time& time, const Date& date) {
		using namespace std::chrono;
		auto days = local_days{date};
		return time < days;
	}
}