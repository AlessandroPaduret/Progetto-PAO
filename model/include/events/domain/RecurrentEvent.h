#ifndef RECURRENTEVENT_H
#define RECURRENTEVENT_H

#include <vector>
#include <chrono>
#include <memory>
#include <iostream>
#include <unordered_set>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/GroupSchedulable.h"
#include "events/domain/Event.h"

namespace events {

class RecurrentEvent : public GroupSchedulable<Event> {
private:
    std::shared_ptr<DateGenerator> m_generator;
    Event m_templateEvent;  // Evento template da cui generare le occorrenze
    std::unordered_set<TimePoint, TimePointHasher> m_exceptions; // occorrenze escluse
public:

    /// Costruttore e distruttore

    /** @brief Costruttore con parametri opzionali.
     * @param generator Generatore di date di ricorrenza
     * @param templateEvent Evento template da cui generare le occorrenze (default: evento vuoto)
    */
    RecurrentEvent(std::shared_ptr<DateGenerator> generator, Event templateEvent = Event());

    /** @brief Operatore di output per stampare i dettagli dell'evento ricorrente */
    friend std::ostream& operator<<(std::ostream& os, const RecurrentEvent& event);

    /** @brief Aggiunge un'eccezione a una specifica occorrenza dell'evento ricorrente 
     * @param tp TimePoint rappresentante la data di ricorrenza specifica da escludere
    */
    void addException(TimePoint tp);

    /** @brief Elimina tutte le eccezioni associate a una specifica occorrenza dell'evento ricorrente
    * @param tp TimePoint rappresentante la data di ricorrenza specifica a cui rimuovere tutte le eccezioni
    */
    void deleteExceptions(TimePoint tp);

    /// Implementazione dei metodi virtuali di GroupSchedulable

    /** @brief Restituisce le occorrenze in un intervallo di tempo 
     * @param from Inizio dell'intervallo
     * @param to Fine dell'intervallo
     * @return Vettore di puntatori unici a Schedulable nell'intervallo specificato
    */
    std::vector<std::unique_ptr<Event>> getSchedulable(const TimePoint from, const TimePoint to) const override;
};

} // namespace events

#endif
