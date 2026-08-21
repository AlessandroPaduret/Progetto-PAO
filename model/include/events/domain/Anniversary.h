#ifndef ANNIVERSARY_H
#define ANNIVERSARY_H

#include <chrono>
#include <memory>
#include <unordered_set>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"
#include "events/core/Occurrence.h"

namespace events {

/**
 * @class Anniversary
 * @brief Anniversario annuale (compleanni, ricorrenze): ogni anno nella data
 *        indicata, gestendo correttamente gli anni bisestili (29/2 cade il
 *        28/2 negli anni non bisestili). Riusa YearlyGenerator per composizione.
 *
 * Ha stato PER-OCCORRENZA (ogni ricorrenza annuale puo' essere evasa).
 */
class Anniversary : public Activity {
private:
    TimePoint m_date;                 ///< Data base (es. la data di nascita)
    TimePoint m_end;                  ///< Fine della ricorrenza (max = senza fine)
    std::unordered_set<TimePoint, TimePointHasher> m_doneOccurrences;  ///< Ricorrenze evase

protected:
    Anniversary* clone_impl() const override;

public:
    /** @brief Costruttore.
     *  @param title Titolo dell'anniversario
     *  @param date Data base (mezzanotte; giorno/mese definiscono la ricorrenza)
     *  @param end Fine della ricorrenza (default: senza fine)
     */
    explicit Anniversary(const String& title = "",
                         const TimePoint date =
                             std::chrono::time_point_cast<std::chrono::seconds>(
                                 Clock::now()),
                         const TimePoint end = TimePoint::max());

    /** @return La data base (mezzanotte del giorno/mese della ricorrenza) */
    TimePoint getStart() const override;

    /** @brief Imposta la data base (il giorno/mese della ricorrenza). */
    void setDate(TimePoint date);

    /** @return La fine della ricorrenza (TimePoint::max() = senza fine) */
    TimePoint getEnd() const;

    /** @brief Imposta la fine della ricorrenza */
    void setEnd(TimePoint end);

    // --- Stato per-occorrenza ----------------------------------------------

    /** @return Le ricorrenze annuali gia' evase */
    const std::unordered_set<TimePoint, TimePointHasher>&
    getDoneOccurrences() const;

    /** @return true se la ricorrenza all'istante indicato e' evasa */
    bool isDoneAt(TimePoint occurrenceStart) const override;

    /** @brief Segna la ricorrenza all'istante indicato come evasa/non evasa */
    void setDoneAt(TimePoint occurrenceStart, bool done) override;

    /// Implementazione dei metodi virtuali di Activity

    /** @return true (occupa l'intero giorno, va nella striscia all-day) */
    bool isAllDay() const override;

    /** @return Le ricorrenze annuali in [from, to] (leap-aware) */
    std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const override;

    /** @brief Sposta l'anniversario alla nuova data base. */
    void moveTo(TimePoint newStart) override;

    /** @return Descrizione testuale (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso ActivityVisitor::visit(const Anniversary&) */
    void accept(ActivityVisitor& visitor) const override;

    /** @brief Crea una copia dell'anniversario */
    std::unique_ptr<Anniversary> clone() const;
};

} // namespace events

#endif // ANNIVERSARY_H