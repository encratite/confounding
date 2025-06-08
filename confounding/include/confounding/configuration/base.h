#pragma once

#include <string>

#include "confounding/types.h"

namespace confounding {
	class Configuration {
	public:
		std::string barchart_directory;
		Date intraday_reference_date;

		Configuration();

		static const Configuration& get();
	private:
		bool _initialized;

		void load();
	};
}