#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/domain/Reminder.h"
#include "events/generators/FixedIntervalGenerator.h"

namespace events {

Reminder::Reminder(const String &title, const TimePoint trigger,
                   const String &message, const Duration repeat)
    : Activity(title), m_trigger(trigger), m_message(message),
      m_repeat(repeat) {
  if (repeat < Duration::zero()) {
    throw std::invalid_argument("L'intervallo di ripetizione non puo' essere negativo.");
  }
}

Reminder *Reminder::clone_impl() const { return new Reminder(*this); }

std::unique_ptr<Reminder> Reminder::clone() const {
  return std::unique_ptr<Reminder>(clone_impl());
}

std::ostream &operator<<(std::ostream &os, const Reminder &reminder) {
  return os << "[Promemoria]\n"
            << reminder.getTitle() << "\n"
            << "Attivazione: " << reminder.m_trigger << "\n"
            << "Messaggio: " << reminder.m_message << "\n"
            << "Ripetizione: "
            << (reminder.isRepeating() ? formatDuration(reminder.m_repeat)
                                       : "una tantum")
            << "\n";
}

TimePoint Reminder::getTrigger() const { return m_trigger; }

void Reminder::setTrigger(const TimePoint trigger) { m_trigger = trigger; }

String Reminder::getMessage() const { return m_message; }

void Reminder::setMessage(const String &message) { m_message = message; }

Duration Reminder::getRepeatInterval() const { return m_repeat; }

void Reminder::setRepeatInterval(const Duration repeat) {
  if (repeat < Duration::zero()) {
    throw std::invalid_argument("L'intervallo di ripetizione non puo' essere negativo.");
  }
  m_repeat = repeat;
}

bool Reminder::isRepeating() const { return m_repeat > Duration::zero(); }

void Reminder::snooze(const Duration delay) { m_trigger += delay; }

TimePoint Reminder::getStart() const { return m_trigger; }

std::vector<Occurrence> Reminder::occurrencesIn(const TimePoint from,
                                                const TimePoint to) const {
  std::vector<Occurrence> result;
  if (!isRepeating()) {
    if (m_trigger >= from && m_trigger <= to) {
      result.push_back(Occurrence{this, m_trigger, Duration::zero()});
    }
    return result;
  }

  // Riutilizza la logica del generatore a intervalli fissi (composizione)
  const FixedIntervalGenerator generator(m_trigger, m_repeat);
  for (const TimePoint tp : generator.generateDates(from, to)) {
    result.push_back(Occurrence{this, tp, Duration::zero()});
  }
  return result;
}

String Reminder::describe() const {
  String out = "Promemoria: " + getTitle() + " - " + formatDateTime(m_trigger);
  if (isRepeating()) {
    out += ", si ripete ogni " + formatDuration(m_repeat);
  }
  if (!m_message.empty()) {
    out += " (\"" + m_message + "\")";
  }
  return out;
}

void Reminder::accept(ActivityVisitor &visitor) const { visitor.visit(*this); }

} // namespace events
