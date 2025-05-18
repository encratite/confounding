#include <algorithm>
#include <execution>
#include <filesystem>
#include <format>
#include <map>

#pragma warning(push)
#pragma warning(disable: 4267 4244)
#include <fast-cpp-csv-parser/csv.h>
#pragma warning(pop)

#include "yaml.h"
#include "parser.h"
#include "configuration/base.h"
#include "configuration/contracts.h"
#include "configuration/filters.h"
#include "common.h"
#include "archive.h"

namespace confounding {
	namespace {
		constexpr unsigned default_f_records_limit = 3;
	}

	void parse_futures_all() {
		const auto& contract_configuration = ContractConfiguration::get();
		std::for_each(
			std::execution::par,
			contract_configuration.begin(),
			contract_configuration.end(),
			[](const Contract& contract) {
				parse_futures_single(contract);
			});
	}

	void parse_futures_single(const Contract& contract) {
		const std::string& symbol = contract.symbol;
		const auto& configuration = Configuration::get();
		const auto& filter_configuration = ContractFilterConfiguration::get();
		const auto& filter = filter_configuration.get_filter(symbol);
		Path barchart_path = configuration.barchart_directory;
		Path filename = std::format("{}.D1.csv", symbol);
		Path path = barchart_path / filename;
		io::CSVReader<4> csv(path.string());
		csv.read_header(io::ignore_extra_column, "symbol", "time", "close", "open_interest");
		GlobexRecord record;
		std::string globex_string;
		std::string date_string;
		GlobexRecordMap daily_records;
		while (csv.read_row(
			globex_string,
			date_string,
			record.close,
			record.open_interest)) {
			record.globex_code = GlobexCode(globex_string);
			record.date = get_date(date_string);
			daily_records[record.date].push_back(record);
		}
		for (auto& [date, records] : daily_records) {
			std::sort(records.begin(), records.end(), [](const GlobexRecord& a, const GlobexRecord& b) {
				return a.globex_code > b.globex_code;
			});
		}
		unsigned f_number_limit = default_f_records_limit;
		const auto& filter_limit = filter.f_records_limit();
		if (filter_limit)
			f_number_limit = *filter_limit;
		for (unsigned f_number = 1; f_number <= f_number_limit; f_number++)
			generate_archive(f_number, false, symbol, daily_records);
		if (filter.enable_fy_records())
			generate_archive(std::nullopt, true, symbol, daily_records);
		throw Exception("Not implemented: missing intraday data");
	}

	void generate_archive(
		std::optional<unsigned> f_number,
		bool fy_record,
		const std::string& symbol,
		const GlobexRecordMap& daily_records
	) {
		Archive archive;
		std::map<Date, GlobexRecord> selected_daily_records;
		if (f_number) {
			for (const auto& [date, records] : daily_records) {
				std::size_t index = static_cast<std::size_t>(*f_number) - 1;
				if (index >= records.size()) {
					std::string message = std::format("Symbol {} lacks an F{} record at {:%F}", symbol, *f_number, date);
					throw Exception(message);
				}
			}
		}
		throw Exception("Not implemented: archive output");
	}
}