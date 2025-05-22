#include <algorithm>
#include <execution>
#include <filesystem>
#include <format>
#include <map>
#include <deque>

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
		constexpr std::size_t momentum_window_size = 40;
		constexpr std::chrono::hours min_session_end_offset(8);
		constexpr double close_minimum = 0.001;
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
		auto intraday_records = read_intraday_records(symbol, filter);
		unsigned f_number_limit = default_f_records_limit;
		if (filter.f_records_limit)
			f_number_limit = *filter.f_records_limit;
		for (unsigned f_number = 1; f_number <= f_number_limit; f_number++)
			generate_archive(f_number, false, symbol, daily_records, intraday_records, filter);
		if (filter.enable_fy_records)
			generate_archive(std::nullopt, true, symbol, daily_records, intraday_records, filter);
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

	IntradayRecordMap read_intraday_records(const std::string& symbol, const ContractFilter& filter) {
		const std::string& path = get_symbol_path(symbol, "H1");
		io::CSVReader<3> csv(path);
		csv.read_header(io::ignore_extra_column, "symbol", "time", "close");
		IntradayClose record;
		std::string globex_string;
		std::string time_string;
		IntradayRecordMap intraday_records;
		auto liquid_hours_start = filter.liquid_hours_start.to_duration();
		auto liquid_hours_end = filter.liquid_hours_end.to_duration();
		while (csv.read_row(
			globex_string,
			time_string,
			record.close)
		) {
			GlobexCode globex_code = globex_string;
			record.time = get_time(time_string);
			auto midnight = std::chrono::floor<std::chrono::days>(record.time);
			auto hours_since_midnight = record.time - midnight;
			if (hours_since_midnight >= liquid_hours_start && hours_since_midnight < liquid_hours_end) {
				Date date = get_date(record.time);
				IntradayRecordsKey key(date, globex_code);
				intraday_records[key].push_back(record);
			}
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
		const IntradayRecordMap& intraday_records,
		const ContractFilter& filter
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
		std::deque<double> recent_closes;
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
			recent_closes.push_front(daily_globex_record.close);
			while (recent_closes.size() > momentum_window_size)
				recent_closes.pop_back();
			IntradayRecordsKey key(date, daily_globex_record.globex_code);
			auto iterator = intraday_records.find(key);
			if (
				iterator == intraday_records.begin() ||
				iterator == intraday_records.end()
			) {
				// There are typically fewer intraday records than daily records available anyway, skip it
				continue;
			}
			if (recent_closes.size() < momentum_window_size) {
				// Can't calculate all momentum/volatility features yet
				continue;
			}
			const auto& today = iterator->second;
			// Collect local intraday closes around the day we are currently processing,
			// in a period ranging from the previous day to three days after today
			std::size_t intraday_closes_reserve = (intraday_max_holding_days + 1) * hours_per_day;
			std::vector<IntradayClose> intraday_closes(intraday_closes_reserve);
			iterator--;
			for (int i = -1; i < intraday_max_holding_days; i++) {
				if (iterator == intraday_records.end())
					throw Exception("Unable to calculate all returns for symbol {} at {} due to missing data", symbol, get_date_string(date));
				const auto& records = iterator->second;
				std::ranges::copy(records, std::back_inserter(intraday_closes));
				iterator++;
			}
			for (const auto& record : today) {
				std::chrono::local_days local_days{ date };
				Time close_time = local_days + std::chrono::duration_cast<std::chrono::hours>(filter.session_end.to_duration());
				bool use_today = record.time > close_time + min_session_end_offset;
				std::size_t recent_closes_offset = use_today ? 0 : 1;
				double close = record.close;
				auto get_recent_close = [&](std::size_t i) {
					return recent_closes[recent_closes_offset + i];
				};
				double close_1d = get_recent_close(0);
				double close_2d = get_recent_close(1);
				double close_10d = get_recent_close(9);
				double close_40d = get_recent_close(39);
				if (
					close < close_minimum ||
					close_1d < close_minimum ||
					close_2d < close_minimum ||
					close_10d < close_minimum ||
					close_40d < close_minimum
				) {
					// At least one of the recent values reached pathologically low values that will grossly distort ratios
					// Just skip all of these abnormal values
					continue;
				}
				double momentum_1d = get_rate_of_change(close, close_1d);
				double momentum_2d = get_rate_of_change(close, close_2d);
				double momentum_2d_gap = get_rate_of_change(close_1d, close_2d);
				double momentum_10d = get_rate_of_change(close, close_10d);
				double momentum_40d = get_rate_of_change(close, close_40d);
				auto intraday_record = IntradayRecord{
					.momentum_1d = momentum_1d,
					.momentum_2d = momentum_2d,
					.momentum_2d_gap = momentum_2d_gap,
					// Missing: momentum_8h
					.momentum_10d = momentum_10d,
					.momentum_40d = momentum_40d,
					// Missing: all volatility values
					// Missing: all return values
				};
				archive.intraday_records.push_back(intraday_record);
			}
		}
		throw Exception("Not implemented: archive output");
	}
}