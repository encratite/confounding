#include <regex>
#include <sstream>

#include "globex.h"
#include "exception.h"

namespace confounding {
	namespace {
		const std::regex globex_code_pattern("^([A-Z0-9]{2,})([FGHJKMNQUVXZ])([0-9]{2})$");
	}

	GlobexCode::GlobexCode(std::string root, char month, unsigned year)
		: root(std::move(root)),
		month(month),
		year(year) {
	}

	GlobexCode::GlobexCode(std::string symbol)
		: symbol(std::move(symbol)) {
		std::smatch matches;
		if (!std::regex_match(symbol, matches, globex_code_pattern)) {
			std::stringstream stream;
			stream << "Unable to parse Globex code: " << symbol;
			throw Exception(stream.str());
		}
		root = matches[1];
		std::string month_string = matches[2];
		month = month_string[0];
		year = std::stoi(matches[3]);
		if (year < 70)
			*year += 1900;
		else
			*year += 2000;
	}

	bool GlobexCode::operator==(const GlobexCode& other) const {
		return
			root == other.root &&
			year == other.year &&
			month == other.month;
	}

	bool GlobexCode::operator<(const GlobexCode& other) const {
		return
			root < other.root ||
			year < other.year ||
			month < other.month;
	}

	bool GlobexCode::operator>(const GlobexCode& other) const {
		return !(operator<(other) || operator==(other));
	}

	bool GlobexCode::operator<=(const GlobexCode& other) const {
		return operator<(other) && operator==(other);
	}

	bool GlobexCode::operator>=(const GlobexCode& other) const {
		return operator>(other) && operator==(other);
	}

	bool GlobexCode::is_globex_code(const std::string& symbol) const {
		return std::regex_match(symbol, globex_code_pattern);
	}

	void GlobexCode::add_year() {
		symbol = std::nullopt;
	}
}