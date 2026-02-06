#ifndef RECURRENTEVENT_H
#define RECURRENTEVENT_H

#include <vector>
#include <chrono>
#include <string>
#include <iostream>
#include <unordered_set>
#include <unordered_map>


#include "Event.h"
#include "Repeatable.h"
#include "CommonTypes.h"


struct TimePointHasher {
    std::size_t operator()(const TimePoint& tp) const {
        return std::hash<long long>{}(tp.time_since_epoch().count());
    }
};

class RecurrentEvent : public Event, public Repeatable<Event> {
private:
    Duration m_mainInterval;
    TimePoint m_end;
    std::vector<Duration> m_offsets;                                        // assumo pochi offset quindi vectore semplice
    std::unordered_set<TimePoint, TimePointHasher> m_exceptions;            // assumo tante eccezione quindi accesso = O(1)
    std::unordered_map<TimePoint, Event, TimePointHasher> m_modifications;  // assumo tante modifiche quindi accesso = O(1)
    //std::unique_ptr<RecurrentEvent> m_nextRuleset;
public:

    /// Costruttore e distruttore

    /** @brief Costruttore con parametri opzionali.
     *  @param title Titolo dell'evento (default: stringa vuota)
     *  @param start Orario di inizio (default: ora attuale)
     *  @param duration Durata dell'evento (default: zero)
     *  @param recurrenceInterval Intervallo di ricorrenza (default: zero)
     *  @param end Data di fine della ricorrenza (default: massimo) */
    RecurrentEvent(const String& title = "", 
                   const TimePoint start = std::chrono::time_point_cast<Duration>(Clock::now()), 
                   const Duration duration = Duration::zero(),
                   const Duration mainInterval = Duration::zero(),
                   const TimePoint end = TimePoint::max(),
                   const std::vector<Duration>& offsets = {Duration::zero()});
    
    /** @brief Costruttore di copia
     * @param other Evento ricorrente da copiare
    */
    RecurrentEvent(const RecurrentEvent& other);

    /** @brief Operatore di assegnazione 
    * @param other Evento ricorrente da assegnare
    * @return Riferimento all'oggetto assegnato
    */
    RecurrentEvent& operator=(const RecurrentEvent& other);

    /** @brief Aggiunge un offset di ricorrenza 
     *  @param interval Intervallo di ricorrenza
    */
    void addOffset(const Duration interval);

    /** @brief Toglie un offset di ricorrenza 
     *  @param interval Intervallo di ricorrenza
    */
    void deleteOffset(const Duration interval);

    /** @brief Operatore di output per stampare i dettagli dell'evento ricorrente */
    friend std::ostream& operator<<(std::ostream& os, const RecurrentEvent& event);

    /** @brief Crea una copia dell'evento ricorrente */
    virtual std::unique_ptr<Event> clone() const override;

    /// Implementazione dei metodi virtuali di Schedulable

    /** @return Il punto temporale (data/ora) di fine della ricorrenza */
    TimePoint getEnd() const override;

    /** @param from Inizio del range 
     *  @param to Fine del range
     *  @return Se uno degli eventi della ricorrenza è compreso nel range specificato */
    bool isIn(const TimePoint from, const TimePoint to) const override;

    /** @param other Altro evento
     *  @return Se uno degli eventi della ricorrenza si sovrappone con un altro evento */
    bool overlapsWith(const Schedulable& other) const override;

    /// Implementazione dei metodi virtuali di Repeatable<Event>
    
    /** @brief Aggiunge un'eccezione 
     *  @param exception Eccezione da aggiungere
    */
    void addException(const TimePoint exception) override;

    /** @brief Toglie un'eccezione 
     * @param exception Eccezione da togliere
    */
    void deleteException(const TimePoint exception) override;
    
    /** @brief Aggiunge una modifica 
     * @param modifiedEvent Evento modificato da aggiungere
    */
    void addModification(const Event& modifiedEvent) override;

    /** @brief Toglie una modifica 
     * @param modifiedEvent Evento modificato da togliere
    */
    void deleteModification(const TimePoint modifiedEvent) override;

    /** @brief Verifica se un determinato istante è un'eccezione 
     * @param time Istante da verificare
    */
    bool isException(const TimePoint time) const override;

    /** @brief Verifica se un determinato istante è una modifica 
     * @param time Istante da verificare
    */
    bool isModified(const TimePoint time) const override;

    /** @brief Restituisce le occorrenze in un intervallo di tempo 
     * @param from Inizio dell'intervallo
     * @param to Fine dell'intervallo
     * @return Vettore di puntatori unici a Schedulable nell'intervallo specificato
    */
    std::vector<std::unique_ptr<Event>> getOccurrences(
        const TimePoint from,
        const TimePoint to) const override;
};

#endif