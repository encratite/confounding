#include <algorithm>
#include <execution>
#include <filesystem>
#include <format>
#include <map>
#include <deque>
#include <ranges>
#include <math.h>

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

namespace confounding {
	namespace {
		constexpr unsigned default_f_records_limit = 3;
		constexpr unsigned hours_per_day = 24;
		constexpr Date first_intraday_date{std::chrono::year{2008}, std::chrono::month{1}, std::chrono::day{1}};
		constexpr int intraday_max_holding_days = 3;
		constexpr std::size_t recent_closes_window_size = 40;
		constexpr std::size_t recent_returns_window_size = recent_closes_window_size;
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
			&parse_futures_single
		);
	}

	void parse_futures_single(const Contract& contract) {
		const std::string& symbol = contract.symbol;
		const auto& filter_configuration = ContractFilterConfiguration::get();
		const auto& filter = filter_configuration.get_filter(symbol);
		const auto& contract_configuration = ContractConfiguration::get();
		auto daily_records = read_daily_records(symbol, filter);
		auto intraday_records = read_intraday_records(symbol, filter);
		unsigned f_number_limit = default_f_records_limit;
		if (filter.f_records_limit)
			f_number_limit = *filter.f_records_limit;
		for (unsigned f_number = 1; f_number <= f_number_limit; f_number++)
			generate_archive(f_number, false, symbol, daily_records, intraday_records, filter, contract);
		if (filter.enable_fy_records)
			generate_archive(std::nullopt, true, symbol, daily_records, intraday_records, filter, contract);
		throw Exception("Not implemented: missing intraday data");
	}

	GlobexRecordMap read_daily_records(const std::string& symbol, const ContractFilter& filter) {
		const std::string& path = get_symbol_path(symbol, "D1");
		io::CSVReader<4> csv(path);
		csv.read_header(io::ignore_extra_column, "symbol", "time", "close", "open_interest");
		GlobexRecord record;
		std::string globex_string;
		std::string date_string;
		std::string close_string;
		GlobexRecordMap daily_records;
		while (csv.read_row(
			globex_string,
			date_string,
			close_string,
			record.open_interest)
		) {
			record.globex_code = GlobexCode(globex_string);
			record.date = get_date(date_string);
			record.close = Money(close_string);
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
		std::string close_string;
		IntradayRecordMap intraday_records;
		auto liquid_hours_start = filter.liquid_hours_start.to_duration();
		auto liquid_hours_end = filter.liquid_hours_end.to_duration();
		while (
			csv.read_row(
				globex_string,
				time_string,
				close_string
			)
		) {
			GlobexCode globex_code = globex_string;
			record.time = get_time(time_string);
			record.close = Money(close_string);
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
		const ContractFilter& filter,
		const Contract& contract
	) {
		std::vector<RawIntradayRecord> raw_intraday_records;
		Archive archive;
		// Allocate more memory than necessary for the H1 intraday records without shrinking them
		// at the end of the function because the archive will be freed anyway
		archive.daily_records.reserve(daily_records.size());
		const auto& last_date = daily_records.rbegin()->first;
		auto days1 = std::chrono::sys_days{first_intraday_date};
		auto days2 = std::chrono::sys_days{last_date};
		std::chrono::days days_diff = days2 - days1;
		std::size_t daily_records_reserve = days_diff.count();
		std::size_t intraday_records_reserve = hours_per_day * daily_records_reserve;
		raw_intraday_records.reserve(intraday_records_reserve);
		std::map<Date, GlobexRecord> selected_daily_records;
		std::deque<double> recent_closes;
		std::deque<double> recent_returns;
		const auto& configuration = Configuration::get();
		auto reference_date = configuration.intraday_reference_date;
		auto add_nan_records = [&]() {
			for (unsigned i = 0; i < hours_per_day; i++)
				add_nan_record(raw_intraday_records);
		};
		for (; reference_date <= last_date; add_day(reference_date)) {
			// Health check
			if (raw_intraday_records.size() % hours_per_day != 0)
				throw Exception("Invalid number of records in raw_intraday_records: {}", raw_intraday_records.size());
			auto weekday = std::chrono::weekday{reference_date};
			if (weekday == std::chrono::Saturday || weekday == std::chrono::Sunday) {
				// Skipping weekends like this isn't entirely correct since CME futures actually do have intraday records
				// for Sundays but they're on the low liquidity side so it shouldn't hurt much
				continue;
			}
			auto daily_iterator = daily_records.find(reference_date);
			if (daily_iterator == daily_records.end()) {
				// Could be a holiday, generate NaN records
				add_nan_records();
				continue;
			}
			const auto& [date, records] = *daily_iterator;
			GlobexRecord daily_globex_record = get_daily_globex_record(
				reference_date,
				f_number,
				fy_record,
				symbol,
				records,
				archive
			);
			update_recent_closes(daily_globex_record, recent_closes, recent_returns);
			if (recent_closes.size() < recent_closes_window_size) {
				// Can't calculate all momentum/volatility features yet, generate NaN records
				add_nan_records();
				continue;
			}
			IntradayRecordsKey key(date, daily_globex_record.globex_code);
			auto intraday_iterator = intraday_records.find(key);
			if (
				intraday_iterator == intraday_records.begin() ||
				intraday_iterator == intraday_records.end()
				) {
				// There are typically fewer intraday records than daily records available anyway, skip it
				add_nan_records();
				continue;
			}
			const auto& today = intraday_iterator->second;
			// Collect local intraday closes around the day we are currently processing,
			// in a period ranging from the previous day to three days after today
			std::size_t intraday_closes_reserve = (intraday_max_holding_days + 1) * hours_per_day;
			std::vector<IntradayClose> intraday_closes(intraday_closes_reserve);
			intraday_iterator--;
			for (int i = -1; i < intraday_max_holding_days; i++) {
				if (intraday_iterator == intraday_records.end())
					throw Exception("Unable to calculate all returns for symbol {} at {} due to missing data", symbol, get_date_string(date));
				const auto& records = intraday_iterator->second;
				std::ranges::copy(records, std::back_inserter(intraday_closes));
				intraday_iterator++;
			}
			Time reference_time = get_time(reference_date);
			for (const auto& record : today) {
				while (reference_time < record.time) {
					// Fill the gaps in the intraday data with NaN records
					add_nan_record(raw_intraday_records);
					reference_time += std::chrono::hours{1};
				}
				generate_intraday_record(
					record,
					date,
					intraday_closes,
					recent_closes,
					recent_returns,
					raw_intraday_records,
					archive,
					filter,
					contract
				);
			}
		}
		throw Exception("Not implemented: archive output");
	}

	GlobexRecord get_daily_globex_record(
		Date date,
		std::optional<unsigned> f_number,
		bool fy_record,
		const std::string& symbol,
		const std::vector<GlobexRecord>& records,
		Archive& archive
	) {
		GlobexRecord daily_globex_record;
		if (f_number) {
			std::size_t index = static_cast<std::size_t>(*f_number) - 1;
			if (index >= records.size()) {
				// Use get_date_string rather than {%F} to work around an MSVC bug
				// that erroneously spits out errors, despite the code compiling just fine
				throw Exception("Symbol {} lacks a daily F{} record at {}", symbol, *f_number, get_date_string(date));
			}
			daily_globex_record = records[index];
		}
		else if (fy_record) {
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
		return daily_globex_record;
	}

	void update_recent_closes(
		const GlobexRecord& daily_globex_record,
		std::deque<double>& recent_closes,
		std::deque<double>& recent_returns
	) {
		recent_closes.push_front(daily_globex_record.close.to_double());
		while (recent_closes.size() > recent_closes_window_size)
			recent_closes.pop_back();
		if (recent_closes.size() >= 2) {
			double close1 = recent_closes[0];
			double close2 = recent_closes[1];
			if (close1 > close_minimum && close2 > close_minimum) {
				double returns = get_rate_of_change(close1, close2);
				recent_returns.push_front(returns);
				while (recent_returns.size() > recent_returns_window_size)
					recent_returns.pop_back();
			}
		}
	}

	void generate_intraday_record(
		const IntradayClose& record,
		Date date,
		const std::vector<IntradayClose>& intraday_closes,
		const std::deque<double>& recent_closes,
		const std::deque<double>& recent_returns,
		std::vector<RawIntradayRecord>& raw_intraday_records,
		Archive& archive,
		const ContractFilter& filter,
		const Contract& contract
	) {
		std::chrono::local_days local_days{date};
		Time close_time = local_days + std::chrono::duration_cast<std::chrono::hours>(filter.session_end.to_duration());
		bool use_today = record.time > close_time + min_session_end_offset;
		std::size_t recent_closes_offset = use_today ? 0 : 1;
		double close = record.close.to_double();
		auto get_recent_close = [&](std::size_t i) {
			return recent_closes[recent_closes_offset + i];
		};
		double close_1d = get_recent_close(0);
		double close_2d = get_recent_close(1);
		double close_10d = get_recent_close(9);
		double close_40d = get_recent_close(39);
		auto time_8h = record.time - std::chrono::hours(8);
		auto intraday_iterator = std::find_if(intraday_closes.begin(), intraday_closes.end(), [&](const IntradayClose& intraday_close) {
			return intraday_close.time == time_8h;
		});
		if (intraday_iterator == intraday_closes.end()) {
			// The intraday buffer lacks a corresponding value for that offset
			// Could be the result of daily maintenance, but skip it either way
			add_nan_record(raw_intraday_records);
			return;
		}
		double close_8h = intraday_iterator->close.to_double();
		if (
			close < close_minimum ||
			close_1d < close_minimum ||
			close_2d < close_minimum ||
			close_10d < close_minimum ||
			close_40d < close_minimum ||
			close_8h < close_minimum
		) {
			// At least one of the recent values reached pathologically low values that will grossly distort ratios
			// Just skip all of these abnormal values
			add_nan_record(raw_intraday_records);
			return;
		}
		double momentum_1d = get_rate_of_change(close, close_1d);
		double momentum_2d = get_rate_of_change(close, close_2d);
		double momentum_2d_gap = get_rate_of_change(close_1d, close_2d);
		double momentum_8h = get_rate_of_change(close, close_2d);
		double momentum_10d = get_rate_of_change(close, close_10d);
		double momentum_40d = get_rate_of_change(close, close_40d);
		double volatility_10d = get_volatility(10, recent_returns);
		double volatility_40d = get_volatility(40, recent_returns);
		auto intraday_record = RawIntradayRecord{
			.momentum_1d = momentum_1d,
			.momentum_2d = momentum_2d,
			.momentum_2d_gap = momentum_2d_gap,
			.momentum_8h = momentum_8h,
			.momentum_10d = momentum_10d,
			.momentum_40d = momentum_40d,
			.volatility_10d = volatility_10d,
			.volatility_40d = volatility_40d,
			// Missing: all return values
		};
		if (raw_intraday_records.size() == 0)
			archive.first_intraday_time = record.time;
		raw_intraday_records.push_back(intraday_record);
	}

	double get_volatility(std::size_t n, const std::deque<double>& recent_returns) {
		double mean_sum = 0.0;
		auto view = std::views::take(n);
		for (double x : recent_returns | view)
			mean_sum += x;
		double mean = mean_sum / n;
		double delta_sum = 0.0;
		for (double x : recent_returns | view) {
			double delta = x - mean;
			delta_sum += delta * delta;
		}
		double standard_deviation = delta_sum / (n - 1);
		double volatility = std::sqrt(static_cast<double>(n)) * standard_deviation;
		return volatility;
	}

	void add_nan_record(std::vector<RawIntradayRecord>& raw_intraday_records) {
		if (raw_intraday_records.size() == 0)
			return;
		constexpr float nan = std::numeric_limits<float>::signaling_NaN();
		auto intraday_record = RawIntradayRecord{
			.momentum_1d = nan,
			.momentum_2d = nan,
			.momentum_2d_gap = nan,
			.momentum_8h = nan,
			.momentum_10d = nan,
			.momentum_40d = nan,
			.volatility_10d = nan,
			.volatility_40d = nan,
			.returns_next_close_long = 0,
			.returns_next_close_short = 0,
			.returns_24h_long = 0,
			.returns_24h_short = 0,
			.returns_48h_long = 0,
			.returns_48h_short = 0,
			.returns_72h_long = 0,
			.returns_72h_short = 0
		};
		raw_intraday_records.push_back(intraday_record);
	}
}