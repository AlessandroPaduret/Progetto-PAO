#ifndef MONTHLY_GENERATOR_H
#define MONTHLY_GENERATOR_H

#include <cstddef>
#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

/**
 * @class MonthlyGenerator
 * @brief Generatore mensile di date: un'occorrenza ogni N mesi CALENDARIALI,
 *        mantenendo lo stesso giorno del mese (con clampping: il 31 cade
 *        l'ultimo giorno del mese, es. 31/1 -> 28/2 negli anni non bisestili).
 *
 * A differenza di FixedIntervalGenerator (che usa una durata fissa in secondi),
 * qui il passo avanza per mesi di calendario, quindi e' esatto anche per
 * "ogni 2 mesi". Supporta il limite di occorrenze (0 = illimitate).
 */
class MonthlyGenerator : public DateGenerator {
private:
    TimePoint m_start;
    int m_months;                 ///< Passo in mesi (>= 1)
    TimePoint m_end;
    std::size_t m_maxOccurrences = 0;  ///< 0 = illimitate

public:
    /** @brief Costruttore.
     *  @param start Data/ora della prima occorrenza
     *  @param months Passo in mesi (default: 1)
     *  @param end Fine della ricorrenza (default: senza fine)
     */
    MonthlyGenerator(TimePoint start, int months = 1,
                     TimePoint end = TimePoint::max());

    /** @return La data/ora della prima occorrenza */
    TimePoint getStart() const;

    /** @return Il passo in mesi */
    int getMonths() const;

    /** @brief Imposta il passo in mesi (>= 1) */
    void setMonths(int months);

    /** @return La fine della ricorrenza (TimePoint::max() = senza fine) */
    TimePoint getEnd() const;

    /** @brief Imposta la data/ora della prima occorrenza */
    void setStart(TimePoint newStart);

    /** @brief Imposta la fine della ricorrenza */
    void setEnd(TimePoint newEnd);

    /** @brief Imposta il numero massimo di occorrenze (0 = illimitate) */
    void setMaxOccurrences(std::size_t n);

    /** @return Il numero massimo di occorrenze (0 = illimitate) */
    std::size_t getMaxOccurrences() const;

    /// Implementazione dei metodi virtuali di DateGenerator

    /** @brief Genera le date mensili in [from, to] (inclusivo) */
    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;


    /** @return Descrizione testuale (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso DateGeneratorVisitor::visit(const MonthlyGenerator&) */
    void accept(DateGeneratorVisitor& visitor) const override;
};

} // namespace events

#endif // MONTHLY_GENERATOR_H