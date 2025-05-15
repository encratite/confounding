#pragma once

#include <string>
#include <vector>

#include "contract.h"

namespace confounding {
	class ContractConfiguration {
	public:
		ContractConfiguration();
		static const ContractConfiguration& get();

	private:
		bool _initialized;
		std::vector<Contract> _contracts;

		void load();
	};
}