#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/domain/Meeting.h"

namespace events {

Meeting::Meeting(const String &title, const TimePoint start,
                 const Duration duration, const String &location)
    : Activity(title), m_start(start), m_duration(duration),
      m_location(location) {
  if (duration < Duration::zero()) {
    throw std::invalid_argument("La durata non puo' essere negativa.");
  }
}

Meeting *Meeting::clone_impl() const { return new Meeting(*this); }

std::unique_ptr<Meeting> Meeting::clone() const {
  return std::unique_ptr<Meeting>(clone_impl());
}

TimePoint Meeting::getStart() const { return m_start; }

Duration Meeting::getDuration() const { return m_duration; }

TimePoint Meeting::getEnd() const { return m_start + m_duration; }

void Meeting::setStart(const TimePoint start) { m_start = start; }

void Meeting::setDuration(const Duration duration) {
  if (duration < Duration::zero()) {
    throw std::invalid_argument("La durata non puo' essere negativa.");
  }
  m_duration = duration;
}

String Meeting::getLocation() const { return m_location; }

void Meeting::setLocation(const String &location) { m_location = location; }

size_t Meeting::attendeeCount() const { return m_attendees.size(); }

const std::vector<String> &Meeting::getAttendees() const { return m_attendees; }

bool Meeting::addAttendee(const String &attendee) {
  if (std::find(m_attendees.begin(), m_attendees.end(), attendee) !=
      m_attendees.end()) {
    return false;
  }
  m_attendees.push_back(attendee);
  return true;
}

bool Meeting::removeAttendee(const String &attendee) {
  const auto it =
      std::find(m_attendees.begin(), m_attendees.end(), attendee);
  if (it == m_attendees.end()) {
    return false;
  }
  m_attendees.erase(it);
  return true;
}

std::vector<Occurrence> Meeting::occurrencesIn(const TimePoint from,
                                               const TimePoint to) const {
  if (m_start >= from && m_start <= to) {
    return {Occurrence{this, m_start, m_duration}};
  }
  return {};
}

void Meeting::moveTo(const TimePoint newStart) { m_start = newStart; }

String Meeting::describe() const {
  String out = "Riunione: " + getTitle() + " - " + formatDateTime(m_start) +
               ", durata " + formatDuration(m_duration);
  if (!m_location.empty()) {
    out += ", luogo " + m_location;
  }
  if (!m_attendees.empty()) {
    out += " (" + std::to_string(m_attendees.size()) + " partecipanti)";
  }
  return out;
}

void Meeting::accept(ActivityVisitor &visitor) const { visitor.visit(*this); }

} // namespace events