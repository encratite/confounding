#pragma once

#include <string>
#include <optional>
#include <set>

#include "confounding/exports.h"
#include "confounding/globex.h"
#include "confounding/types.h"

namespace confounding {
	typedef std::optional<std::set<char>> FilterMonths;

	struct CONFOUNDING_API ContractFilter {
		bool include_record(const Date& time, const GlobexCode& globex_code) const;

		std::string barchart_symbol;
		std::string exchange_symbol;
		std::optional<unsigned> f_records_limit;
		bool enable_fy_records;
		std::optional<GlobexCode> legacy_cutoff;
		std::optional<GlobexCode> first_filter_contract;
		std::optional<GlobexCode> last_filter_contract;
		FilterMonths include_months;
		FilterMonths exclude_months;
		std::optional<Date> cutoff_date;
		TimeOfDay session_end;
		std::optional<TimeOfDay> liquid_hours_start;
		std::optional<TimeOfDay> liquid_hours_end;
		bool features_only;
	};
}