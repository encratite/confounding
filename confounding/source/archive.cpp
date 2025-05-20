#include "archive.h"

namespace confounding {
	DailyRecord::DailyRecord(Date date, double close)
		: date(date),
		close(close) {
	}
}