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

	typedef std::map<Date, std::vector<GlobexRecord>> GlobexRecordMap;

	void parse_futures_all();
	void parse_futures_single(const Contract& symbol);
	void generate_archive(
		std::optional<unsigned> f_number,
		bool fy_record,
		const std::string& symbol,
		const GlobexRecordMap& daily_records
	);
}