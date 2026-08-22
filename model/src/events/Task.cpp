#include <chrono>
#include <memory>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/domain/Task.h"

namespace events {

Task::Task(const String &title, const TimePoint due, const Priority priority)
    : Activity(title), m_due(due), m_priority(priority) {}

Task *Task::clone_impl() const { return new Task(*this); }

std::unique_ptr<Task> Task::clone() const {
  return std::unique_ptr<Task>(clone_impl());
}

TimePoint Task::getDue() const { return m_due; }

void Task::setDue(const TimePoint due) { m_due = due; }

Priority Task::getPriority() const { return m_priority; }

void Task::setPriority(const Priority priority) { m_priority = priority; }

bool Task::isDone() const { return m_done; }

void Task::setDone(const bool done) { m_done = done; }

bool Task::isOverdue(const TimePoint now) const {
  return !isDone() && now > m_due;
}

Duration Task::timeRemaining(const TimePoint now) const { return m_due - now; }

String Task::priorityLabel(const Priority priority) {
  switch (priority) {
  case Priority::Low:
    return "bassa";
  case Priority::High:
    return "alta";
  case Priority::Medium:
  default:
    return "media";
  }
}

TimePoint Task::getStart() const { return m_due; }

void Task::moveTo(const TimePoint newStart) { m_due = newStart; }

std::vector<Occurrence> Task::occurrencesIn(const TimePoint from,
                                            const TimePoint to) const {
  if (m_due >= from && m_due <= to) {
    return {Occurrence{this, m_due, Duration::zero()}};
  }
  return {};
}

String Task::describe() const {
  String out = "Compito: " + getTitle() + " - priorita' " +
               priorityLabel(m_priority) + ", scade il " + formatDateTime(m_due);
  if (isDone()) {
    out += " (evaso)";
  }
  return out;
}

void Task::accept(ActivityVisitor &visitor) const { visitor.visit(*this); }

} // namespace events