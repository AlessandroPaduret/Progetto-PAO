#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/domain/Event.h"

namespace events {

Event::Event(const String &title, const TimePoint start,
             const Duration duration)
    : Activity(title), m_start(start), m_duration(duration) {
  if (duration < Duration::zero()) {
    throw std::invalid_argument("La durata non puo' essere negativa.");
  }
}

Event *Event::clone_impl() const { return new Event(*this); }

std::unique_ptr<Event> Event::clone() const {
  return std::unique_ptr<Event>(clone_impl());
}

std::ostream &operator<<(std::ostream &os, const Event &event) {

  std::chrono::hh_mm_ss time{event.m_duration};

  return os << "[Evento]\n"
            << event.getTitle() << "\n"
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
    throw std::invalid_argument("La durata non puo' essere negativa.");
  }
  m_duration = duration;
}

void Event::setEnd(const TimePoint end) { setDuration(end - m_start); }

bool Event::isIn(const TimePoint from, const TimePoint to) const {
  return (m_start >= from) && (getEnd() <= to);
}

bool Event::overlapsWith(const Event &other) const {
  return (m_start < other.getEnd()) && (getEnd() > other.getStart());
}

std::vector<Occurrence> Event::occurrencesIn(const TimePoint from,
                                             const TimePoint to) const {
  // Stessa semantica del resto del modello: inizio in [from, to] inclusivo
  if (m_start >= from && m_start <= to) {
    return {Occurrence{this, m_start, m_duration}};
  }
  return {};
}

void Event::moveTo(const TimePoint newStart) { m_start = newStart; }

String Event::describe() const {
  return "Evento: " + getTitle() + " - inizio " + formatDateTime(m_start) +
         ", durata " + formatDuration(m_duration);
}

void Event::accept(ActivityVisitor &visitor) const { visitor.visit(*this); }

} // namespace events
