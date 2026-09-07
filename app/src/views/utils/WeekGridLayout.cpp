#include "views/utils/WeekGridLayout.h"

#include <algorithm>
#include <numeric>

#include "views/utils/ViewShared.h"

namespace app {

namespace {
constexpr int kMinutesPerDay = 24 * 60;

events::TimePoint effectiveEnd(const events::Occurrence& occ) {
    return occ.duration > events::Duration::zero() ? occ.end()
                                                    : occ.start + std::chrono::minutes(1);
}
} // namespace

namespace WeekGridLayout {

std::vector<AllDayItem> layoutAllDayRows(const std::vector<events::Occurrence>& occurrences,
                                         const QDate& viewStart, int dayCount) {
    std::vector<std::vector<bool>> dayRows(dayCount); // [giorno][riga] occupata
    std::vector<AllDayItem> items;

    for (int i = 0; i < static_cast<int>(occurrences.size()); ++i) {
        const events::Occurrence& occ = occurrences[i];
        if (!coversFullDay(occ)) {
            continue;
        }
        const QDate startDate = localTime(occ.start).date();
        const QDate endExclusive = localTime(occ.end()).date();
        int firstDay = viewStart.daysTo(startDate);
        int lastDay = viewStart.daysTo(endExclusive) - 1;
        if (lastDay < firstDay) {
            lastDay = firstDay;
        }
        firstDay = qBound(0, firstDay, dayCount - 1);
        lastDay = qBound(0, lastDay, dayCount - 1);

        int row = 0;
        bool free = false;
        while (!free) {
            free = true;
            for (int d = firstDay; d <= lastDay; ++d) {
                if (static_cast<int>(dayRows[d].size()) > row && dayRows[d][row]) {
                    free = false;
                    ++row;
                    break;
                }
            }
        }
        for (int d = firstDay; d <= lastDay; ++d) {
            if (static_cast<int>(dayRows[d].size()) <= row) {
                dayRows[d].resize(row + 1, false);
            }
            dayRows[d][row] = true;
        }
        items.push_back({i, firstDay, lastDay, row});
    }

    return items;
}

std::vector<QRect> layoutDayColumn(const std::vector<events::Occurrence>& dayOccurrences,
                                   const QDate& date, int columnWidth) {
    std::vector<QRect> result(dayOccurrences.size());

    std::vector<int> order(dayOccurrences.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&dayOccurrences](int a, int b) {
        return dayOccurrences[a].start < dayOccurrences[b].start;
    });

    int k = 0;
    while (k < static_cast<int>(order.size())) {
        auto clusterStop = effectiveEnd(dayOccurrences[order[k]]);
        int j = k + 1;
        while (j < static_cast<int>(order.size()) &&
               dayOccurrences[order[j]].start < clusterStop) {
            clusterStop = std::max(clusterStop, effectiveEnd(dayOccurrences[order[j]]));
            ++j;
        }

        std::vector<int> column;
        std::vector<events::TimePoint> columnEnd;
        for (int t = k; t < j; ++t) {
            const events::TimePoint start = dayOccurrences[order[t]].start;
            int col = 0;
            while (col < static_cast<int>(columnEnd.size()) && !(start >= columnEnd[col])) {
                ++col;
            }
            if (col == static_cast<int>(columnEnd.size())) {
                columnEnd.push_back(start);
            }
            column.push_back(col);
            columnEnd[col] = std::max(columnEnd[col], effectiveEnd(dayOccurrences[order[t]]));
        }
        const int clusterCols = std::max(1, static_cast<int>(columnEnd.size()));

        for (int t = k; t < j; ++t) {
            const int idx = order[t];
            const events::Occurrence& occ = dayOccurrences[idx];
            // Ritaglia l'intervallo visibile su QUESTO giorno: un'occorrenza
            // iniziata ieri (a cavallo di mezzanotte) parte visivamente dalle
            // 00:00 di oggi; una che finisce domani si ferma alle 24:00 di
            // oggi. Il resto (l'altra meta') e' un'altra OccurrenceWidget,
            // creata dalla colonna del giorno adiacente (vedi
            // WeekView::distributeOccurrences).
            const QDateTime dayStart(date, QTime(0, 0));
            const QDateTime dayEnd = dayStart.addDays(1);
            const QDateTime localStart = std::max(localTime(occ.start), dayStart);
            const QDateTime localEnd = std::min(localTime(occ.end()), dayEnd);

            const int topMin = localStart.time().msecsSinceStartOfDay() / 60000;
            const int bottomMin = localEnd == dayEnd
                                      ? kMinutesPerDay
                                      : localEnd.time().msecsSinceStartOfDay() / 60000;
            const int lo = qBound(0, topMin, kMinutesPerDay);
            const int hi = qBound(0, bottomMin, kMinutesPerDay);
            int h = (hi - lo) * kWeekHourHeight / 60 - 4;
            if (h < kWeekMinOccurrenceHeight) {
                h = kWeekMinOccurrenceHeight;
            }

            const int colWidth = columnWidth / clusterCols;
            const int x = column[t - k] * colWidth + 2;
            const int y = lo * kWeekHourHeight / 60 + 2;
            result[idx] = QRect(x, y, colWidth - 4, h);
        }
        k = j;
    }
    return result;
}

} // namespace WeekGridLayout

} // namespace app
