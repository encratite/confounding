#include "archive.h"

namespace confounding {
	DailyRecord::DailyRecord(Date date, Money close)
		: date(date),
		close(close) {
	}
}