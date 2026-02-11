#include <chrono>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/generators/FixedIntervalGenerator.h"

namespace events {

FixedIntervalGenerator::FixedIntervalGenerator(TimePoint start,
                                               Duration interval, TimePoint end)
    : m_start(start), m_interval(interval), m_end(end) {}

TimePoint FixedIntervalGenerator::getStart() const { return m_start; }

Duration FixedIntervalGenerator::getInterval() const { return m_interval; }

TimePoint FixedIntervalGenerator::getEnd() const { return m_end; }
void FixedIntervalGenerator::setStart(TimePoint newStart) {
  m_start = newStart;
}

void FixedIntervalGenerator::setInterval(Duration newInterval) {
  m_interval = newInterval;
}

void FixedIntervalGenerator::setEnd(TimePoint newEnd) { m_end = newEnd; }

/// Implementazione dei metodi virtuali di DateGenerator

std::vector<TimePoint>
FixedIntervalGenerator::generateDates(const TimePoint from,
                                      const TimePoint to) const {
  std::vector<TimePoint> dates;

    // 1. Troviamo il punto di partenza reale
    // Dobbiamo iniziare o da m_start o da 'from', a seconda di chi è più avanti
    TimePoint startSearch = std::max(m_start, from);

    // 2. Allineamento matematico (La "Formula")
    // Calcoliamo quanti intervalli sono passati tra m_start e startSearch
    // per non iniziare da un orario a caso (come le 14:38)
    auto durationSinceStart = std::max(startSearch - m_start, Duration(0));
    auto numIntervals = durationSinceStart / m_interval;
    
    // La prima occorrenza valida dopo o uguale a 'from'
    TimePoint current = m_start + (numIntervals * m_interval);
    
    // Se a causa dell'arrotondamento siamo finiti prima di 'from', saltiamo al prossimo
    if (current < from) {
        current += m_interval;
    }

    // 3. Generazione nel range
    while (current <= to && current <= m_end) {
        dates.push_back(current);
        current += m_interval;
    }

    return dates;
}

bool FixedIntervalGenerator::occursInRange(const TimePoint from,
                                           const TimePoint to) const {
  return to > m_start && from < m_end;
}

} // namespace events
