#pragma once

#include <string>

#include "common.h"
#include "contract.h"
#include "filter.h"
#include "types.h"
#include "globex.h"
#include "archive.h"

namespace confounding {
	struct GlobexRecord {
		GlobexCode globex_code;
		Date date;
		Money close;
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
		Money close;
	};

	typedef std::map<Date, std::vector<GlobexRecord>> GlobexRecordMap;
	typedef std::map<IntradayRecordsKey, std::vector<IntradayClose>> IntradayRecordMap;

	class ArchiveGenerator {
	public:
		ArchiveGenerator(
			std::optional<unsigned> f_number,
			bool fy_record,
			const std::string& symbol,
			const GlobexRecordMap& daily_records,
			const IntradayRecordMap& intraday_records,
			const ContractFilter& filter,
			const Contract& contract
		);

		static void parse_futures();

		void run();

	private:
		std::optional<unsigned> _f_number;
		bool _fy_record;
		const std::string& _symbol;
		const GlobexRecordMap& _daily_records;
		const IntradayRecordMap& _intraday_records;
		const ContractFilter& _filter;
		const Contract& _contract;
		Archive _archive;
		std::vector<RawIntradayRecord> _raw_intraday_records;
		std::deque<double> _recent_closes;
		std::deque<double> _recent_returns;
		std::vector<IntradayClose> _today_closes;
		std::vector<IntradayClose> _intraday_closes;
		GlobexRecord _globex_today;
		GlobexRecord _globex_tomorrow;

		static void parse_single_contract(const Contract& contract);
		static GlobexRecordMap read_daily_records(const std::string& symbol, const ContractFilter& filter);
		static IntradayRecordMap read_intraday_records(const std::string& symbol, const ContractFilter& filter);
		static std::string get_symbol_path(const std::string& symbol, const std::string& suffix);

		bool get_globex_records(Date reference_date);
		GlobexRecord get_daily_globex_record(
			Date date,
			const std::vector<GlobexRecord>& records
		);
		bool get_intraday_closes();
		void update_recent_closes(const GlobexRecord& daily_globex_record);
		void generate_intraday_record(const IntradayClose& record);
		bool get_features(
			const IntradayClose& record,
			bool use_today,
			RawIntradayRecord& raw_intraday_record
		);
		void get_returns(
			const IntradayClose& record,
			bool use_today,
			RawIntradayRecord& raw_intraday_record
		);
		double get_volatility(std::size_t n);
		void add_nan_record(Time time);
	};
}
