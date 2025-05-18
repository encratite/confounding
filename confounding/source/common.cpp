#include <memory>
#include <chrono>
#include <cstdio>
#include <format>

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
		{
			std::stringstream error_stream;
			error_stream << "Failed to parse date: " << string << std::endl;
			throw Exception(error_stream.str());
		}
		return date;
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