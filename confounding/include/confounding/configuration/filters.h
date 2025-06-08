#pragma once

#include <vector>

#include <yaml-cpp/yaml.h>

#include "confounding/filter.h"
#include "confounding/types.h"

namespace confounding {
	class ContractFilterConfiguration {
	public:
		ContractFilterConfiguration();

		static const ContractFilterConfiguration& get();

		const ContractFilter& get_filter(const std::string& symbol) const;

	private:
		bool _initialized;
		std::vector<ContractFilter> _filters;

		void load();
		static FilterMonths get_filter_months(const std::string& name, const YAML::Node& entry);
		static TimeOfDay get_time_of_day(const std::string& key, const YAML::iterator::value_type& entry);
	};
}