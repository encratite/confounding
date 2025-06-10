#pragma once

#include <stdexcept>
#include <string>
#include <format>

#include "confounding/exports.h"

namespace confounding {
	class CONFOUNDING_API Exception: public std::runtime_error {
	public:
		Exception(const std::string& message);

		template <typename... Args>
			requires (sizeof...(Args) >= 1)
		Exception(const std::format_string<Args...> format, Args&&... arguments)
			: Exception(std::format(format, std::forward<Args>(arguments)...)) {
		}
	};
}