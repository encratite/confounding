#include <mutex>

#include "yaml.h"
#include "configuration/base.h"

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
		_initialized = true;
	}
}