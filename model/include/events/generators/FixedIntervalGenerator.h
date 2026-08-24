#ifndef FIXED_INTERVAL_GENERATOR_H
#define FIXED_INTERVAL_GENERATOR_H

#include <cstddef>
#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

class MoveGeneratorVisitor;

/**
 * @class FixedIntervalGenerator
 * @brief Generatore a intervallo fisso in secondi (giorni/settimane).
 *
 * IMMUTABILE: tutta la configurazione entra dal costruttore (incluso
 * `maxOccurrences`); sono esposti solo accessor read-only. Per spostare o
 * troncare la serie usare MoveGeneratorVisitor (che ricostruisce un nuovo
 * generatore).
 */
class FixedIntervalGenerator : public DateGenerator {
private:
    TimePoint m_start;
    Duration m_interval;
    TimePoint m_end;
    std::size_t m_maxOccurrences;  ///< 0 = illimitate

public:
    /** @brief Costruttore (tutta la configurazione e' iniettabile da qui).
     *  @param start Prima occorrenza
     *  @param interval Passo tra le occorrenze (> 0)
     *  @param end Fine della ricorrenza (default: senza fine)
     *  @param maxOccurrences Limite di occorrenze (0 = illimitate)
     */
    FixedIntervalGenerator(TimePoint start, Duration interval,
                           TimePoint end = TimePoint::max(),
                           std::size_t maxOccurrences = 0);

    /** @return La data di inizio dell'intervallo */
    TimePoint getStart() const override;

    /** @return L'intervallo di tempo tra le date generate */
    Duration getInterval() const;

    /** @return La data di fine dell'intervallo */
    TimePoint getEnd() const;

    /** @return Il numero massimo di occorrenze (0 = illimitate) */
    std::size_t getMaxOccurrences() const;

    /// Implementazione dei metodi virtuali di DateGenerator

    /** @brief Genera le date comprese nell'intervallo [from, to] (inclusivo) */
    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;

    /** @return Descrizione testuale del generatore (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso DateGeneratorVisitor::visit(const FixedIntervalGenerator&) */
    void accept(DateGeneratorVisitor& visitor) const override;
};

} // namespace events

#endif // FIXED_INTERVAL_GENERATOR_H