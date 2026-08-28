#include <chrono>
#include <memory>
#include <unordered_set>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/domain/Task.h"
#include "events/generators/SingleGenerator.h"

namespace events {

Task::Task(const String &title, const TimePoint due, const Priority priority,
           std::unique_ptr<DateGenerator> generator)
    : Activity(title, Duration::zero(),
               generator ? std::move(generator)
                         : std::make_unique<SingleGenerator>(due)),
      m_priority(priority) {}

std::unique_ptr<Activity> Task::clone() const {
  auto copy = std::make_unique<Task>(getTitle(), getDue(), m_priority,
                                     getGenerator().clone());
  copy->m_doneOccurrences = m_doneOccurrences;
  return copy;
}

TimePoint Task::getDue() const { return getStart(); }

void Task::setDue(const TimePoint due) { moveTo(due); }

Priority Task::getPriority() const { return m_priority; }

void Task::setPriority(const Priority priority) { m_priority = priority; }

bool Task::isDone(const TimePoint tp) const {
  return m_doneOccurrences.find(tp) != m_doneOccurrences.end();
}

void Task::setDone(const TimePoint tp, const bool done) {
  if (done) {
    m_doneOccurrences.insert(tp);
  } else {
    m_doneOccurrences.erase(tp);
  }
}

bool Task::isDone() const { return isDone(getStart()); }

bool Task::setDone(const bool done) {
  if (!getGenerator().isIn(getStart())) {
    return false;
  }
  setDone(getStart(), done);
  return true;
}

bool Task::isCheckable() const { return true; }

bool Task::isChecked(const TimePoint tp) const { return isDone(tp); }

void Task::setChecked(const TimePoint tp, const bool checked) {
  setDone(tp, checked);
}

const std::unordered_set<TimePoint, TimePointHasher> &
Task::getDoneOccurrences() const {
  return m_doneOccurrences;
}

bool Task::isOverdue(const TimePoint tp, const TimePoint now) const {
  return !isDone(tp) && now > tp;
}

Duration Task::timeRemaining(const TimePoint tp,
                             const TimePoint now) const {
  return tp - now;
}

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