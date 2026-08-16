#include <chrono>
#include <memory>

#include "events/core/CommonTypes.h"
#include "events/domain/ActivityFactory.h"
#include "events/domain/Deadline.h"
#include "events/domain/Event.h"
#include "events/domain/RecurrentEvent.h"
#include "events/domain/Reminder.h"
#include "events/generators/FixedIntervalGenerator.h"
#include "events/generators/YearlyGenerator.h"

namespace events {

std::unique_ptr<Event> ActivityFactory::createSimpleEvent(const String &title,
                                                          TimePoint start,
                                                          Duration duration) {
  return std::make_unique<Event>(title, start, duration);
}

std::unique_ptr<RecurrentEvent>
ActivityFactory::createRecurrentEvent(const String &title, TimePoint start,
                                      Duration duration, Duration interval) {
  auto gen = std::make_shared<FixedIntervalGenerator>(start, interval);
  return std::make_unique<RecurrentEvent>(gen, Event(title, start, duration));
}

std::unique_ptr<RecurrentEvent>
ActivityFactory::createBirthday(const String &name,
                                std::chrono::year_month_day date) {

  std::chrono::sys_days midnightDays = std::chrono::sys_days{date};

  // Converti nel TimePoint usato dal modello (precisione secondi)
  TimePoint midnightStart = std::chrono::time_point_cast<Duration>(midnightDays);

  // Il generatore e il template condividono la stessa mezzanotte
  auto gen = std::make_shared<YearlyGenerator>(midnightStart);

  // Durata di un giorno meno 1 secondo per evitare sovrapposizioni
  Event templateEvent(name + " - Compleanno", midnightStart,
                      std::chrono::hours(24) - std::chrono::seconds(1));

  return std::make_unique<RecurrentEvent>(gen, std::move(templateEvent));
}

std::unique_ptr<RecurrentEvent>
ActivityFactory::createSimpleWeekly(const String &title, TimePoint start,
                                    Duration duration, TimePoint end) {
  auto gen =
      std::make_shared<FixedIntervalGenerator>(start, std::chrono::hours(24 * 7), end);
  return std::make_unique<RecurrentEvent>(gen, Event(title, start, duration));
}

std::unique_ptr<Deadline> ActivityFactory::createDeadline(const String &title,
                                                          TimePoint due,
                                                          Priority priority) {
  return std::make_unique<Deadline>(title, due, priority);
}

std::unique_ptr<Reminder> ActivityFactory::createReminder(const String &title,
                                                          TimePoint trigger,
                                                          const String &message,
                                                          Duration repeat) {
  return std::make_unique<Reminder>(title, trigger, message, repeat);
}

} // namespace events
