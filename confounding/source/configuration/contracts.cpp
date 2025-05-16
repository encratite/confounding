#include <mutex>

#include <yaml-cpp/yaml.h>

#include "configuration/contracts.h"
#include "exception.h"

namespace confounding {
	namespace {
		constexpr const char* configuration_file = "contracts.yaml";

		std::mutex mutex;
	}

	ContractConfiguration::ContractConfiguration()
		: _initialized(false) {
	}

	const ContractConfiguration& ContractConfiguration::get() {
		std::lock_guard<std::mutex> lock(mutex);
		static ContractConfiguration configuration;
		configuration.load();
		return configuration;
	}

	std::vector<Contract>::const_iterator ContractConfiguration::begin() const {
		return _contracts.cbegin();
	}

	std::vector<Contract>::const_iterator ContractConfiguration::end() const {
		return _contracts.cend();
	}

	void ContractConfiguration::load() {
		if (_initialized)
			return;
		YAML::Node doc = YAML::LoadFile(configuration_file);
		if (!doc.IsSequence())
			throw Exception("The contract configuration file must consist of a sequence at the top level");
		for (const auto& entry : doc) {
			Contract contract{
				.symbol = entry["symbol"].as<std::string>(),
				.name = entry["name"].as<std::string>(),
				.currency = entry["currency"].as<std::string>(),
				.tick_size = entry["tick_size"].as<double>(),
				.tick_value = entry["tick_value"].as<double>(),
				.margin = entry["margin"].as<double>(),
				.broker_fee = entry["broker_fee"].as<double>(),
				.exchange_fee = entry["exchange_fee"].as<double>(),
				.spread = entry["spread"].as<unsigned>(),
			};
		}
		_initialized = true;
	}
}