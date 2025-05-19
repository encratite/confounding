#pragma once

#include <string>

#include "contract.h"
#include "types.h"
#include "globex.h"

namespace confounding {
	struct GlobexRecord {
		GlobexCode globex_code;
		Date date;
		double close;
		unsigned open_interest;
	};

	struct IntradayRecordsKey {
		Date date;
		GlobexCode globex_code;

		IntradayRecordsKey(Date date, GlobexCode globex_code);

		bool operator<(const IntradayRecordsKey& key) const;
	};

	struct IntradayClose {
		Time time;
		double close;
	};

	typedef std::map<Date, std::vector<GlobexRecord>> GlobexRecordMap;
	typedef std::map<IntradayRecordsKey, std::vector<IntradayClose>> IntradayRecordMap;

	void parse_futures_all();
	void parse_futures_single(const Contract& symbol);
	GlobexRecordMap read_daily_records(const std::string& symbol);
	IntradayRecordMap read_intraday_records(const std::string& symbol);
	std::string get_symbol_path(const std::string& symbol, const std::string& suffix);
	void generate_archive(
		std::optional<unsigned> f_number,
		bool fy_record,
		const std::string& symbol,
		const GlobexRecordMap& daily_records,
		const IntradayRecordMap& intraday_records
	);
}