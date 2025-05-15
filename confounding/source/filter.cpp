#include "filter.h"
#include "common.h"

namespace confounding {
	ContractFilter::ContractFilter(
		std::optional<std::string> exchange_symbol,
		std::optional<unsigned> f_records_limit,
		bool enable_fy_records,
		std::optional<GlobexCode> legacy_cutoff,
		std::optional<GlobexCode> first_filter_contract,
		std::optional<GlobexCode> last_filter_contract,
		FilterMonths include_months,
		FilterMonths exclude_months,
		std::optional<Date> cutoff_date
	)
		: _exchange_symbol(std::move(exchange_symbol)),
		_f_records_limit(std::move(f_records_limit)),
		_enable_fy_records(enable_fy_records),
		_legacy_cutoff(std::move(legacy_cutoff)),
		_first_filter_contract(std::move(first_filter_contract)),
		_last_filter_contract(std::move(last_filter_contract)),
		_include_months(std::move(include_months)),
		_exclude_months(std::move(exclude_months)),
		_cutoff_date(std::move(cutoff_date)) {
	}

	bool ContractFilter::include_record(const Time& time, const GlobexCode& globex_code) const {
		if (_cutoff_date && time < *_cutoff_date)
			return false;
		if (_legacy_cutoff && globex_code < *_legacy_cutoff)
			return false;
		if (_first_filter_contract && _last_filter_contract) {
			if (
				globex_code < *_first_filter_contract ||
				globex_code >= *_last_filter_contract)
				return true;
		}
		else if (_first_filter_contract && globex_code < *_first_filter_contract)
			return true;
		else if (_last_filter_contract && globex_code >= *_last_filter_contract)
			return true;
		if (_include_months)
			return _include_months->contains(*globex_code.month);
		else if (_exclude_months)
			return _exclude_months->contains(*globex_code.month);
		return true;
	}
}