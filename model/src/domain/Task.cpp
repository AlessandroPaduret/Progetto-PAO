#include "domain/Task.h"

#include <utility>

#include "core/ActivityVisitor.h"
#include "core/Format.h"

namespace events {

Task::Task(String title,
           TimePoint due,
           Duration duration,
           Priority priority,
           std::shared_ptr<const DateGenerator> generator,
           TimePoint end)
    : Activity(std::move(title), due, duration, std::move(generator), end),
      m_priority(priority) {}

std::unique_ptr<Activity> Task::clone() const {
    return std::make_unique<Task>(*this);
}

bool Task::isDone(TimePoint tp) const {
    return m_doneOccurrences.find(tp) != m_doneOccurrences.end();
}

void Task::setDone(TimePoint tp, bool done) {
    if (done) {
        m_doneOccurrences.insert(tp);
    } else {
        m_doneOccurrences.erase(tp);
    }
}

bool Task::isDone() const {
    return isDone(getStart());
}

bool Task::setDone(bool done) {
    setDone(getStart(), done);
    return true;
}

bool Task::isOverdue(TimePoint tp, TimePoint now) const {
    return !isDone(tp) && now > tp;
}

Duration Task::timeRemaining(TimePoint tp, TimePoint now) const {
    return tp - now;
}

String Task::priorityLabel(Priority priority) {
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

String Task::describe() const {
    String out = "Compito: " + getTitle() + " - priorita' " +
                 priorityLabel(m_priority) + ", scade il " + formatDateTime(getStart());
    if (isDone()) {
        out += " (evaso)";
    }
    return out;
}

void Task::accept(ActivityVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace events