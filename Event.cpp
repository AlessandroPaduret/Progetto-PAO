#include "Event.h"

Event::Event(const std::string& title, 
             const TimePoint start, 
             const Duration duration)
    : m_title(title), m_start(start), m_duration(duration) {}

std::string Event::getTitle() const { 
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