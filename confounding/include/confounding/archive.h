#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "common.h"
#include "types.h"

namespace confounding {
	struct DailyRecord {
		Date date;
		Money close;
	};


	/*
	The Z-score features for intraday records are initially calculated using doubles for greater accuracy, especially for the standard deviation.
	The values that are ultimately persisted to the archive are floats rather than doubles for performance reasons.
	*/
	template<typename T>
	requires (std::same_as<T, float> || std::same_as<T, double>)
	struct IntradayRecordT {
		// close(t) / last_session_close(t - 8 h)
		T momentum_1d;
		// close(t) / last_session_close(t - 8 h - 1 day)
		T momentum_2d;
		// last_session_close(t - 8 h) / last_session_close(t - 8 h - 1 day)
		T momentum_2d_gap;
		// close(t) / close(t - 8 h)
		T momentum_8h;
		// close(t) / last_session_close(t - 8 h - 9 days)
		T momentum_10d;
		// close(t) / last_session_close(t - 8 h - 39 days)
		T momentum_40d;
		// daily_volatility(10 days)
		T volatility_10d;
		// daily_volatility(40 days)
		T volatility_40d;
		// All returns are stored as ticks rather than relative return or money
		// Returns do not contain slippage and commission
		// Returns to next daily close that lies at least 8 hours after the current point in time
		int32_t returns_next_close;
		// Returns for fixed holding periods for a certain number of hours
		int32_t returns_20h;
		int32_t returns_22h;
		int32_t returns_24h;
		int32_t returns_26h;
		int32_t returns_28h;
		int32_t returns_48h;
		int32_t returns_72h;
		// Missing: TP/SL-based returns
	};

	typedef IntradayRecordT<double> RawIntradayRecord;
	typedef IntradayRecordT<float> IntradayRecord;

	struct Archive {
		std::string symbol; 
		std::vector<DailyRecord> daily_records;
		std::vector<Time> intraday_timestamps;
		std::vector<IntradayRecord> intraday_records;
	};
}