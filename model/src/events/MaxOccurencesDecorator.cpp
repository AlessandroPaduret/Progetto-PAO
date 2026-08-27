#include <chrono>
#include <vector>
#include <sstream>

#include "events/core/CommonTypes.h"
#include "events/generators/MaxOccurrencesDecorator.h"

namespace events {

MaxOccurrencesDecorator::MaxOccurrencesDecorator(std::unique_ptr<DateGenerator> generator, std::size_t maxOccurrences)
        : m_generator(std::move(generator)), m_maxOccurrences(maxOccurrences) {}

TimePoint MaxOccurrencesDecorator::getStart() const  { 
        return m_generator->getStart(); 
}

TimePoint MaxOccurrencesDecorator::getEnd() const { 
        return m_generator->getEnd(); 
    }

std::size_t MaxOccurrencesDecorator::getMaxOccurrences() const {
  return m_maxOccurrences;
}

/// Implementazione dei metodi virtuali di DateGenerator

std::vector<TimePoint> MaxOccurrencesDecorator::generateDates(TimePoint from, TimePoint to) const {
        auto dates = m_generator->generateDates(from, to);
        if (m_maxOccurrences > 0 && dates.size() > m_maxOccurrences) {
            dates.resize(m_maxOccurrences);
        }
        return dates;
    }

bool MaxOccurrencesDecorator::isIn(TimePoint tp) const {
    if (!m_generator->isIn(tp)) return false;
    if (m_maxOccurrences == 0) return true;

    // Verifica che `tp` rientri nelle prime N occorrenze a partire dall'inizio
    auto dates = m_generator->generateDates(m_generator->getStart(), tp);
    return dates.size() <= m_maxOccurrences;
}

String MaxOccurrencesDecorator::describe() const {
        return m_generator->describe() + " (max " + std::to_string(m_maxOccurrences) + " occorrenze)";
    }

void MaxOccurrencesDecorator::accept(DateGeneratorVisitor& visitor) const {
        // Puoi fare il forward del visitor all'oggetto decorato
        m_generator->accept(visitor);
    }

} // namespace events
