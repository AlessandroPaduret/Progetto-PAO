#include <iostream>
#include <memory>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/domain/Deadline.h"

namespace events {

Deadline::Deadline(const String &title, const TimePoint due,
                   const Priority priority)
    : Activity(title), m_due(due), m_priority(priority), m_done(false) {}

Deadline *Deadline::clone_impl() const { return new Deadline(*this); }

std::unique_ptr<Deadline> Deadline::clone() const {
  return std::unique_ptr<Deadline>(clone_impl());
}

std::ostream &operator<<(std::ostream &os, const Deadline &deadline) {
  return os << "[Scadenza]\n"
            << deadline.getTitle() << "\n"
            << "Scade: " << deadline.m_due << "\n"
            << "Priorita': " << Deadline::priorityLabel(deadline.m_priority) << "\n"
            << "Evasa: " << (deadline.m_done ? "si" : "no") << "\n";
}

TimePoint Deadline::getDue() const { return m_due; }

void Deadline::setDue(const TimePoint due) { m_due = due; }

Priority Deadline::getPriority() const { return m_priority; }

void Deadline::setPriority(const Priority priority) { m_priority = priority; }

bool Deadline::isDone() const { return m_done; }

void Deadline::setDone(const bool done) { m_done = done; }

bool Deadline::isOverdue(const TimePoint now) const {
  return !m_done && now > m_due;
}

Duration Deadline::timeRemaining(const TimePoint now) const {
  return m_due - now;
}

String Deadline::priorityLabel(const Priority priority) {
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

TimePoint Deadline::getStart() const { return m_due; }

std::vector<Occurrence> Deadline::occurrencesIn(const TimePoint from,
                                                const TimePoint to) const {
  if (m_due >= from && m_due <= to) {
    return {Occurrence{this, m_due, Duration::zero()}};
  }
  return {};
}

String Deadline::describe() const {
  String out = "Scadenza: " + getTitle() + " - priorita' " +
               priorityLabel(m_priority) + ", scade il " + formatDateTime(m_due);
  if (m_done) {
    out += " (evasa)";
  }
  return out;
}

void Deadline::accept(ActivityVisitor &visitor) const { visitor.visit(*this); }

} // namespace events
