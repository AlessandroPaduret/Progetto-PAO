#include <chrono>
#include <vector>
#include <sstream>

#include "events/core/CommonTypes.h"
#include "events/generators/MaxOccurrencesDecorator.h"

namespace events {

    
/// Implementazione di DateGenerator - Ciclo di Vita

MaxOccurrencesDecorator::MaxOccurrencesDecorator(std::unique_ptr<DateGenerator> generator, std::size_t maxOccurrences)
        : m_generator(std::move(generator)), m_maxOccurrences(maxOccurrences) {}

std::unique_ptr<DateGenerator> MaxOccurrencesDecorator::clone() const {
    return std::make_unique<MaxOccurrencesDecorator>(m_generator->clone(), m_maxOccurrences);
}


/// Query dello Stato e Accessor Specifici

const DateGenerator& MaxOccurrencesDecorator::getWrappedGenerator() const { 
        return *m_generator; 
}

TimePoint MaxOccurrencesDecorator::getStart() const  { 
        return m_generator->getStart(); 
}

TimePoint MaxOccurrencesDecorator::getEnd() const { 
        return m_generator->getEnd(); 
}

void MaxOccurrencesDecorator::setStart(TimePoint start) {
    m_generator->setStart(start);
}

void MaxOccurrencesDecorator::setEnd(TimePoint end) {
    m_generator->setEnd(end);
}

std::size_t MaxOccurrencesDecorator::getMaxOccurrences() const {
  return m_maxOccurrences;
}


/// Algoritmi di Generazione e Verifica Date

std::vector<TimePoint> MaxOccurrencesDecorator::generateDates(TimePoint from, TimePoint to) const {
    if (from > to) {
        return {};
    }

    // 1. Cerca sempre dall'inizio del generatore fino a 'to'
    auto dates = m_generator->generateDates(m_generator->getStart(), to);

    // 2. Togli gli elementi in eccesso (mantiene al massimo le prime m_maxOccurrences in assoluto).
    //    maxOccurrences == 0 significa "illimitate".
    if (m_maxOccurrences > 0 && dates.size() > m_maxOccurrences) {
        dates.resize(m_maxOccurrences);
    }

    // 3. Togli i primi elementi che hanno il tempo < di 'from'
    std::vector<TimePoint> result;
    for (const auto& date : dates) {
        if (date >= from) { // Filtra via quelle < from (e sono già <= to perché generate fino a 'to')
            result.push_back(date);
        }
    }

    return result;
}

bool MaxOccurrencesDecorator::isIn(TimePoint tp) const {
    if (!m_generator->isIn(tp)) return false;
    if (m_maxOccurrences == 0) return true;

    // Verifica che `tp` rientri nelle prime N occorrenze a partire dall'inizio
    auto dates = m_generator->generateDates(m_generator->getStart(), tp);
    return dates.size() <= m_maxOccurrences;
}


/// Ispezione e Serializzazione
String MaxOccurrencesDecorator::describe() const {
        return m_generator->describe() + " (max " + std::to_string(m_maxOccurrences) + " occorrenze)";
    }

void MaxOccurrencesDecorator::accept(DateGeneratorVisitor& visitor) const {
        // Il visitor sul decoratore serializza il generatore avvolto e poi
        // il limite di occorrenze (doppio dispatch sul tipo concreto).
        visitor.visit(*this);
    }

} // namespace events
