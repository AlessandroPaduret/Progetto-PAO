#include <chrono>
#include <memory>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/domain/Task.h"

namespace events {

Task::Task(const String &title, const TimePoint due, const Priority priority,
           std::shared_ptr<DateGenerator> generator)
    : Activity(title, due, Duration::zero(), std::move(generator)),
      m_priority(priority) {}

Task *Task::clone_impl() const { return new Task(*this); }

std::unique_ptr<Task> Task::clone() const {
  return std::unique_ptr<Task>(clone_impl());
}

TimePoint Task::getDue() const { return getStart(); }

void Task::setDue(const TimePoint due) { moveTo(due); }

Priority Task::getPriority() const { return m_priority; }

void Task::setPriority(const Priority priority) { m_priority = priority; }

bool Task::isDone() const { return m_done; }

void Task::setDone(const bool done) { m_done = done; }

bool Task::isCheckable() const { return true; }

bool Task::isChecked() const { return isDone(); }

void Task::setChecked(const bool checked) { setDone(checked); }

bool Task::isOverdue(const TimePoint now) const {
  return !isDone() && now > getStart();
}

Duration Task::timeRemaining(const TimePoint now) const { return getStart() - now; }

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

std::vector<Occurrence> Task::occurrencesIn(const TimePoint from,
                                            const TimePoint to) const {
  std::vector<Occurrence> result;
  for (const TimePoint tp : occurrenceDates(from, to)) {
    result.push_back(Occurrence{this, tp, Duration::zero()});
  }
  return result;
}

String Task::describe() const {
  String out = "Compito: " + getTitle() + " - priorita' " +
               priorityLabel(m_priority) + ", scade il " + formatDateTime(getStart());
  if (isDone()) {
    out += " (evaso)";
  }
  return out;
}

void Task::accept(ActivityVisitor &visitor) const { visitor.visit(*this); }

} // namespace events