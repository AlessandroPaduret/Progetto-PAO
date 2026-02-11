#include <ctime>
#include <iostream>
#include <memory>
#include <string>

#include "events/core/CommonTypes.h"
#include "events/domain/Event.h"

namespace events {

Event *Event::clone_impl() const { return new Event(*this); }

Event::Event(const String &title, const TimePoint start,
             const Duration duration)
    : m_title(title), m_start(start), m_duration(duration) {
  if (duration < Duration::zero()) {
    throw std::invalid_argument("La durata non può essere negativa.");
  }
}

void Event::setTitle(const String &title) { m_title = title; }

String Event::getTitle() const { return m_title; }

std::ostream &operator<<(std::ostream &os, const Event &event) {

  std::chrono::hh_mm_ss time{event.m_duration};
  
  return os << "Evento: " << event.getTitle() << "\n"
     << "Inizio: " << event.m_start << "\n"
     << "Fine: " << event.getEnd() << "\n"
     << "Durata: " << time << " hh:mm::ss\n";
}

TimePoint Event::getStart() const { return m_start; }

Duration Event::getDuration() const { return m_duration; }

TimePoint Event::getEnd() const { return m_start + m_duration; }

void Event::setStart(const TimePoint start) { m_start = start; }

void Event::setDuration(const Duration duration) {
  if (duration < Duration::zero()) {
    throw std::invalid_argument("La durata non può essere negativa.");
  }
  m_duration = duration;
}

void Event::setEnd(const TimePoint end) { setDuration(end - m_start); }

bool Event::isIn(const TimePoint from, const TimePoint to) const {
  return (m_start >= from) && (getEnd() <= to);
}

bool Event::overlapsWith(const Schedulable &other) const {
  return (m_start < other.getEnd()) && (getEnd() > other.getStart());
}

std::unique_ptr<Event> Event::clone() const {
  return std::unique_ptr<Event>(clone_impl());
}

} // namespace events
