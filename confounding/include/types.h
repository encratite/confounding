#pragma once

#include <chrono>
#include <filesystem>

namespace confounding {
	typedef std::chrono::year_month_day Date;
	// Warning: This only work for H1 bars and will have to be modified to std::chrono::minutes with M30 bars
	typedef std::chrono::local_time<std::chrono::hours> Time;
	// Can't use std::chrono::hours in this case because several assets have session closes at 12:30 PM/1:30 PM
	typedef std::chrono::hh_mm_ss<std::chrono::minutes> TimeOfDay;
	typedef std::filesystem::path Path;
}