#include <iostream>
#include <chrono>
#include <format>

#include <confounding/types.h>

void generate_display_strings() {
	using namespace confounding;
	for (int year = 1990; year <= 2040; year++) {
		for (unsigned month = 1; month <= 12; month++) {
			Date first_date{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{1}};
			auto next_month = std::chrono::local_days{first_date} + std::chrono::months{1};
			Time first_time{std::chrono::local_days{first_date}};
			Time last_time{std::chrono::duration_cast<std::chrono::hours>(next_month.time_since_epoch())};
			int first_count = first_time.time_since_epoch().count();
			int last_count = last_time.time_since_epoch().count();
			std::string condition = std::format(
				"<DisplayString Condition=\"_MyDur._MyRep &gt;= {} &amp;&amp; _MyDur._MyRep &lt; {}\">"
				"{}-{:02}-{{(_MyDur._MyRep - {}) / 24 + 1}} {{(_MyDur._MyRep % 24) / 10}}{{(_MyDur._MyRep % 24) % 10}}:00:00"
				"</DisplayString>",
				first_count,
				last_count,
				year,
				month,
				first_count
			);
			std::cout << condition << std::endl;
		}
	}
}

int main() {
	generate_display_strings();
	return 0;
}