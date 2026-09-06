#include <generator>

#include "events/core/DateGenerator.h"
#include "events/core/CommonTypes.h"

namespace events {

[[nodiscard]] std::generator<TimePoint> DateGenerator::occurrences(TimePoint start, TimePoint from, TimePoint limit) const {
    if (start > limit || from > limit) {
        co_return;
    }

    TimePoint current = align(start, from);

    while (current <= limit && current != TimePoint::max()) {
        co_yield current;
        current = next(current);
    }
}

}