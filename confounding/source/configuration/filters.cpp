
#include <mutex>
#include <ranges>
#include <format>

#include "yaml.h"
#include "configuration/filters.h"
#include "exception.h"
#include "common.h"

namespace confounding {
	namespace {
		constexpr const char* configuration_file = "filters.yaml";

		std::mutex mutex;
	}

	ContractFilterConfiguration::ContractFilterConfiguration()
		: _initialized(false) {
	}

	const ContractFilterConfiguration& ContractFilterConfiguration::get() {
		std::lock_guard<std::mutex> lock(mutex);
		static ContractFilterConfiguration configuration;
		configuration.load();
		return configuration;
	}

	const ContractFilter& ContractFilterConfiguration::get_filter(const std::string& symbol) const {
		auto iterator = std::ranges::find_if(_filters, [&](const ContractFilter& filter) {
			return filter.exchange_symbol == symbol;
		});
		if (iterator == _filters.end())
			throw Exception("Unable to find a contract filter matching symbol \"{}\"", symbol);
		return *iterator;
	}

	void ContractFilterConfiguration::load() {
		using namespace YAML;
		if (_initialized)
			return;
		YAML::Node doc = YAML::LoadFile(configuration_file);
		if (!doc.IsSequence())
			throw Exception("The contract filter configuration file must consist of a sequence at the top level");
		for (const auto& entry : doc) {
			auto barchart_symbol = entry["barchart_symbol"].as<std::string>();
			auto exchange_symbol = entry["exchange_symbol"].as<std::optional<std::string>>();
			if (!exchange_symbol)
				exchange_symbol = barchart_symbol;
			auto f_records_limit = entry["f_records_limit"].as<std::optional<unsigned>>();
			auto enable_fy_records_opt = entry["enable_fy_records"].as<std::optional<bool>>();
			bool enable_fy_records = true;
			if (enable_fy_records_opt)
				enable_fy_records = *enable_fy_records_opt;
			auto legacy_cutoff = entry["legacy_cutoff"].as<std::optional<GlobexCode>>();
			auto first_filter_contract = entry["first_filter_contract"].as<std::optional<GlobexCode>>();
			auto last_filter_contract = entry["last_filter_contract"].as<std::optional<GlobexCode>>();
			auto include_months = get_filter_months("include_months", entry);
			auto exclude_months = get_filter_months("exclude_months", entry);
			std::optional<Date> cutoff_date;
			auto cutoff_date_entry = entry["cutoff_date"];
			auto session_end = get_time_of_day("session_end", entry);
			auto liquid_hours_start = get_time_of_day("liquid_hours_start", entry);
			auto liquid_hours_end = get_time_of_day("liquid_hours_end", entry);
			if (cutoff_date_entry) {
				std::string cutoff_date_string = cutoff_date_entry.as<std::string>();
				cutoff_date = get_date(cutoff_date_string);
			}
			ContractFilter filter{
				.barchart_symbol = barchart_symbol,
				.exchange_symbol = *exchange_symbol,
				.f_records_limit = f_records_limit,
				.enable_fy_records = enable_fy_records,
				.legacy_cutoff = legacy_cutoff,
				.first_filter_contract = first_filter_contract,
				.last_filter_contract = last_filter_contract,
				.include_months = include_months,
				.exclude_months = exclude_months,
				.cutoff_date = cutoff_date,
				.session_end = session_end,
				.liquid_hours_start = liquid_hours_start,
				.liquid_hours_end = liquid_hours_end
			};
			_filters.push_back(std::move(filter));
		}
	}

	FilterMonths ContractFilterConfiguration::get_filter_months(const std::string& name, const YAML::Node& entry) {
		auto months = entry[name];
		if (months) {
			auto vector = months.as<std::vector<char>>();
			std::set<char> set(vector.begin(), vector.end());
			return set;
		}
		else
			return std::nullopt;
	}

	TimeOfDay ContractFilterConfiguration::get_time_of_day(const std::string& key, const YAML::iterator::value_type& entry) {
		std::string time_of_day_string = entry[key].as<std::string>();
		TimeOfDay time_of_day = confounding::get_time_of_day(time_of_day_string);
		return time_of_day;
	}
}