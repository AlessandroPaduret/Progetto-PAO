#include <chrono>
#include <memory>
#include <stdexcept>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/domain/AllDayEvent.h"

namespace events {

AllDayEvent::AllDayEvent(const String &title, const TimePoint start,
                         const TimePoint end)
    : Activity(title), m_start(start), m_end(end) {
  if (end <= start) {
    throw std::invalid_argument(
        "La fine deve essere successiva all'inizio (almeno un giorno).");
  }
}

AllDayEvent *AllDayEvent::clone_impl() const { return new AllDayEvent(*this); }

std::unique_ptr<AllDayEvent> AllDayEvent::clone() const {
  return std::unique_ptr<AllDayEvent>(clone_impl());
}

TimePoint AllDayEvent::getStart() const { return m_start; }

TimePoint AllDayEvent::getEnd() const { return m_end; }

long AllDayEvent::days() const {
  return static_cast<long>((m_end - m_start).count() / 86400);
}

void AllDayEvent::setStart(const TimePoint start) {
  const Duration span = m_end - m_start;
  m_start = start;
  m_end = start + span;
}

void AllDayEvent::setEnd(const TimePoint end) {
  if (end <= m_start) {
    throw std::invalid_argument(
        "La fine deve essere successiva all'inizio (almeno un giorno).");
  }
  m_end = end;
}

bool AllDayEvent::isAllDay() const { return true; }

std::vector<Occurrence> AllDayEvent::occurrencesIn(const TimePoint from,
                                                   const TimePoint to) const {
  // Intervallo [m_start, m_end): presente se interseca [from, to]
  if (m_end > from && m_start <= to) {
    return {Occurrence{this, m_start, m_end - m_start}};
  }
  return {};
}

void AllDayEvent::moveTo(const TimePoint newStart) {
  const Duration span = m_end - m_start;
  m_start = newStart;
  m_end = newStart + span;
}

String AllDayEvent::describe() const {
  return "Tutto il giorno: " + getTitle() + " dal " +
         formatDateTime(m_start) + " al " + formatDateTime(m_end);
}

void AllDayEvent::accept(ActivityVisitor &visitor) const {
  visitor.visit(*this);
}

} // namespace events