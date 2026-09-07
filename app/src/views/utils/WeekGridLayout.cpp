#include "views/utils/WeekGridLayout.h"

#include <algorithm>
#include <numeric>
#include <ranges>

namespace app {

namespace WeekGridLayout {

std::vector<AllDayPlacement> layoutAllDayRows(const std::vector<DaySpan>& spans, int dayCount) {
    std::vector<std::vector<bool>> dayRows(dayCount);  // [giorno][riga] occupata
    std::vector<AllDayPlacement> placements;
    placements.reserve(spans.size());

    for (int i = 0; i < static_cast<int>(spans.size()); ++i) {
        const int firstDay = qBound(0, spans[i].startDay, dayCount - 1);
        const int lastDay = qBound(firstDay, spans[i].endDay, dayCount - 1);

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
        placements.push_back({i, row, firstDay, lastDay});
    }

    return placements;
}

std::vector<QRect> layoutDayColumn(const std::vector<TimeSlot>& timeSlots, int columnWidth) {
    std::vector<QRect> result(timeSlots.size());

    std::vector<int> order(timeSlots.size());
    std::iota(order.begin(), order.end(), 0);
    std::ranges::sort(order, {}, [&timeSlots](int i) { return timeSlots[i].startMinutes; });

    int k = 0;
    while (k < static_cast<int>(order.size())) {
        int clusterStop = timeSlots[order[k]].endMinutes;
        int j = k + 1;
        while (j < static_cast<int>(order.size()) && timeSlots[order[j]].startMinutes < clusterStop) {
            clusterStop = std::max(clusterStop, timeSlots[order[j]].endMinutes);
            ++j;
        }

        // Coloring greedy del cluster: ogni slot va nella prima colonna gia'
        // libera (il cui ultimo slot finisce prima del suo inizio).
        std::vector<int> column;
        std::vector<int> columnEnd;
        for (int t = k; t < j; ++t) {
            const int start = timeSlots[order[t]].startMinutes;
            int col = 0;
            while (col < static_cast<int>(columnEnd.size()) && start < columnEnd[col]) {
                ++col;
            }
            if (col == static_cast<int>(columnEnd.size())) {
                columnEnd.push_back(start);
            }
            column.push_back(col);
            columnEnd[col] = std::max(columnEnd[col], timeSlots[order[t]].endMinutes);
        }
        const int clusterCols = std::max(1, static_cast<int>(columnEnd.size()));

        for (int t = k; t < j; ++t) {
            const int idx = order[t];
            const TimeSlot& slot = timeSlots[idx];
            int h = (slot.endMinutes - slot.startMinutes) * kWeekHourHeight / 60 - 4;
            h = std::max(h, kWeekMinOccurrenceHeight);

            const int colWidth = columnWidth / clusterCols;
            const int x = column[t - k] * colWidth + 2;
            const int y = slot.startMinutes * kWeekHourHeight / 60 + 2;
            result[idx] = QRect(x, y, colWidth - 4, h);
        }
        k = j;
    }
    return result;
}

} // namespace WeekGridLayout

} // namespace app
