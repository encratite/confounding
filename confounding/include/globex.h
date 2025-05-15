#pragma once

#include <string>
#include <optional>

namespace confounding {
	class GlobexCode {
	public:
		std::optional<std::string> symbol;
		std::optional<std::string> root;
		std::optional<char> month;
		std::optional<unsigned> year;

		GlobexCode(std::string root, char month, unsigned year);
		GlobexCode(std::string symbol);
		bool operator==(const GlobexCode& other) const;
		bool operator<(const GlobexCode& other) const;
		bool operator>(const GlobexCode& other) const;
		bool operator<=(const GlobexCode& other) const;
		bool operator>=(const GlobexCode& other) const;
		bool is_globex_code(const std::string& symbol) const;
		void add_year();
	};
}