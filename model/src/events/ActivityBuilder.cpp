#include "events/builders/ActivityBuilder.h"
#include "events/generators/MaxOccurrencesDecorator.h"
#include "events/generators/SingleGenerator.h"
#include <memory>
#include <utility>

namespace events {

ActivityBuilder::ActivityBuilder(String title, TimePoint start)
    : m_title(std::move(title)), m_start(start) {}

std::unique_ptr<DateGenerator> ActivityBuilder::resolveGenerator() {
  if (m_generator) {
    return std::move(m_generator); // Sposta il generatore svuotando m_generator
  }
  return std::make_unique<SingleGenerator>(m_start);
}

ActivityBuilder &ActivityBuilder::withDuration(Duration duration) {
  m_duration = duration;
  return *this;
}

ActivityBuilder &ActivityBuilder::addGenerator(std::unique_ptr<DateGenerator> generator) {
  m_generator = std::move(generator);
  return *this;
}

ActivityBuilder &ActivityBuilder::addException(TimePoint tp) {
  m_exceptions.insert(tp);
  return *this;
}

ActivityBuilder &ActivityBuilder::withMaxOccurrences(std::size_t maxOccurrences) {
  if (m_generator) {
    m_generator = std::make_unique<MaxOccurrencesDecorator>(std::move(m_generator), maxOccurrences);
  }
  return *this;
}

Activity ActivityBuilder::build() {
  Activity activity(m_title, m_duration, resolveGenerator());
  for (const TimePoint tp : m_exceptions) {
    activity.addException(tp);
  }
  return activity;
}

// --- TaskBuilder ---

TaskBuilder::TaskBuilder(String title, TimePoint due)
    : ActivityBuilder(std::move(title), due) {}

TaskBuilder &TaskBuilder::withPriority(Priority priority) {
  m_priority = priority;
  return *this;
}


Task TaskBuilder::build() {
  Task task(m_title, m_start, m_priority, resolveGenerator());
  task.setDone(m_done);
  for (const TimePoint tp : m_exceptions) {
    task.addException(tp);
  }
  return task;
}

// --- MeetingBuilder ---

MeetingBuilder::MeetingBuilder(String title, TimePoint start)
    : ActivityBuilder(std::move(title), start) {}

MeetingBuilder &MeetingBuilder::withLocation(const String &location) {
  m_location = location;
  return *this;
}

MeetingBuilder &MeetingBuilder::addAttendee(const String &attendee) {
  m_attendees.push_back(attendee);
  return *this;
}

Meeting MeetingBuilder::build() {
  Meeting meeting(m_title, m_duration, m_location, resolveGenerator());
  for (const String &attendee : m_attendees) {
    meeting.addAttendee(attendee);
  }
  for (const TimePoint tp : m_exceptions) {
    meeting.addException(tp);
  }
  return meeting;
}

} // namespace events