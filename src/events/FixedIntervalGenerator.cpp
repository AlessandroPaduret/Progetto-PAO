#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/generators/FixedIntervalGenerator.h"

FixedIntervalGenerator::FixedIntervalGenerator(TimePoint start, Duration interval, TimePoint end)
    : m_start(start), m_interval(interval), m_end(end) {}

    TimePoint FixedIntervalGenerator::getStart() const { 
    return m_start; 
}

Duration FixedIntervalGenerator::getInterval() const { 
    return m_interval; 
}

TimePoint FixedIntervalGenerator::getEnd() const {
    return m_end;
}
void FixedIntervalGenerator::setStart(TimePoint newStart) { 
    m_start = newStart; 
}

void FixedIntervalGenerator::setInterval(Duration newInterval) { 
    m_interval = newInterval; 
}

void FixedIntervalGenerator::setEnd(TimePoint newEnd) {
    m_end = newEnd;
}

/// Implementazione dei metodi virtuali di DateGenerator

std::vector<TimePoint> FixedIntervalGenerator::generateDates(const TimePoint from,const TimePoint to) const {
    std::vector<TimePoint> dates;

    TimePoint current = from;

    while(current <= to) {
        dates.push_back(current);
        current += m_interval;
    }

    return dates;
}

bool FixedIntervalGenerator::occursInRange(const TimePoint from, const TimePoint to) const {
    return to > m_start && from < m_end;
}