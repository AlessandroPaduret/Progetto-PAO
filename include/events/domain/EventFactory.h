#ifndef EVENT_FACTORY_H
#define EVENT_FACTORY_H

#include <memory>
#include <string>

#include "events/core/CommonTypes.h"
#include "events/domain/Event.h"
#include "events/domain/RecurrentEvent.h"
#include "events/generators/FixedIntervalGenerator.h"
#include "events/providers/NullProvider.h"

namespace events {

class EventFactory {
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

    /**
     * @brief Crea un evento ricorrente settimanale semplice (ogni settimana nello stesso giorno e ora)
     * @param title Il titolo dell'evento ricorrente
     * @param start Il momento di inizio del primo evento (definisce anche il giorno e l'ora della ricorrenza)
     * @param duration La durata di ogni evento
     * @param end Il momento di fine della ricorrenza (non genererà eventi dopo questa data)
     * @return Un puntatore unico all'evento ricorrente creato
     */
    static std::unique_ptr<RecurrentEvent> createSimpleWeekly(const String& title, TimePoint start, Duration duration, TimePoint end);
};

} // namespace events

#endif // EVENT_FACTORY_H
