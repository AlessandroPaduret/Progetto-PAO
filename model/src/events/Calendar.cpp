#include <algorithm>
#include <cctype>
#include <cstddef>
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

Activity* Calendar::find(const Activity* target) {
    if (!target) return nullptr;

    const auto it = std::ranges::find_if(m_activities, [target](const auto& ptr) {
        return ptr.get() == target;
    });

    return (it != m_activities.end()) ? it->get() : nullptr;
}

const Activity* Calendar::find(const Activity* target) const {
    if (!target) return nullptr;

    const auto it = std::ranges::find_if(m_activities, [target](const auto& ptr) {
        return ptr.get() == target;
    });

    return (it != m_activities.end()) ? it->get() : nullptr;
}


std::unique_ptr<Activity> Calendar::pop(const Activity* activity) {
    if (!activity) return nullptr;

    const auto it = std::ranges::find_if(m_activities, [activity](const auto& act) {
        return act.get() == activity;
    });

    if (it == m_activities.end()) {
        return nullptr;
    }

    std::unique_ptr<Activity> extracted = std::move(*it);
    m_activities.erase(it);
    return extracted;
}

void Calendar::clear() { m_activities.clear(); }

size_t Calendar::size() const { return m_activities.size(); }

bool Calendar::empty() const { return m_activities.empty(); }

std::vector<Occurrence> Calendar::occurrencesIn(TimePoint from, TimePoint to) const {
    std::vector<Occurrence> result;

    for (const auto& activity : m_activities) {
        auto occs = activity->occurrencesIn(from, to);
        result.insert_range(result.end(), occs | std::views::as_rvalue);
    }

    // Ordinamento C++20 con std::ranges::sort e proiezione su Occurrence::start
    std::ranges::sort(result, {}, &Occurrence::start);

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
