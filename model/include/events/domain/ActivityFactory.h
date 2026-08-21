#ifndef ACTIVITY_FACTORY_H
#define ACTIVITY_FACTORY_H

#include <chrono>
#include <memory>

#include "events/core/CommonTypes.h"
#include "events/domain/Anniversary.h"
#include "events/domain/Event.h"
#include "events/domain/Meeting.h"
#include "events/domain/RecurrentEvent.h"
#include "events/domain/Task.h"

namespace events {

/**
 * @brief Factory dei percorsi di creazione canonici delle attivita'.
 */
class ActivityFactory {
public:
    /** @brief Crea un evento con i parametri specificati
     *  @param title Il titolo dell'evento
     *  @param start Il momento di inizio dell'evento
     *  @param duration La durata dell'evento
     *  @return Un puntatore unico all'evento creato
     */
    static std::unique_ptr<Event> createSimpleEvent(const String& title, TimePoint start, Duration duration);

    /** @brief Crea un evento ricorrente con i parametri specificati
     *  @param title Il titolo dell'evento ricorrente
     *  @param start Il momento di inizio del primo evento
     *  @param duration La durata di ogni evento
     *  @param interval L'intervallo di ricorrenza (es. ogni 7 giorni)
     *  @return Un puntatore unico all'evento ricorrente creato
     */
    static std::unique_ptr<RecurrentEvent> createRecurrentEvent(const String& title, TimePoint start, Duration duration, Duration interval);

    /** @brief Crea un evento ricorrente settimanale semplice (ogni settimana nello stesso giorno e ora)
     *  @param title Il titolo dell'evento ricorrente
     *  @param start Il momento di inizio del primo evento (definisce anche giorno e ora della ricorrenza)
     *  @param duration La durata di ogni evento
     *  @param end Il momento di fine della ricorrenza (non generera' eventi dopo questa data)
     *  @return Un puntatore unico all'evento ricorrente creato
     */
    static std::unique_ptr<RecurrentEvent> createSimpleWeekly(const String& title, TimePoint start, Duration duration, TimePoint end);

    /** @brief Crea una riunione con luogo e partecipanti opzionali
     *  @param title Il titolo della riunione
     *  @param start Il momento di inizio
     *  @param duration La durata
     *  @param location Luogo o link (default: vuoto)
     *  @return Un puntatore unico alla riunione creata
     */
    static std::unique_ptr<Meeting> createMeeting(const String& title, TimePoint start, Duration duration, const String& location = "");

    /** @brief Crea un compito con scadenza e priorita'
     *  @param title Il titolo del compito
     *  @param due La scadenza
     *  @param priority La priorita' (default: Medium)
     *  @return Un puntatore unico al compito creato
     */
    static std::unique_ptr<Task> createTask(const String& title, TimePoint due, Priority priority = Priority::Medium);

    /** @brief Crea un anniversario annuale (gestione anni bisestili inclusa).
     *  @param title Il titolo (es. "Mario - Compleanno")
     *  @param date La data base (mezzanotte; giorno/mese definiscono la ricorrenza)
     *  @return Un puntatore unico all'anniversario creato
     */
    static std::unique_ptr<Anniversary> createAnniversary(const String& title, TimePoint date);
};

} // namespace events

#endif // ACTIVITY_FACTORY_H