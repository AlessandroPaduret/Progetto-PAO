#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/Format.h"
#include "events/domain/Meeting.h"

namespace events {

Meeting::Meeting(const String &title, const TimePoint start,
                 const Duration duration, const String &location,
                 std::shared_ptr<DateGenerator> generator)
    : Activity(title, start, duration, std::move(generator)),
      m_location(location) {}

Meeting *Meeting::clone_impl() const { return new Meeting(*this); }

std::unique_ptr<Meeting> Meeting::clone() const {
  return std::unique_ptr<Meeting>(clone_impl());
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
  const auto it = std::find(m_attendees.begin(), m_attendees.end(), attendee);
  if (it == m_attendees.end()) {
    return false;
  }
  m_attendees.erase(it);
  return true;
}

String Meeting::describe() const {
  String out = "Riunione: " + getTitle() + " - " + formatDateTime(getStart()) +
               ", durata " + formatDuration(getDuration());
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