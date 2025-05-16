#include <algorithm>
#include <execution>
#include <filesystem>
#include <format>
#include <map>

#include <fast-cpp-csv-parser/csv.h>

#include "parser.h"
#include "configuration/base.h"
#include "configuration/contracts.h"
#include "common.h"
#include "archive.h"

namespace confounding {
	void parse_futures_all() {
		const auto& contract_configuration = ContractConfiguration::get();
		std::for_each(
			std::execution::par,
			contract_configuration.begin(),
			contract_configuration.end(),
			[](const Contract& contract) {
				parse_futures_single(contract.symbol);
			});
	}

	void parse_futures_single(const std::string& symbol) {
		const auto& configuration = Configuration::get();
		Path barchart_path = configuration.barchart_directory;
		Path filename = std::format("{}.D1.csv", symbol);
		Path path = barchart_path / filename;
		io::CSVReader<4> csv(path.string());
		csv.read_header(io::ignore_extra_column, "symbol", "time", "close", "open_interest");
		GlobexRecord record;
		std::string globex_string;
		std::string date_string;
		std::map<Date, std::vector<GlobexRecord>> records;
		while (csv.read_row(
			globex_string,
			date_string,
			record.close,
			record.open_interest)) {
			record.globex_code = GlobexCode(globex_string);
			record.date = get_date(date_string);
			records[record.date].push_back(record);
		}
		for (auto& [key, value] : records) {
			std::sort(value.begin(), value.end(), [](const GlobexRecord& a, const GlobexRecord& b) {
				return a.globex_code > b.globex_code;
			});
		}
		Archive archive;
	}
}