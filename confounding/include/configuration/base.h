#pragma once

#include <string>

namespace confounding {
	class Configuration {
	public:
		std::string barchart_directory;

		Configuration();

		static const Configuration& get();
	private:
		bool _initialized;

		void load();
	};
}