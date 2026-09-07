#include "generators/FixedIntervalGenerator.h"
#include <algorithm>

namespace events {

FixedIntervalGenerator::FixedIntervalGenerator(Duration interval)
    : m_interval(interval > Duration::zero() ? interval : std::chrono::hours(24)) {}

TimePoint FixedIntervalGenerator::next(TimePoint point) const {
    return point + m_interval;
}

TimePoint FixedIntervalGenerator::align(TimePoint start, TimePoint from) const {
    if (from <= start) {
        return start;
    }

    auto durationSinceStart = from - start;
    auto numIntervals = durationSinceStart / m_interval;

    TimePoint candidate = start + (numIntervals * m_interval);

    // la divisione arrotonda per difetto, potrebbe restare sotto 'from'
    if (candidate < from) {
        candidate += m_interval;
    }

    return candidate;
}

void FixedIntervalGenerator::accept(DateGeneratorVisitor& visitor) const {
    visitor.visit(*this);
}

bool FixedIntervalGenerator::isEqualImpl(const utils::Cacheable& other) const {
    const auto& typedOther = static_cast<const FixedIntervalGenerator&>(other);
    return m_interval == typedOther.m_interval;
}

std::size_t FixedIntervalGenerator::hash() const {
    return std::hash<Duration::rep>{}(m_interval.count());
}

} // namespace events