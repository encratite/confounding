#include "filter.h"
#include "common.h"

namespace confounding {
	bool ContractFilter::include_record(const Date& time, const GlobexCode& globex_code) const {
		if (cutoff_date && time < *cutoff_date)
			return false;
		if (legacy_cutoff && globex_code < *legacy_cutoff)
			return false;
		if (first_filter_contract && last_filter_contract) {
			if (
				globex_code < *first_filter_contract ||
				globex_code >= *last_filter_contract)
				return true;
		}
		else if (first_filter_contract && globex_code < *first_filter_contract)
			return true;
		else if (last_filter_contract && globex_code >= *last_filter_contract)
			return true;
		if (include_months)
			return include_months->contains(*globex_code.month);
		else if (exclude_months)
			return exclude_months->contains(*globex_code.month);
		return true;
	}
}