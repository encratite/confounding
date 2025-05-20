#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "types.h"

namespace confounding {
	struct DailyRecord {
		Date date;
		double close;

		DailyRecord(Date date, double close);
	};

	struct IntradayRecord {
		// Z-score of close(t) / last_session_close(t - 8 h)
		double momentum_1d;
		// Z-score of close(t) / last_session_close(t - 8 h - 1 day)
		double momentum_2d;
		// Z-score of last_session_close(t - 8 h) / last_session_close(t - 8 h - 1 day)
		double momentum_2d_gap;
		// Z-score of close(t) / close(t - 8 h)
		double momentum_8h;
		// Z-score of close(t) / last_session_close(t - 8 h - 9 days)
		double momentum_10d;
		// Z-score of close(t) / last_session_close(t - 8 h - 39 days)
		double momentum_40d;
		// Z-score of daily_volatility(10 days)
		double volatility_10d;
		// Z-score of daily_volatility(40 days)
		double volatility_40d;
		// All returns as ticks rather than relative return or money
		// Returns do slippage or commission and are specific to one side (long/short)
		// Returns to next daily close that lies at least 8 hours after the current point in time
		int32_t returns_next_close_long;
		int32_t returns_next_close_short;
		// Returns for fixed holding periods of 24/48/72 hours
		int32_t returns_24h_long;
		int32_t returns_24h_short;
		int32_t returns_48h_long;
		int32_t returns_48h_short;
		int32_t returns_72h_long;
		int32_t returns_72h_short;
		// Missing: TP/SL-based returns
	};

	struct Archive {
		std::string symbol;
		std::vector<DailyRecord> daily_records;
		std::vector<Time> intraday_time;
		std::vector<IntradayRecord> intraday_records;
	};
}