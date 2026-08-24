#include <memory>
#include <utility>

#include "events/builders/ActivityBuilder.h"
#include "events/generators/SingleGenerator.h"

namespace events {

ActivityBuilder::ActivityBuilder(String title, TimePoint start)
    : m_title(std::move(title)), m_start(start) {}

std::shared_ptr<DateGenerator> ActivityBuilder::resolveGenerator() const {
  if (m_generator) {
    return m_generator;
  }
  return std::make_shared<SingleGenerator>(m_start);
}

ActivityBuilder &ActivityBuilder::withDuration(Duration duration) {
  m_duration = duration;
  return *this;
}

ActivityBuilder &ActivityBuilder::addGenerator(
    std::shared_ptr<DateGenerator> generator) {
  m_generator = std::move(generator);
  return *this;
}

ActivityBuilder &ActivityBuilder::addException(TimePoint tp) {
  m_exceptions.insert(tp);
  return *this;
}

Activity ActivityBuilder::build() const {
  Activity activity(m_title, m_duration, resolveGenerator());
  for (const TimePoint tp : m_exceptions) {
    activity.addException(tp);
  }
  return activity;
}

TaskBuilder::TaskBuilder(String title, TimePoint due)
    : ActivityBuilder(std::move(title), due) {}

TaskBuilder &TaskBuilder::withPriority(Priority priority) {
  m_priority = priority;
  return *this;
}

TaskBuilder &TaskBuilder::makeCheckable() {
  // I compiti sono sempre spuntabili: il metodo e' fornito per uniformita'.
  return *this;
}

Task TaskBuilder::build() const {
  Task task(m_title, m_start, m_priority, resolveGenerator());
  task.setDone(m_done);
  for (const TimePoint tp : m_exceptions) {
    task.addException(tp);
  }
  return task;
}

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

Meeting MeetingBuilder::build() const {
  Meeting meeting(m_title, m_start, m_duration, m_location, resolveGenerator());
  for (const String &attendee : m_attendees) {
    meeting.addAttendee(attendee);
  }
  for (const TimePoint tp : m_exceptions) {
    meeting.addException(tp);
  }
  return meeting;
}

} // namespace events