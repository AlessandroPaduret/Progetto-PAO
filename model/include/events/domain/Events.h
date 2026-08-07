#ifndef EVENTS_H
#define EVENTS_H

#include <vector>
#include <chrono>
#include <memory>
#include <iostream>

#include "events/core/CommonTypes.h"
#include "events/core/GroupSchedulable.h"
#include "events/domain/Event.h"

namespace events {

/** @brief Raccolta di eventi non ricorrenti su cui interrogare la timeline
 * 
 *  È l'equivalente della lista "Events" del progetto: un insieme di eventi
 *  singoli (senza regolarità) su cui si può chiamare getSchedulable(from, to).
*/
class Events : public GroupSchedulable<Event> {
private:
    std::vector<Event> m_events;
public:

    /** @brief Aggiunge un evento alla raccolta
     *  @param event Evento da aggiungere
    */
    void addEvent(const Event& event);

    /** @brief Restituisce il numero di eventi nella raccolta
     *  @return Il numero di eventi
    */
    size_t size() const;

    /** @brief Operatore di output per stampare gli eventi raccolti */
    friend std::ostream& operator<<(std::ostream& os, const Events& events);

    /// Implementazione dei metodi virtuali di GroupSchedulable

    /** @brief Restituisce gli eventi il cui inizio è compreso in [from, to] (inclusivi), ordinati per inizio
     *  @param from Inizio dell'intervallo
     *  @param to Fine dell'intervallo
     *  @return Vettore di puntatori unici a copie degli eventi nell'intervallo
    */
    std::vector<std::unique_ptr<Event>> getSchedulable(const TimePoint from, const TimePoint to) const override;
};

} // namespace events

#endif // EVENTS_H
