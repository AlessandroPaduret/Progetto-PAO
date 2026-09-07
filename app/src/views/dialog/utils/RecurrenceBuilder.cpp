#include "views/dialog/utils/RecurrenceBuilder.h"

#include <algorithm>

namespace app::RecurrenceBuilder {

events::TimePoint calculateEndAfterCount(const events::DateGenerator& generator,
                                          events::TimePoint seriesStart, int count) {
    events::TimePoint current = generator.align(seriesStart, seriesStart);
    for (int i = 1; i < count && current != events::TimePoint::max(); ++i) {
        current = generator.next(current);
    }
    return current;
}

QDate calculateNthWeeklyDate(QDate startDate, int baseDow,
                              const std::vector<int>& selectedWeekdays,
                              int every, int count) {
    std::vector<int> offsets;
    offsets.reserve(selectedWeekdays.size());
    for (int dow : selectedWeekdays) {
        offsets.push_back((dow - baseDow + 7) % 7);
    }
    std::ranges::sort(offsets);

    const int weekdaysPerCycle = static_cast<int>(offsets.size());
    const int period = (count - 1) / weekdaysPerCycle;
    const int idx = (count - 1) % weekdaysPerCycle;
    return startDate.addDays(offsets[idx] + period * every * 7);
}

} // namespace app::RecurrenceBuilder
