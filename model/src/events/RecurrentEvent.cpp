#include <iostream>
#include <memory>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/domain/RecurrentEvent.h"

namespace events {

// Costruttore: Inizializza il generatore e l'evento "stampino" (template)
RecurrentEvent::RecurrentEvent(std::shared_ptr<DateGenerator> generator,
                               Event templateEvent)
    : m_generator(std::move(generator)),
      m_templateEvent(std::move(templateEvent)) {}

      // Operatore di output per debug/stampa
std::ostream &operator<<(std::ostream &os, const events::RecurrentEvent &event) {
  os << "[Recurrent Event]\n" << event.m_templateEvent << "\n"
      << event.m_generator->describe()
      << "\nwith " << event.m_exceptions.size() << " exceptions";
  return os;
}

std::vector<std::unique_ptr<Event>>
RecurrentEvent::getSchedulable(const TimePoint from, const TimePoint to) const {

  std::vector<std::unique_ptr<Event>> result;

  // Scorri le date generate dal generatore nell'intervallo specificato
  std::vector<TimePoint> dates = m_generator->generateDates(from, to);

  for (const TimePoint tp : dates) {

    // Salta le occorrenze segnate come eccezioni
    if (m_exceptions.find(tp) != m_exceptions.end()) {
      continue;
    }

    // Crea un'istanza standard basata sull'evento template
    std::unique_ptr<Event> standardOccurrence = m_templateEvent.clone();
    standardOccurrence->setStart(tp);
    result.push_back(std::move(standardOccurrence));
  }

  return result;
}

void RecurrentEvent::addException(TimePoint tp) {
  m_exceptions.insert(tp);
}

void RecurrentEvent::deleteExceptions(TimePoint tp) {
  m_exceptions.erase(tp);
}

} // namespace events
