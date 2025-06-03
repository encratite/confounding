#pragma once

#include <string>

#include "common.h"

namespace confounding {
	struct Contract {
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