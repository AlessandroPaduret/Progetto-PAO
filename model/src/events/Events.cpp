#include <algorithm>
#include <memory>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/domain/Events.h"

namespace events {

void Events::addEvent(const Event& event) {
    m_events.push_back(event);
}

size_t Events::size() const {
    return m_events.size();
}

std::vector<std::unique_ptr<Event>>
Events::getSchedulable(const TimePoint from, const TimePoint to) const {
    std::vector<std::unique_ptr<Event>> result;

    // Stessa semantica dei generatori: include gli eventi il cui inizio è in [from, to]
    for (const Event& e : m_events) {
        if (e.getStart() >= from && e.getStart() <= to) {
            result.push_back(e.clone());
        }
    }

    // Ordina per data di inizio per una timeline coerente
    std::sort(result.begin(), result.end(),
              [](const std::unique_ptr<Event>& a, const std::unique_ptr<Event>& b) {
                  return a->getStart() < b->getStart();
              });

    return result;
}

std::ostream& operator<<(std::ostream& os, const Events& events) {
    os << "[Events] " << events.m_events.size() << " events\n";
    for (const Event& e : events.m_events) {
        os << e << "\n";
    }
    return os;
}

} // namespace events
