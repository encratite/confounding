#pragma once

#include <string>

#include "types.h"
#include "globex.h"

namespace confounding {
	struct GlobexRecord {
		GlobexCode globex_code;
		Date date;
		double close;
		unsigned open_interest;
	};

	void parse_futures_all();
	void parse_futures_single(const std::string& symbol);
}