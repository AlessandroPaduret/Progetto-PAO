#ifndef ACTIVITY_FACTORY_H
#define ACTIVITY_FACTORY_H

#include <chrono>
#include <memory>

#include "events/core/CommonTypes.h"
#include "events/domain/Deadline.h"
#include "events/domain/Event.h"
#include "events/domain/RecurrentEvent.h"
#include "events/domain/Reminder.h"

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

    /** @brief Crea un evento ricorrente annuale per il compleanno di una persona
     *  @param name Il nome della persona (usato per il titolo dell'evento)
     *  @param date La data del compleanno (year_month_day)
     *  @return Un puntatore unico all'evento ricorrente creato
     */
    static std::unique_ptr<RecurrentEvent> createBirthday(const String& name, std::chrono::year_month_day date);

    /** @brief Crea un evento ricorrente settimanale semplice (ogni settimana nello stesso giorno e ora)
     *  @param title Il titolo dell'evento ricorrente
     *  @param start Il momento di inizio del primo evento (definisce anche giorno e ora della ricorrenza)
     *  @param duration La durata di ogni evento
     *  @param end Il momento di fine della ricorrenza (non generera' eventi dopo questa data)
     *  @return Un puntatore unico all'evento ricorrente creato
     */
    static std::unique_ptr<RecurrentEvent> createSimpleWeekly(const String& title, TimePoint start, Duration duration, TimePoint end);

    /** @brief Crea una scadenza con i parametri specificati
     *  @param title Il titolo della scadenza
     *  @param due L'istante di scadenza
     *  @param priority La priorita' (default: Medium)
     *  @return Un puntatore unico alla scadenza creata
     */
    static std::unique_ptr<Deadline> createDeadline(const String& title, TimePoint due, Priority priority = Priority::Medium);

    /** @brief Crea un promemoria con i parametri specificati
     *  @param title Il titolo del promemoria
     *  @param trigger L'istante di attivazione
     *  @param message Il messaggio del promemoria
     *  @param repeat L'intervallo di ripetizione (default: zero = una tantum)
     *  @return Un puntatore unico al promemoria creato
     */
    static std::unique_ptr<Reminder> createReminder(const String& title, TimePoint trigger, const String& message = "", Duration repeat = Duration::zero());
};

} // namespace events

#endif // ACTIVITY_FACTORY_H
