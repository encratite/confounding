#pragma once

#include <chrono>
#include <filesystem>

namespace confounding {
	typedef std::chrono::year_month_day Date;
	typedef std::chrono::local_time<std::chrono::seconds> Time;
	typedef std::filesystem::path Path;
}