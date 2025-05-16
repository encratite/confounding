#pragma once

#include <string>
#include <vector>

#include "contract.h"

namespace confounding {
	class ContractConfiguration {
	public:
		ContractConfiguration();
		static const ContractConfiguration& get();
		std::vector<Contract>::const_iterator begin() const;
		std::vector<Contract>::const_iterator end() const;

	private:
		bool _initialized;
		std::vector<Contract> _contracts;

		void load();
	};
}