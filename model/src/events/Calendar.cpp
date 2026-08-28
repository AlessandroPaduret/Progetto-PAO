#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/domain/Calendar.h"

namespace events {

Activity &Calendar::add(std::unique_ptr<Activity> activity) {
  if (!activity) {
    throw std::invalid_argument("Non si puo' aggiungere un'attivita' nulla.");
  }
  m_activities.push_back(std::move(activity));
  return *m_activities.back();
}

bool Calendar::remove(const Activity *activity) {
  const auto it = std::find_if(m_activities.begin(), m_activities.end(),
                               [activity](const std::unique_ptr<Activity> &a) {
                                 return a.get() == activity;
                               });
  if (it == m_activities.end()) {
    return false;
  }
  m_activities.erase(it);
  return true;
}

void Calendar::clear() { m_activities.clear(); }

size_t Calendar::size() const { return m_activities.size(); }

bool Calendar::empty() const { return m_activities.empty(); }

std::vector<Occurrence> Calendar::occurrencesIn(const TimePoint from,
                                                const TimePoint to) const {
  std::vector<Occurrence> result;
  for (const auto &activity : m_activities) {
    std::vector<Occurrence> occurrences = activity->occurrencesIn(from, to);
    result.insert(result.end(), occurrences.begin(), occurrences.end());
  }

  std::sort(result.begin(), result.end(),
            [](const Occurrence &a, const Occurrence &b) {
              return a.start < b.start;
            });
  return result;
}

std::vector<const Activity *> Calendar::search(const String &needle) const {
  auto lower = [](String s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
  };
  const String loweredNeedle = lower(needle);

  std::vector<const Activity *> result;
  for (const auto &activity : m_activities) {
    if (lower(activity->getTitle()).find(loweredNeedle) != String::npos) {
      result.push_back(activity.get());
    }
  }
  return result;
}

Calendar::const_iterator Calendar::begin() const { return m_activities.begin(); }

Calendar::const_iterator Calendar::end() const { return m_activities.end(); }

} // namespace events
