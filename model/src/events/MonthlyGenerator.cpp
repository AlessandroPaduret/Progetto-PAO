#include "events/generators/MonthlyGenerator.h"
#include <chrono>
#include <algorithm>

namespace events {

MonthlyGenerator::MonthlyGenerator(int months)
    : m_months(months) {
        if (months < 1) {
            throw std::invalid_argument("Il numero di mesi deve essere positivo");
        }
    }

TimePoint MonthlyGenerator::next(TimePoint point) const {
    namespace sys = std::chrono;

    // 1. Convertiamo in giorni solari e isoliamo l'orario del giorno
    auto sysDays = sys::floor<sys::days>(point);
    sys::year_month_day ymd{sysDays};
    auto timeOfDay = point - sysDays;

    // 2. Avanziamo di m_months mesi usando l'aritmetica native C++20
    auto targetYm = ymd.year() / ymd.month() + sys::months(m_months);
    sys::year_month_day nextYmd = targetYm / ymd.day();

    // 3. Gestione clamping per mesi più corti (es. 31 Gennaio -> 28/29 Febbraio)
    if (!nextYmd.ok()) {
        nextYmd = targetYm / sys::last;
    }

    // 4. Ricomponiamo il TimePoint mantenendo l'orario di partenza
    return sys::sys_days{nextYmd} + timeOfDay;
}

TimePoint MonthlyGenerator::align(TimePoint start, TimePoint from) const {
    namespace sys = std::chrono;

    // Se la finestra di ricerca precede o coincide con l'inizio dell'attività
    if (from <= start) {
        return start;
    }

    auto startDays = sys::floor<sys::days>(start);
    auto fromDays  = sys::floor<sys::days>(from);

    sys::year_month_day startYmd{startDays};
    sys::year_month_day fromYmd{fromDays};

    auto startTimeOfDay = start - startDays;

    // 1. Calcolo del numero totale di mesi trascorsi in O(1)
    int startTotalMonths = static_cast<int>(startYmd.year()) * 12 + static_cast<unsigned>(startYmd.month());
    int fromTotalMonths  = static_cast<int>(fromYmd.year()) * 12 + static_cast<unsigned>(fromYmd.month());

    int diffMonths = std::max(0, fromTotalMonths - startTotalMonths);
    
    // 2. Calcolo dei passi d'intervallo trascorsi (arrotondamento per difetto)
    int numIntervals = diffMonths / m_months;

    // 3. Costruzione della prima data candidata <= from
    auto candidateYm = startYmd.year() / startYmd.month() + sys::months(numIntervals * m_months);
    sys::year_month_day candidateYmd = candidateYm / startYmd.day();

    if (!candidateYmd.ok()) {
        candidateYmd = candidateYm / sys::last;
    }

    TimePoint candidate = sys::sys_days{candidateYmd} + startTimeOfDay;

    // 4. Se l'arrotondamento ci posiziona prima di 'from', saltiamo al ciclo successivo
    if (candidate < from) {
        candidate = next(candidate);
    }

    return candidate;
}

void MonthlyGenerator::accept(DateGeneratorVisitor& visitor) const {
    visitor.visit(*this);
}

bool MonthlyGenerator::isEqualImpl(const utils::Cacheable& other) const {
    const auto& typedOther = static_cast<const MonthlyGenerator&>(other);
    return m_months == typedOther.m_months;
}

std::size_t MonthlyGenerator::hash() const {
    return std::hash<int>{}(m_months);
}

} // namespace events