#pragma once

#include <stdexcept>
#include <string>

namespace confounding {
	class Exception: public std::runtime_error {
	public:
		Exception(const std::string& message);
	};
}