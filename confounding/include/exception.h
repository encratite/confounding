#pragma once

#include <stdexcept>
#include <string>
#include <format>

namespace confounding {
	class Exception: public std::runtime_error {
	public:
		Exception(const std::string& message);

		template <typename... Args>
			requires (sizeof...(Args) >= 1)
		Exception(const std::format_string<Args...> format, Args&&... arguments)
			: Exception(std::format(format, std::forward<Args>(arguments)...)) {
		}
	};
}