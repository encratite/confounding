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
		constexpr unsigned hours_per_day = 24;
		constexpr Date first_intraday_date{ std::chrono::year{2008}, std::chrono::month{1}, std::chrono::day{1} };
		constexpr int intraday_max_holding_days = 3;
	}

	IntradayRecordsKey::IntradayRecordsKey(Date date, GlobexCode globex_code)
		: date(date),
		globex_code(globex_code) {
	}

	bool IntradayRecordsKey::operator<(const IntradayRecordsKey& key) const {
		return
			date < key.date &&
			globex_code < key.globex_code;
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
		const auto& filter_configuration = ContractFilterConfiguration::get();
		const auto& filter = filter_configuration.get_filter(symbol);
		auto daily_records = read_daily_records(symbol, filter);
		auto intraday_records = read_intraday_records(symbol);
		unsigned f_number_limit = default_f_records_limit;
		if (filter.f_records_limit)
			f_number_limit = *filter.f_records_limit;
		for (unsigned f_number = 1; f_number <= f_number_limit; f_number++)
			generate_archive(f_number, false, symbol, daily_records, intraday_records);
		if (filter.enable_fy_records)
			generate_archive(std::nullopt, true, symbol, daily_records, intraday_records);
		throw Exception("Not implemented: missing intraday data");
	}

	GlobexRecordMap read_daily_records(const std::string& symbol, const ContractFilter& filter) {
		const std::string& path = get_symbol_path(symbol, "D1");
		io::CSVReader<4> csv(path);
		csv.read_header(io::ignore_extra_column, "symbol", "time", "close", "open_interest");
		GlobexRecord record;
		std::string globex_string;
		std::string date_string;
		GlobexRecordMap daily_records;
		while (csv.read_row(
			globex_string,
			date_string,
			record.close,
			record.open_interest)
		) {
			record.globex_code = GlobexCode(globex_string);
			record.date = get_date(date_string);
			// Only include contracts that are sufficiently liquid and feature volume/open interest data
			// Most Barchart futures data from prior to 2006 has to be filtered out
			if (filter.include_record(record.date, record.globex_code))
				daily_records[record.date].push_back(record);
		}
		for (auto& [date, records] : daily_records) {
			std::sort(
				records.begin(),
				records.end(),
				[](const GlobexRecord& a, const GlobexRecord& b) {
					return a.globex_code > b.globex_code;
				});
		}
		return std::move(daily_records);
	}

	IntradayRecordMap read_intraday_records(const std::string& symbol) {
		const std::string& path = get_symbol_path(symbol, "H1");
		io::CSVReader<3> csv(path);
		csv.read_header(io::ignore_extra_column, "symbol", "time", "close");
		IntradayClose record;
		std::string globex_string;
		std::string time_string;
		IntradayRecordMap intraday_records;
		while (csv.read_row(
			globex_string,
			time_string,
			record.close)
		) {
			GlobexCode globex_code = globex_string;
			record.time = get_time(time_string);
			Date date = get_time_from_date(record.time);
			IntradayRecordsKey key(date, globex_code);
			intraday_records[key].push_back(record);
		}
		return std::move(intraday_records);
	}

	std::string get_symbol_path(const std::string& symbol, const std::string& suffix) {
		const auto& configuration = Configuration::get();
		Path barchart_path = configuration.barchart_directory;
		Path filename = std::format("{}.{}.csv", symbol, suffix);
		Path path = barchart_path / filename;
		return path.string();
	}

	void generate_archive(
		std::optional<unsigned> f_number,
		bool fy_record,
		const std::string& symbol,
		const GlobexRecordMap& daily_records,
		const IntradayRecordMap& intraday_records
	) {
		Archive archive;
		// Allocate more memory than necessary for the H1 intraday records without shrinking them
		// at the end of the function because the archive will be freed anyway
		archive.daily_records.reserve(daily_records.size());
		const auto& last_date = daily_records.rbegin()->first;
		std::chrono::sys_days days1 = std::chrono::sys_days{ first_intraday_date };
		std::chrono::sys_days days2 = std::chrono::sys_days{ last_date };
		std::chrono::days days_diff = days2 - days1;
		std::size_t intraday_records_reserve = hours_per_day * days_diff.count();
		archive.intraday_records.reserve(intraday_records_reserve);
		std::map<Date, GlobexRecord> selected_daily_records;
		for (const auto& [date, records] : daily_records) {
			GlobexRecord daily_globex_record;
			if (f_number) {
				std::size_t index = static_cast<std::size_t>(*f_number) - 1;
				if (index >= records.size()) {
					// Use get_date_string rather than {%F} to work around an MSVC bug
					// that erroneously spits out errors, despite the code compiling just fine
					throw Exception("Symbol {} lacks a daily F{} record at {}", symbol, *f_number, get_date_string(date));
				}
				daily_globex_record = records[index];
			} else if (fy_record) {
				const auto& f1_record = records.front();
				GlobexCode globex_code = f1_record.globex_code;
				globex_code.add_year();
				auto iterator = std::find_if(
					records.begin(),
					records.end(),
					[&](const GlobexRecord& record) {
						return record.globex_code == globex_code;
					});
				if (iterator == records.end())
					throw Exception("Symbol {} lacks a daily FY record at {}", symbol, get_date_string(date));
				DailyRecord daily_record = DailyRecord(daily_globex_record.date, daily_globex_record.close);
				daily_globex_record = *iterator;
				archive.daily_records.push_back(daily_record);
			}
			IntradayRecordsKey key(date, daily_globex_record.globex_code);
			auto iterator = intraday_records.find(key);
			if (
				iterator == intraday_records.begin() ||
				iterator == intraday_records.end()
			) {
				// There are typically fewer intraday records than daily records available anyway, skip it
				continue;
			}
			const auto& today = iterator->second;
			// Collect local intraday closes around the day we are currently processing,
			// in a period ranging from the previous day to three days after today
			std::size_t intraday_closes_reserve = (intraday_max_holding_days + 1) * hours_per_day;
			std::vector<IntradayClose> intraday_closes(intraday_closes_reserve);
			iterator--;
			for (int i = -1; i < intraday_max_holding_days; i++) {
				if (iterator == intraday_records.end()) {
					throw Exception("Unable to calculate all returns for symbol {} at {} due to missing data", symbol, get_date_string(date));
				}
				const auto& records = iterator->second;
				std::ranges::copy(records, std::back_inserter(intraday_closes));
				iterator++;
			}
			for (const auto& record : today) {
			}
		}
		throw Exception("Not implemented: archive output");
	}
}