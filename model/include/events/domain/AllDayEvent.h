#ifndef ALLDAYEVENT_H
#define ALLDAYEVENT_H

#include <chrono>
#include <memory>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"
#include "events/core/Occurrence.h"

namespace events {

/**
 * @class AllDayEvent
 * @brief Attivita' "tutto il giorno": occupa UNA O PIU' date intere, senza
 *        orario. Nella GUI viene mostrata nella striscia dedicata in alto
 *        delle viste Giorno/Settimana (stile Google Calendar).
 *
 * L'intervallo e' espresso in giorni: inizio (mezzanotte del primo giorno) e
 * fine (mezzanotte del giorno DOPO l'ultimo), in modo che [start, end) copra
 * esattamente i giorni coinvolti.
 */
class AllDayEvent : public Activity {
private:
    TimePoint m_start;  ///< Mezzanotte del primo giorno
    TimePoint m_end;    ///< Mezzanotte del giorno successivo all'ultimo (esclusa)

protected:
    AllDayEvent* clone_impl() const override;

public:
    /** @brief Costruttore.
     *  @param title Titolo dell'attivita'
     *  @param start Mezzanotte del primo giorno
     *  @param end Mezzanotte del giorno successivo all'ultimo (esclusa)
     *  @throws std::invalid_argument se end non e' successivo a start
     */
    explicit AllDayEvent(const String& title = "",
                         const TimePoint start =
                             std::chrono::time_point_cast<std::chrono::seconds>(
                                 Clock::now()),
                         const TimePoint end = std::chrono::time_point_cast<
                             std::chrono::seconds>(Clock::now()));

    /** @return La mezzanotte del primo giorno */
    TimePoint getStart() const override;

    /** @return La mezzanotte del giorno successivo all'ultimo (esclusa) */
    TimePoint getEnd() const;

    /** @return Il numero di giorni coperti (>= 1) */
    long days() const;

    /** @brief Imposta l'inizio (mezzanotte del primo giorno) */
    void setStart(TimePoint start);

    /** @brief Imposta la fine (mezzanotte del giorno successivo all'ultimo) */
    void setEnd(TimePoint end);

    /// Implementazione dei metodi virtuali di Activity

    /** @return true (occupa date intere, va nella striscia all-day) */
    bool isAllDay() const override;

    /** @return L'occorrenza all'inizio (durata zero) se [start, end) interseca [from, to] */
    std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const override;

    /** @brief Sposta l'attivita' alla nuova mezzanotte di inizio (durata in giorni invariata). */
    void moveTo(TimePoint newStart) override;

    /** @return Descrizione testuale (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso ActivityVisitor::visit(const AllDayEvent&) */
    void accept(ActivityVisitor& visitor) const override;

    /** @brief Crea una copia dell'attivita' */
    std::unique_ptr<AllDayEvent> clone() const;
};

} // namespace events

#endif // ALLDAYEVENT_H