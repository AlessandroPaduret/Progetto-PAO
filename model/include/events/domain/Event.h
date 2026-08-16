#ifndef EVENT_H
#define EVENT_H

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"

namespace events {

/**
 * @class Event
 * @brief Attivita' con un unico intervallo temporale: titolo, inizio e durata.
 */
class Event : public Activity {
private:
    TimePoint m_start;      ///< Data e ora di inizio
    Duration m_duration;    ///< Durata dell'evento

protected:
    Event* clone_impl() const override;

public:
    /**
     * @brief Costruttore con parametri opzionali.
     * @param title Titolo dell'evento (default: stringa vuota).
     * @param start Orario di inizio (default: ora attuale).
     * @param duration Durata (default: zero).
     * @throws std::invalid_argument se la durata e' negativa.
     */
    Event(const String& title = "",
          const TimePoint start = std::chrono::time_point_cast<std::chrono::seconds>(Clock::now()),
          const Duration duration = Duration::zero());

    /** @brief Operatore di output per stampare i dettagli dell'evento */
    friend std::ostream& operator<<(std::ostream& os, const Event& event);

    /** @return Il punto temporale (data/ora) di inizio */
    TimePoint getStart() const override;

    /** @return La durata dell'evento */
    Duration getDuration() const;

    /** @return Il punto temporale (data/ora) di fine */
    TimePoint getEnd() const;

    /** @brief Imposta l'orario di inizio dell'evento */
    void setStart(TimePoint start);

    /** @brief Imposta la durata @throws std::invalid_argument se negativa */
    void setDuration(Duration duration);

    /** @brief Imposta l'orario di fine, modificando la durata */
    void setEnd(TimePoint end);

    /** @return true se l'evento e' interamente compreso in [from, to] */
    bool isIn(TimePoint from, TimePoint to) const;

    /** @return true se l'evento si sovrappone temporalmente a un altro */
    bool overlapsWith(const Event& other) const;

    /// Implementazione dei metodi virtuali di Activity

    /** @return La singola occorrenza dell'evento se il suo inizio e' in [from, to] */
    std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const override;

    /** @brief Sposta l'evento all'inizio indicato (durata invariata). */
    void moveTo(TimePoint newStart) override;

    /** @return Descrizione testuale dell'evento (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso ActivityVisitor::visit(const Event&) */
    void accept(ActivityVisitor& visitor) const override;

    /** @brief Crea una copia dell'evento */
    std::unique_ptr<Event> clone() const;
};

} // namespace events

#endif // EVENT_H
