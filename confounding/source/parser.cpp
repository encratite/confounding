#include <string>

#include <fast-cpp-csv-parser/csv.h>

#include "configuration/base.h"

namespace confounding {
	void parse_futures_all() {
	}

	void parse_futures_single(const std::string& symbol) {
		auto configuration = Configuration::get();
	}
}