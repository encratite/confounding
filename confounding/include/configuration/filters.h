#pragma once

#include <vector>

#include <yaml-cpp/yaml.h>

#include "filter.h"

namespace confounding {
	class ContractFilterConfiguration {
	public:
		ContractFilterConfiguration();
		static const ContractFilterConfiguration& get();

	private:
		bool _initialized;
		std::vector<ContractFilter> _filters;

		void load();
		static FilterMonths get_filter_months(const std::string& name, const YAML::Node& entry);
	};
}