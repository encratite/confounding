#pragma once

#include <string>

#include "exception.h"
#include "types.h"

namespace confounding {
	std::shared_ptr<char> read_file(const std::string& path);
	Date get_date(const std::string& string);
	bool operator<(const Time& time, const Date& date);
}