#include "events/domain/Meeting.h"

#include <algorithm>
#include <utility>

#include "events/core/ActivityVisitor.h"
#include "events/core/Format.h"

namespace events {

Meeting::Meeting(String title,
                 TimePoint start,
                 Duration duration,
                 String location,
                 std::shared_ptr<const DateGenerator> generator,
                 TimePoint end)
    : Activity(std::move(title), start, duration, std::move(generator), end),
      m_location(std::move(location)) {}

std::unique_ptr<Activity> Meeting::clone() const {
    return std::make_unique<Meeting>(*this);
}

bool Meeting::addAttendee(const String& attendee) {
    if (std::find(m_attendees.begin(), m_attendees.end(), attendee) != m_attendees.end()) {
        return false;
    }
    m_attendees.push_back(attendee);
    return true;
}

bool Meeting::removeAttendee(const String& attendee) {
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

void Meeting::accept(ActivityVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace events