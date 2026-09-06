#include "views/utils/WeekGridLayout.h"

#include <algorithm>

#include "views/utils/ViewShared.h"

namespace app {

namespace {
constexpr int kMinutesPerDay = 24 * 60;

events::TimePoint effectiveEnd(const events::Occurrence& occ) {
    return occ.duration > events::Duration::zero() ? occ.end()
                                                    : occ.start + std::chrono::minutes(1);
}

/** @brief Impila le occorrenze "tutto il giorno" su righe: ogni item occupa
 *  la prima riga libera per tutta la sua estensione [firstDay, lastDay]. */
struct AllDayItem {
    int index, firstDay, lastDay, row;
};

std::vector<AllDayItem> layoutAllDay(const std::vector<events::Occurrence>& occurrences,
                                     const QDate& viewStart, int dayCount,
                                     std::vector<bool>& placed, int& outMaxRows) {
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
        placed[i] = true;
    }

    outMaxRows = 1;
    for (int d = 0; d < dayCount; ++d) {
        outMaxRows = std::max(outMaxRows, static_cast<int>(dayRows[d].size()));
    }
    return items;
}

} // namespace

namespace WeekGridLayout {

WeekGridResult place(const std::vector<events::Occurrence>& occurrences,
                     const QDate& viewStart, int dayCount,
                     const WeekGridGeometry& geometry) {
    WeekGridResult result;
    result.placements.assign(occurrences.size(), OccurrencePlacement{QRect(), false});

    std::vector<bool> placed(occurrences.size(), false);

    // --- Striscia "tutto il giorno" -----------------------------------------
    int maxRows = 1;
    const std::vector<AllDayItem> allDayItems =
        layoutAllDay(occurrences, viewStart, dayCount, placed, maxRows);
    result.allDayHeight = maxRows * geometry.allDayRowHeight;

    for (const AllDayItem& item : allDayItems) {
        const int x = geometry.gutterWidth + item.firstDay * geometry.dayWidth + 2;
        const int w = (item.lastDay - item.firstDay + 1) * geometry.dayWidth - 4;
        const int y = geometry.headerHeight + 2 + item.row * geometry.allDayRowHeight;
        result.placements[item.index] = {QRect(x, y, w, geometry.allDayRowHeight - 4), true};
    }

    // --- Griglia oraria: layout a colonne per giorno ------------------------
    const int gridTop = geometry.headerHeight + result.allDayHeight;
    for (int day = 0; day < dayCount; ++day) {
        std::vector<int> dayIndex;
        for (int i = 0; i < static_cast<int>(occurrences.size()); ++i) {
            if (coversFullDay(occurrences[i])) {
                continue;
            }
            if (viewStart.daysTo(localTime(occurrences[i].start).date()) == day) {
                dayIndex.push_back(i);
            }
        }
        std::sort(dayIndex.begin(), dayIndex.end(), [&occurrences](int a, int b) {
            return occurrences[a].start < occurrences[b].start;
        });

        int k = 0;
        while (k < static_cast<int>(dayIndex.size())) {
            auto clusterStop = effectiveEnd(occurrences[dayIndex[k]]);
            int j = k + 1;
            while (j < static_cast<int>(dayIndex.size()) &&
                   occurrences[dayIndex[j]].start < clusterStop) {
                clusterStop = std::max(clusterStop, effectiveEnd(occurrences[dayIndex[j]]));
                ++j;
            }

            std::vector<int> column;
            std::vector<events::TimePoint> columnEnd;
            for (int t = k; t < j; ++t) {
                const int idx = dayIndex[t];
                const events::TimePoint start = occurrences[idx].start;
                int col = 0;
                while (col < static_cast<int>(columnEnd.size()) && !(start >= columnEnd[col])) {
                    ++col;
                }
                if (col == static_cast<int>(columnEnd.size())) {
                    columnEnd.push_back(start);
                }
                column.push_back(col);
                columnEnd[col] = std::max(columnEnd[col], effectiveEnd(occurrences[idx]));
            }
            const int clusterCols = std::max(1, static_cast<int>(columnEnd.size()));

            for (int t = k; t < j; ++t) {
                const int idx = dayIndex[t];
                const events::Occurrence& occ = occurrences[idx];
                const QDateTime localStart = localTime(occ.start);
                const QDateTime localEnd = localTime(occ.end());

                const int topMin = localStart.time().msecsSinceStartOfDay() / 60000;
                int bottomMin = localEnd.time().msecsSinceStartOfDay() / 60000;
                if (localEnd.date() != localStart.date()) {
                    bottomMin = kMinutesPerDay;
                }
                const int lo = qBound(0, topMin, kMinutesPerDay);
                const int hi = qBound(0, bottomMin, kMinutesPerDay);
                int h = (hi - lo) * geometry.hourHeight / 60 - 4;
                if (h < geometry.minOccurrenceHeight) {
                    h = geometry.minOccurrenceHeight;
                }

                const int colWidth = geometry.dayWidth / clusterCols;
                const int x = geometry.gutterWidth + day * geometry.dayWidth +
                              column[t - k] * colWidth + 2;
                const int y = gridTop + lo * geometry.hourHeight / 60 + 2;
                result.placements[idx] = {QRect(x, y, colWidth - 4, h), true};
                placed[idx] = true;
            }
            k = j;
        }
    }

    for (int i = 0; i < static_cast<int>(placed.size()); ++i) {
        if (!placed[i]) {
            result.placements[i].visible = false;
        }
    }

    return result;
}

} // namespace WeekGridLayout

} // namespace app
