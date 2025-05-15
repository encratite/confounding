#include "exception.h"

namespace confounding {
	Exception::Exception(const std::string& message)
		: std::runtime_error(message.c_str()) {
	}
}