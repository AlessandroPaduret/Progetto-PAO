#include "Event.h"
#include "CommonTypes.h"

#include <iostream>
#include <ctime>
#include <string>
#include <memory>

Event::Event(const String& title, 
             const TimePoint start, 
             const Duration duration)
    : m_title(title), m_start(start), m_duration(duration) {
    if (duration < Duration::zero()) {
        throw std::invalid_argument("La durata non può essere negativa.");
    }
}

void Event::setTitle(const String& title) {
    m_title = title;
}

String Event::getTitle() const { 
    return m_title; 
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

std::unique_ptr<Event> Event::clone() const {
        return std::make_unique<Event>(*this);
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

void Event::setStart(const TimePoint start) {
    m_start = start;
}

void Event::setDuration(const Duration duration) {
    if (duration < Duration::zero()) {
        throw std::invalid_argument("La durata non può essere negativa.");
    }
    m_duration = duration;
}

void Event::setEnd(const TimePoint end) {
    setDuration(end - m_start);
}

bool Event::isIn(const TimePoint from, const TimePoint to) const {
    return (m_start >= from) && (getEnd() <= to);
}

bool Event::overlapsWith(const Schedulable& other) const {
    return (m_start < other.getEnd()) && (getEnd() > other.getStart());
}
