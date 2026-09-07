#include "generators/YearlyGenerator.h"
#include <chrono>
#include <algorithm>

namespace events {

YearlyGenerator::YearlyGenerator(int years)
    : m_years(years) {
        if (years < 1) {
            throw std::invalid_argument("Il numero di anni deve essere positivo");
        }
    }

TimePoint YearlyGenerator::next(TimePoint point) const {
    namespace sys = std::chrono;

    // 1. Isoliamo la componente giorno/mese/anno e l'orario del giorno
    auto sysDays = sys::floor<sys::days>(point);
    sys::year_month_day ymd{sysDays};
    auto timeOfDay = point - sysDays;

    // 2. Avanziamo di m_years anni
    auto targetYear = ymd.year() + sys::years(m_years);
    sys::year_month_day nextYmd = targetYear / ymd.month() / ymd.day();

    // 3. Clamping per anni non bisestili (es. 29 Feb 2024 + 1 anno -> 28 Feb 2025)
    if (!nextYmd.ok()) {
        nextYmd = targetYear / ymd.month() / sys::last;
    }

    // 4. Ricomponiamo il TimePoint mantenendo l'orario originale
    return sys::sys_days{nextYmd} + timeOfDay;
}

TimePoint YearlyGenerator::align(TimePoint start, TimePoint from) const {
    namespace sys = std::chrono;

    if (from <= start) {
        return start;
    }

    auto startDays = sys::floor<sys::days>(start);
    auto fromDays  = sys::floor<sys::days>(from);

    sys::year_month_day startYmd{startDays};
    sys::year_month_day fromYmd{fromDays};

    auto startTimeOfDay = start - startDays;

    // 1. Calcolo della differenza in anni in O(1)
    int startYear = static_cast<int>(startYmd.year());
    int fromYear  = static_cast<int>(fromYmd.year());

    int diffYears = std::max(0, fromYear - startYear);

    // 2. Calcolo dei passi di intervallo trascorsi (arrotondamento per difetto)
    int numIntervals = diffYears / m_years;

    // 3. Costruzione della prima data candidata <= from
    auto candidateYear = startYmd.year() + sys::years(numIntervals * m_years);
    sys::year_month_day candidateYmd = candidateYear / startYmd.month() / startYmd.day();

    if (!candidateYmd.ok()) {
        candidateYmd = candidateYear / startYmd.month() / sys::last;
    }

    TimePoint candidate = sys::sys_days{candidateYmd} + startTimeOfDay;

    // 4. Se l'arrotondamento per difetto cade prima di 'from', saltiamo al ciclo successivo
    if (candidate < from) {
        candidate = next(candidate);
    }

    return candidate;
}

void YearlyGenerator::accept(DateGeneratorVisitor& visitor) const {
    visitor.visit(*this);
}

bool YearlyGenerator::isEqualImpl(const utils::Cacheable& other) const {
    const auto& typedOther = static_cast<const YearlyGenerator&>(other);
    return m_years == typedOther.m_years;
}

std::size_t YearlyGenerator::hash() const {
    return std::hash<int>{}(m_years);
}

} // namespace events