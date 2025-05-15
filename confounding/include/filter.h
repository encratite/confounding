#pragma once

#include <string>
#include <optional>
#include <set>

#include "globex.h"
#include "types.h"

namespace confounding {
	typedef std::optional<std::set<char>> FilterMonths;

	class ContractFilter {
	public:
		ContractFilter(
			std::optional<std::string> exchange_symbol,
			std::optional<unsigned> f_records_limit,
			bool enable_fy_records,
			std::optional<GlobexCode> legacy_cutoff,
			std::optional<GlobexCode> first_filter_contract,
			std::optional<GlobexCode> last_filter_contract,
			FilterMonths include_months,
			FilterMonths exclude_months,
			std::optional<Date> cutoff_date
		);
		bool include_record(const Time& time, const GlobexCode& globex_code) const;

	private:
		std::optional<std::string> _exchange_symbol;
		std::optional<unsigned> _f_records_limit;
		bool _enable_fy_records;
		std::optional<GlobexCode> _legacy_cutoff;
		std::optional<GlobexCode> _first_filter_contract;
		std::optional<GlobexCode> _last_filter_contract;
		FilterMonths _include_months;
		FilterMonths _exclude_months;
		std::optional<Date> _cutoff_date;
	};
}