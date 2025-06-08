#include <regex>
#include <format>

#include "confounding/globex.h"
#include "confounding/exception.h"

namespace confounding {
	namespace {
		const std::regex globex_code_pattern("^([A-Z0-9]{2,})([FGHJKMNQUVXZ])([0-9]{2})$");
	}

	GlobexCode::GlobexCode() {
	}

	GlobexCode::GlobexCode(std::string root, char month, unsigned year)
		: root(std::move(root)),
		month(month),
		year(year) {
	}

	GlobexCode::GlobexCode(const std::string& symbol)
		: symbol(symbol) {
		std::smatch matches;
		if (!std::regex_match(symbol, matches, globex_code_pattern))
			throw Exception("Unable to parse Globex code: {}", symbol);
		root = matches[1];
		std::string month_string = matches[2];
		month = month_string[0];
		year = std::stoi(matches[3]);
		if (year < 70u)
			*year += 1900u;
		else
			*year += 2000u;
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