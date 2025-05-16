#pragma once

#include <string>
#include <vector>

#include "types.h"

namespace confounding {
	struct DailyRecord {
		Date date;
		double close;
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
		// Returns do not include slippage or commission
		// Returns to next daily close that lies at least 8 hours after the current point in time
		double returns_next_close;
		// Returns for fixed holding periods of 24/48/72 hours
		double returns_24h;
		double returns_48h;
		double returns_72h;
		// Missing: TP/SL-based returns
	};

	struct Archive {
		std::string symbol;
		std::vector<DailyRecord> daily_records;
		std::vector<Time> intraday_time;
		std::vector<IntradayRecord> intraday_records;
	};
}