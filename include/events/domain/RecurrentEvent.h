#ifndef RECURRENTEVENT_H
#define RECURRENTEVENT_H

#include <vector>
#include <chrono>
#include <memory>
#include <iostream>

#include "events/core/CommonTypes.h"
#include "events/core/GroupSchedulable.h"
#include "events/domain/Event.h"
#include "events/core/RecurrenceStrategy.h"

class RecurrentEvent : public GroupSchedulable<Event> {
private:
    RecurrenceStrategy<Event> m_recurrenceStrategy;  // Strategia di ricorrenza per generare le date di ricorrenza
    Event m_templateEvent;  // Evento template da cui generare le occorrenze
public:

    /// Costruttore e distruttore

    /** @brief Costruttore con parametri opzionali.
     * @param recurrenceStrategy Strategia di ricorrenza da utilizzare per generare le date di ricorrenza
     * @param templateEvent Evento template da cui generare le occorrenze (default: evento vuoto)
    */
    RecurrentEvent(RecurrenceStrategy<Event> recurrenceStrategy, Event templateEvent = Event());

    /** @brief Operatore di output per stampare i dettagli dell'evento ricorrente */
    friend std::ostream& operator<<(std::ostream& os, const RecurrentEvent& event);

    /** @brief Aggiunge una modifica a una specifica occorrenza dell'evento ricorrente 
     * @param tp TimePoint rappresentante la data di ricorrenza specifica a cui applicare la modifica
     * @param modifiedEvent Evento modificato da associare a quella data di ricorrenza (es. titolo diverso, orario spostato, ecc.)
    */
    void addModification(TimePoint tp, std::unique_ptr<Event> modifiedEvent);

    /** @brief Elimina tutte le modifiche associate a una specifica occorrenza dell'evento ricorrente
     * @param tp TimePoint rappresentante la data di ricorrenza specifica a cui rimuovere tutte le modifiche
     */
    void deleteModifications(TimePoint tp);

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

    /** @brief Imposta la regola di ricorrenza a partire da un certo momento (non ci possono essere 2 regole per lo stesso intervallo)
     * @param next Evento ricorrente successivo
    */
    //void setNextRuleset(std::unique_ptr<Repeatable<Event>> next) override;
};

#endif