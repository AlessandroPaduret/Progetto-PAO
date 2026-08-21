#include <chrono>
#include <memory>

#include "events/core/CommonTypes.h"
#include "events/domain/ActivityFactory.h"
#include "events/domain/Anniversary.h"
#include "events/domain/Event.h"
#include "events/domain/Meeting.h"
#include "events/domain/RecurrentEvent.h"
#include "events/domain/Task.h"
#include "events/generators/FixedIntervalGenerator.h"

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
ActivityFactory::createSimpleWeekly(const String &title, TimePoint start,
                                    Duration duration, TimePoint end) {
  auto gen =
      std::make_shared<FixedIntervalGenerator>(start, std::chrono::hours(24 * 7), end);
  return std::make_unique<RecurrentEvent>(gen, Event(title, start, duration));
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

std::unique_ptr<Anniversary> ActivityFactory::createAnniversary(
    const String &title, TimePoint date) {
  return std::make_unique<Anniversary>(title, date);
}

} // namespace events