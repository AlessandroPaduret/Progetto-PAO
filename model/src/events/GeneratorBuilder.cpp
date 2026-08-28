#include "events/builders/GeneratorBuilder.h"

#include "events/generators/SingleGenerator.h"
#include "events/generators/FixedIntervalGenerator.h"
#include "events/generators/MonthlyGenerator.h"
#include "events/generators/YearlyGenerator.h"
#include "events/generators/MaxOccurrencesDecorator.h"

namespace events {

GeneratorBuilder::GeneratorBuilder(TimePoint start) : m_start(start) {}

GeneratorBuilder GeneratorBuilder::from(TimePoint start) {
    return GeneratorBuilder(start);
}

GeneratorBuilder& GeneratorBuilder::asSingle() {
    m_type = RecurrenceUnit::Single;
    return *this;
}

GeneratorBuilder& GeneratorBuilder::repeatEvery(Duration interval) {
    m_type = RecurrenceUnit::Daily; // Sfruttiamo FixedIntervalGenerator per intervalli fissi in durata
    m_intervalValue = interval;
    return *this;
}

GeneratorBuilder& GeneratorBuilder::repeatMonthly(int intervalMonths) {
    m_type = RecurrenceUnit::Monthly;
    m_numericParam = (intervalMonths > 0) ? intervalMonths : 1;
    return *this;
}

GeneratorBuilder& GeneratorBuilder::repeatYearly(int intervalYears) {
    m_type = RecurrenceUnit::Yearly;
    m_numericParam = (intervalYears > 0) ? intervalYears : 1;
    return *this;
}

GeneratorBuilder& GeneratorBuilder::until(TimePoint end) {
    m_end = end;
    return *this;
}

GeneratorBuilder& GeneratorBuilder::limitTo(std::size_t maxOccurrences) {
    m_maxOccurrences = maxOccurrences;
    return *this;
}

std::unique_ptr<DateGenerator> GeneratorBuilder::build() {
    std::unique_ptr<DateGenerator> baseGen;

    // 1. Istanziazione del generatore concreto di base in base al tipo scelto
    switch (m_type) {
        case RecurrenceUnit::Daily: // Gestito da FixedIntervalGenerator (es. 24h, 7d)
            baseGen = std::make_unique<FixedIntervalGenerator>(m_start, m_intervalValue, m_end);
            break;
            
        case RecurrenceUnit::Monthly:
            baseGen = std::make_unique<MonthlyGenerator>(m_start, m_numericParam, m_end);
            break;
            
        case RecurrenceUnit::Yearly:
            baseGen = std::make_unique<YearlyGenerator>(m_start, m_end);
            break;
            
        case RecurrenceUnit::Single:
        default:
            baseGen = std::make_unique<SingleGenerator>(m_start);
            break;
    }

    // 2. Applicazione opzionale del Decorator per il limite di occorrenze
    // (Ha senso decorare solo se non e' un evento singolo e se il limite e' > 0)
    if (m_maxOccurrences > 0 && m_type != RecurrenceUnit::Single) {
        return std::make_unique<MaxOccurrencesDecorator>(std::move(baseGen), m_maxOccurrences);
    }

    return baseGen;
}

} // namespace events