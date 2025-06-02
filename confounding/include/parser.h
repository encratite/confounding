#pragma once

#include <string>

#include "contract.h"
#include "filter.h"
#include "types.h"
#include "globex.h"
#include "archive.h"

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
	void parse_futures_single(const Contract& contract);
	GlobexRecordMap read_daily_records(const std::string& symbol, const ContractFilter& filter);
	IntradayRecordMap read_intraday_records(const std::string& symbol, const ContractFilter& filter);
	std::string get_symbol_path(const std::string& symbol, const std::string& suffix);
	void generate_archive(
		std::optional<unsigned> f_number,
		bool fy_record,
		const std::string& symbol,
		const GlobexRecordMap& daily_records,
		const IntradayRecordMap& intraday_records,
		const ContractFilter& filter
	);
	GlobexRecord get_daily_globex_record(
		Date date,
		std::optional<unsigned> f_number,
		bool fy_record,
		const std::string& symbol,
		const std::vector<GlobexRecord>& records,
		Archive& archive
	);
	void update_recent_closes(
		const GlobexRecord& daily_globex_record,
		std::deque<double>& recent_closes,
		std::deque<double>& recent_returns
	);
	void generate_intraday_record(
		const IntradayClose& record,
		Date date,
		const std::vector<IntradayClose>& intraday_closes,
		const std::deque<double>& recent_closes,
		const std::deque<double>& recent_returns,
		std::vector<RawIntradayRecord>& raw_intraday_records,
		Archive& archive,
		const ContractFilter& filter
	);
	double get_volatility(std::size_t n, const std::deque<double>& recent_returns);
	void add_nan_record(std::vector<RawIntradayRecord>& raw_intraday_records);
}
