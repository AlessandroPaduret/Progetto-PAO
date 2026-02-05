#include "Event.h"

#include <iostream>
#include <ctime>
#include <string>

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock, std::chrono::seconds>;
using Duration = std::chrono::seconds;
using String = std::string;

Event::Event(const String& title, 
             const TimePoint start, 
             const Duration duration)
    : m_title(title), m_start(start), m_duration(duration) {}

String Event::getTitle() const { 
    return m_title; 
}

TimePoint Event::getStart() const { 
    return m_start; 
}

Duration Event::getDuration() const { 
    return m_duration; 
}

TimePoint Event::getEnd() const { 
    return m_start + m_duration; 
}

bool Event::isIn(const TimePoint from, const TimePoint to) const {
    return (m_start >= from) && (getEnd() <= to);
}

bool Event::overlapsWith(const Schedulable& other) const {
    return (m_start < other.getEnd()) && (getEnd() > other.getStart());
}

void Event::postpone(const Duration delta) {
    m_start += delta;
}

void Event::advance(const Duration delta) {
    m_start -= delta;
}

std::ostream& operator<<(std::ostream& os, const Event& event) {
    std::time_t start_time = Clock::to_time_t(event.getStart());
    std::time_t end_time = Clock::to_time_t(event.getEnd());

    os << "Evento: " << event.getTitle() << "\n"
       << "Inizio: " << std::ctime(&start_time)
       << "Fine: " << std::ctime(&end_time)
       << "Durata: " << event.getDuration().count() << " secondi\n";
    return os;
}