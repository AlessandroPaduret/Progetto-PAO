#include <memory>
#include <utility>

#include "events/core/Activity.h"

namespace events {

Activity::Activity(String title) : m_title(std::move(title)) {}

String Activity::getTitle() const { return m_title; }

void Activity::setTitle(const String &title) { m_title = title; }

bool Activity::isDone() const { return m_done; }

void Activity::setDone(const bool done) { m_done = done; }

bool Activity::isDoneAt(const TimePoint) const { return m_done; }

void Activity::setDoneAt(const TimePoint, const bool done) { m_done = done; }

std::unique_ptr<Activity> Activity::clone() const {
  return std::unique_ptr<Activity>(clone_impl());
}

} // namespace events
