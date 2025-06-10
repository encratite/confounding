#pragma once

#include <string>

#include "confounding/common.h"

namespace confounding {
	struct CONFOUNDING_API Contract {
		std::string symbol;
		std::string name;
		std::string currency;
		Money tick_size;
		Money tick_value;
		Money margin;
		Money broker_fee;
		Money exchange_fee;
		unsigned spread;
	};
}