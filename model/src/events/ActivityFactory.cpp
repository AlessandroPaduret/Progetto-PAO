#include <chrono>
#include <memory>

#include "events/core/CommonTypes.h"
#include "events/domain/ActivityFactory.h"
#include "events/generators/FixedIntervalGenerator.h"
#include "events/generators/YearlyGenerator.h"

namespace events {

std::unique_ptr<Activity> ActivityFactory::createSimpleEvent(const String &title,
                                                             TimePoint start,
                                                             Duration duration) {
  return std::make_unique<Activity>(title, start, duration);
}

std::unique_ptr<Activity> ActivityFactory::createRecurrentEvent(
    const String &title, TimePoint start, Duration duration, Duration interval) {
  auto gen = std::make_shared<FixedIntervalGenerator>(start, interval);
  return std::make_unique<Activity>(title, start, duration, gen);
}

std::unique_ptr<Activity> ActivityFactory::createSimpleWeekly(
    const String &title, TimePoint start, Duration duration, TimePoint end) {
  auto gen = std::make_shared<FixedIntervalGenerator>(
      start, std::chrono::hours(24 * 7), end);
  return std::make_unique<Activity>(title, start, duration, gen);
}

std::unique_ptr<Meeting> ActivityFactory::createMeeting(
    const String &title, TimePoint start, Duration duration,
    const String &location) {
  return std::make_unique<Meeting>(title, start, duration, location);
}

std::unique_ptr<Task> ActivityFactory::createTask(const String &title,
                                                  TimePoint due,
                                                  Priority priority) {
  return std::make_unique<Task>(title, due, priority);
}

std::unique_ptr<Activity> ActivityFactory::createAnniversary(
    const String &title, TimePoint date) {
  // L'anniversario occupa l'intero giorno (mezzanotte - 1 secondo) ed e' una
  // Activity con YearlyGenerator (gestione anni bisestili: 29/2 -> 28/2).
  auto gen = std::make_shared<YearlyGenerator>(date);
  return std::make_unique<Activity>(
      title, date, std::chrono::hours(24) - std::chrono::seconds(1), gen);
}

} // namespace events