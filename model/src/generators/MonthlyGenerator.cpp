#include "generators/MonthlyGenerator.h"
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

    auto sysDays = sys::floor<sys::days>(point);
    sys::year_month_day ymd{sysDays};
    auto timeOfDay = point - sysDays;

    auto targetYm = ymd.year() / ymd.month() + sys::months(m_months);
    sys::year_month_day nextYmd = targetYm / ymd.day();

    // clamp per mesi piu' corti (es. 31 gennaio -> 28/29 febbraio)
    if (!nextYmd.ok()) {
        nextYmd = targetYm / sys::last;
    }

    return sys::sys_days{nextYmd} + timeOfDay;
}

TimePoint MonthlyGenerator::align(TimePoint start, TimePoint from) const {
    namespace sys = std::chrono;

    if (from <= start) {
        return start;
    }

    auto startDays = sys::floor<sys::days>(start);
    auto fromDays  = sys::floor<sys::days>(from);

    sys::year_month_day startYmd{startDays};
    sys::year_month_day fromYmd{fromDays};

    auto startTimeOfDay = start - startDays;

    // mesi trascorsi in O(1) (niente loop passo-passo), poi si arrotonda ai
    // passi di m_months e si ricostruisce direttamente la data candidata
    int startTotalMonths = static_cast<int>(startYmd.year()) * 12 + static_cast<unsigned>(startYmd.month());
    int fromTotalMonths  = static_cast<int>(fromYmd.year()) * 12 + static_cast<unsigned>(fromYmd.month());

    int diffMonths = std::max(0, fromTotalMonths - startTotalMonths);
    int numIntervals = diffMonths / m_months;

    auto candidateYm = startYmd.year() / startYmd.month() + sys::months(numIntervals * m_months);
    sys::year_month_day candidateYmd = candidateYm / startYmd.day();

    if (!candidateYmd.ok()) {
        candidateYmd = candidateYm / sys::last;
    }

    TimePoint candidate = sys::sys_days{candidateYmd} + startTimeOfDay;

    // l'arrotondamento per difetto puo' lasciarci prima di 'from'
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