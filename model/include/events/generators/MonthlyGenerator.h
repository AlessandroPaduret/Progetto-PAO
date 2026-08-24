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
    std::size_t m_maxOccurrences;  ///< 0 = illimitate

public:
    /** @brief Costruttore (immutabile: configurazione solo da qui).
     *  @param start Data/ora della prima occorrenza
     *  @param months Passo in mesi (default: 1)
     *  @param end Fine della ricorrenza (default: senza fine)
     *  @param maxOccurrences Limite di occorrenze (0 = illimitate)
     */
    MonthlyGenerator(TimePoint start, int months = 1,
                     TimePoint end = TimePoint::max(),
                     std::size_t maxOccurrences = 0);

    /** @return La data/ora della prima occorrenza */
    TimePoint getStart() const override;

    /** @return Il passo in mesi */
    int getMonths() const;

    /** @return La fine della ricorrenza (TimePoint::max() = senza fine) */
    TimePoint getEnd() const;

    /** @return Il numero massimo di occorrenze (0 = illimitate) */
    std::size_t getMaxOccurrences() const;

    /// Implementazione dei metodi virtuali di DateGenerator

    /** @brief Genera le date mensili in [from, to] (inclusivo) */
    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;

    /** @return true se `tp` e' una data generata dalla serie mensile */
    bool isIn(TimePoint tp) const override;


    /** @return Descrizione testuale (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso DateGeneratorVisitor::visit(const MonthlyGenerator&) */
    void accept(DateGeneratorVisitor& visitor) const override;
};

} // namespace events

#endif // MONTHLY_GENERATOR_H