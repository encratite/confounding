#include <mutex>

#include "confounding/yaml.h"
#include "confounding/configuration/contracts.h"
#include "confounding/exception.h"

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

	const Contract& ContractConfiguration::get_contract(std::string symbol) const {
		auto iterator = std::find_if(
			_contracts.begin(),
			_contracts.end(),
			[&](const Contract& contract) {
				return contract.symbol == symbol;
			}
		);
		if (iterator == _contracts.end())
			throw Exception("Unable to find a contract matching symbol \"{}\"", symbol);
		return *iterator;
	}

	void ContractConfiguration::load() {
		if (_initialized)
			return;
		YAML::Node doc = YAML::LoadFile(configuration_file);
		if (!doc.IsSequence())
			throw Exception("The contract configuration file must consist of a sequence at the top level");
		for (const auto& entry : doc) {
			auto get_money = [&](const char* key) {
				std::string money_string = entry[key].as<std::string>();
				Money money(money_string);
				return money;
			};
			Contract contract{
				.symbol = entry["symbol"].as<std::string>(),
				.name = entry["name"].as<std::string>(),
				.currency = entry["currency"].as<std::string>(),
				.tick_size = get_money("tick_size"),
				.tick_value = get_money("tick_value"),
				.margin = get_money("margin"),
				.broker_fee = get_money("broker_fee"),
				.exchange_fee = get_money("exchange_fee"),
				.spread = entry["spread"].as<unsigned>(),
			};
		}
		_initialized = true;
	}
}