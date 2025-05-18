#pragma once

#include <optional>

#pragma warning(disable: 4251)
#pragma warning(disable: 4275)

#include <yaml-cpp/yaml.h>

#include "globex.h"

using confounding::GlobexCode;

namespace YAML {
	template<>
	struct convert<std::optional<GlobexCode>> {
		static bool decode(const Node& node, std::optional<GlobexCode>& globex_code) {
			if (!node || node.IsNull()) {
				globex_code = std::nullopt;
				return true;
			}
			std::string string = node.as<std::string>();
			globex_code = GlobexCode(string);
			return true;
		}
	};

	template <typename T>
	requires std::is_default_constructible_v<T>
	struct convert<std::optional<T>> {
		static bool decode(const Node& node, std::optional<T>& output) {
			if (!node || node.IsNull()) {
				output = std::nullopt;
				return true;
			}
			T value;
			if (!convert<T>::decode(node, value))
				return false;
			output = std::move(value);
			return true;
		}
	};
}