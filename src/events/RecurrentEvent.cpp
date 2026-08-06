#include <iostream>
#include <memory>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/domain/RecurrentEvent.h"

namespace events {

// Costruttore: Inizializza il generatore (garantendo il decoratore delle
// eccezioni) e l'evento "stampino" (template)
RecurrentEvent::RecurrentEvent(std::shared_ptr<DateGenerator> generator,
                               Event templateEvent)
    : m_generator(std::move(generator)),
      m_templateEvent(std::move(templateEvent)) {
  // assicuro che il generatore sia decorato con ExceptionGenerator,
  // così addException/deleteExceptions funzionano sempre
  if (!dynamic_cast<ExceptionGenerator *>(m_generator.get())) {
    m_generator = std::make_shared<ExceptionGenerator>(m_generator);
  }
}

      // Operatore di output per debug/stampa
std::ostream &operator<<(std::ostream &os, const events::RecurrentEvent &event) {
  os << "[Recurrent Event]\n" << event.m_templateEvent << "\n"
      << event.m_generator->describe();
  return os;
}

std::vector<std::unique_ptr<Event>>
RecurrentEvent::getSchedulable(const TimePoint from, const TimePoint to) const {

  std::vector<std::unique_ptr<Event>> result;

  // Scorri le date generate dal generatore nell'intervallo specificato
  std::vector<TimePoint> dates = m_generator->generateDates(from, to);

  for (const TimePoint tp : dates) {

    // Crea un'istanza standard basata sull'evento template
    std::unique_ptr<Event> standardOccurrence = m_templateEvent.clone();
    standardOccurrence->setStart(tp);
    result.push_back(std::move(standardOccurrence));
  }

  return result;
}

void RecurrentEvent::addException(TimePoint tp) {
  static_cast<ExceptionGenerator *>(m_generator.get())->addException(tp);
}

void RecurrentEvent::deleteExceptions(TimePoint tp) {
  static_cast<ExceptionGenerator *>(m_generator.get())->deleteException(tp);
}

} // namespace events
