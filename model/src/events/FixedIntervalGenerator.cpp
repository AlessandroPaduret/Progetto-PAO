#include <chrono>
#include <memory>
#include <vector>
#include <sstream>

#include "events/core/CommonTypes.h"
#include "events/generators/FixedIntervalGenerator.h"

namespace events {


/// Implementazione di DateGenerator - Ciclo di Vita

FixedIntervalGenerator::FixedIntervalGenerator(TimePoint start,
                                               Duration interval, 
                                               TimePoint end)
    : m_start(start), m_interval(interval), m_end(end) {}

std::unique_ptr<DateGenerator> FixedIntervalGenerator::clone() const {
  return std::make_unique<FixedIntervalGenerator>(m_start, m_interval, m_end);
}


/// Query dello Stato e Accessor Specifici

TimePoint FixedIntervalGenerator::getStart() const { return m_start; }

Duration FixedIntervalGenerator::getInterval() const { return m_interval; }

TimePoint FixedIntervalGenerator::getEnd() const { return m_end; }

void FixedIntervalGenerator::setStart(TimePoint start) {
  if (start > m_end) {
    m_end = start;  // la fine non resta antecedente al nuovo inizio
  }
  m_start = start;
}

void FixedIntervalGenerator::setEnd(TimePoint end) { m_end = end; }


/// Algoritmi di Generazione e Verifica Date

std::vector<TimePoint>
FixedIntervalGenerator::generateDates(const TimePoint from,
                                      const TimePoint to) const {
  std::vector<TimePoint> dates;

    // 1. Troviamo il punto di partenza reale
    // Dobbiamo iniziare o da m_start o da 'from', a seconda di chi è più avanti
    TimePoint startSearch = std::max(m_start, from);

    // 2. Allineamento
    // Calcoliamo quanti intervalli sono passati tra m_start e startSearch
    auto durationSinceStart = std::max(startSearch - m_start, Duration(0));
    auto numIntervals = durationSinceStart / m_interval;
    
    // La prima occorrenza valida dopo o uguale a 'from'
    TimePoint current = m_start + (numIntervals * m_interval);
    
    // Se a causa dell'arrotondamento siamo finiti prima di 'from', saltiamo al prossimo
    if (current < from) {
        current += m_interval;
    }

    // 3. Generazione nel range (con limite di occorrenze, contate da m_start)
    while (current <= to && current <= m_end) {
        dates.push_back(current);
        current += m_interval;
    }

    return dates;
}

bool FixedIntervalGenerator::isIn(TimePoint tp) const {
  if (tp < m_start || tp > m_end) {
    return false;
  }
  const Duration offset = tp - m_start;
  if (offset < Duration::zero() || offset % m_interval != Duration::zero()) {
    return false;
  }
  return true;
}


/// Ispezione e Serializzazione

String FixedIntervalGenerator::describe() const {
    std::ostringstream oss;
    oss << "[FixedIntervalGenerator] starting at " << m_start.time_since_epoch().count()
        << " with interval of " << m_interval.count() << " seconds"
        << " and ending at " << m_end.time_since_epoch().count();
    return oss.str();
}

void FixedIntervalGenerator::accept(DateGeneratorVisitor& visitor) const {
  visitor.visit(*this);
}

} // namespace events
