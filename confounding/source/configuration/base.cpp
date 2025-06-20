#include <mutex>

#include "confounding/yaml.h"
#include "confounding/configuration/base.h"
#include "confounding/common.h"

namespace {
	constexpr const char* configuration_file = "configuration.yaml";

	std::mutex mutex;
}

namespace confounding {
	Configuration::Configuration()
		: _initialized(false) {
	}

	const Configuration& Configuration::get() {
		std::lock_guard<std::mutex> lock(mutex);
		static Configuration configuration;
		configuration.load();
		return configuration;
	}

	void Configuration::load() {
		if (_initialized)
			return;
		YAML::Node doc = YAML::LoadFile(configuration_file);
		barchart_directory = doc["barchart_directory"].as<std::string>();
		std::string reference_date_string = doc["reference_date"].as<std::string>();
		reference_date = get_date(reference_date_string);
		_initialized = true;
	}
}