#pragma once

#include <string>

namespace confounding {
	struct Contract {
		std::string symbol;
		std::string name;
		std::string currency;
		double tick_size;
		double tick_value;
		double margin;
		double broker_fee;
		double exchange_fee;
		unsigned spread;
	};
}