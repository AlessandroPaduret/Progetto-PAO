#include "domain/RecurrentEvent.h"
#include <vector>
#include <memory>
#include <iostream>

// Costruttore: Inizializza la strategia e l'evento "stampino" (template)
RecurrentEvent::RecurrentEvent(RecurrenceStrategy<Event> recurrenceStrategy, Event templateEvent)
    : m_recurrenceStrategy(std::move(recurrenceStrategy)), 
      m_templateEvent(std::move(templateEvent)) {}

std::vector<std::unique_ptr<Event>> RecurrentEvent::getSchedulable(const TimePoint from, const TimePoint to) const {
    
    std::vector<std::unique_ptr<Event>> result;

    // Scorri le date generate dalla strategia di ricorrenza nell'intervallo specificato
    std::vector<TimePoint> dates = m_recurrenceStrategy.generateDates(from, to);

    for (const TimePoint tp : dates) {
        
        // Verifica se esiste una modifica specifica per questa data di ricorrenza
        std::unique_ptr<Event> specificItem = m_recurrenceStrategy.getItem(tp);

        // Se esiste una modifica specifica, usala; altrimenti, crea un'istanza standard basata sull'evento template
        if (specificItem) {
            result.push_back(specificItem->clone());
        } else {
            std::unique_ptr<Event> standardOccurrence = m_templateEvent.clone();
            standardOccurrence->setStart(tp);
            result.push_back(std::move(standardOccurrence));
        }
    }

    return result;
}

// Operatore di output per debug/stampa
std::ostream& operator<<(std::ostream& os, const RecurrentEvent& event) {
    os << "[Recurrent Event] Template: " << event.m_templateEvent.getTitle() 
       << " | Duration: " << event.m_templateEvent.getDuration().count() << "s";
    return os;
}

void RecurrentEvent::addModification(TimePoint tp, std::unique_ptr<Event> modifiedEvent) {
    m_recurrenceStrategy.addModification(tp, std::move(modifiedEvent));
}

void RecurrentEvent::deleteModifications(TimePoint tp) {
    m_recurrenceStrategy.deleteModifications(tp);
}

void RecurrentEvent::addException(TimePoint tp) {
    m_recurrenceStrategy.addException(tp);
}

void RecurrentEvent::deleteExceptions(TimePoint tp) {
    m_recurrenceStrategy.deleteExceptions(tp);
}