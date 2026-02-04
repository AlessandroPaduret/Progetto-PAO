#ifndef RECURRENTEVENT_H
#define RECURRENTEVENT_H

#include <vector>
#include <chrono>
#include <string>
#include <unordered_set>
#include <functional>
#include "Event.h"


struct TimePointHasher {
    std::size_t operator()(const TimePoint& tp) const {
        return std::hash<long long>{}(tp.time_since_epoch().count());
    }
};

class RecurrentEvent : public Event {
private:
    std::vector<Duration> m_recurrenceInterval;
    std::unordered_set<TimePoint, TimePointHasher> m_exceptions;
    std::unordered_map<TimePoint, Event, TimePointHasher> m_modifications;
    TimePoint m_end;
public:
    RecurrentEvent(const std::string& title = "", 
                   const TimePoint start = std::chrono::time_point_cast<Duration>(Clock::now()), 
                   const Duration duration = Duration::zero(),
                   const Duration recurrenceInterval = Duration::zero());
                   
    std::vector<Duration> getRecurrenceIntervals() const;
    std::vector<Event> getOccurrences(
        const TimePoint from,
        const TimePoint to) const;
    bool isException(const TimePoint time) const;
    bool isModified(const TimePoint time) const;

    void addRecurrenceInterval(const Duration interval);
    void addException(const TimePoint exception);
    void addModification(const Event& modifiedEvent);
};

#endif