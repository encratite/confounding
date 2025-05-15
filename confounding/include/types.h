#pragma once

#include <chrono>

namespace confounding {
	typedef std::chrono::year_month_day Date;
	typedef std::chrono::local_time<std::chrono::seconds> Time;
}