
#include <mutex>
#include <ranges>
#include <format>

#include "confounding/yaml.h"
#include "confounding/configuration/filters.h"
#include "confounding/exception.h"
#include "confounding/common.h"

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
			auto barchart_symbol = entry.as<std::string>();
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
			if (cutoff_date_entry) {
				std::string cutoff_date_string = cutoff_date_entry.as<std::string>();
				cutoff_date = get_date(cutoff_date_string);
			}
			auto session_end_opt = get_time_of_day("session_end", entry);
			if (session_end_opt == std::nullopt)
				throw Exception("Session end missing for symbol {}", barchart_symbol);
			auto session_end = *session_end_opt;
			auto liquid_hours_start = get_time_of_day("liquid_hours_start", entry);
			auto liquid_hours_end = get_time_of_day("liquid_hours_end", entry);
			if (liquid_hours_start.has_value() != liquid_hours_end.has_value())
				throw Exception("Invalid combination of liquid_hours_start and liquid_hours_end for symbol {}", barchart_symbol);
			auto features_only_entry = entry["features_only"];
			bool features_only = false;
			if (features_only_entry)
				features_only = features_only_entry.as<bool>();
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
				.liquid_hours_end = liquid_hours_end,
				.features_only = features_only,
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

	std::optional<TimeOfDay> ContractFilterConfiguration::get_time_of_day(const std::string& key, const YAML::iterator::value_type& entry) {
		auto node = entry[key];
		if (node) {
			std::string time_of_day_string = node.as<std::string>();
			TimeOfDay time_of_day = confounding::get_time_of_day(time_of_day_string);
			return time_of_day;
		} else
			return std::nullopt;
	}
}