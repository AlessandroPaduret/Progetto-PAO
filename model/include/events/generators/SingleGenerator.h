#ifndef SINGLEGENERATOR_H
#define SINGLEGENERATOR_H

#include <memory>
#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

/**
 * @class SingleGenerator
 * @brief Generatore che produce una sola data: sostituisce l'evento singolo.
 *
 * L'attivita' costruita senza un generatore esplicito usa questo generatore
 * come fallback: un "Event" diventa cosi' una Activity con SingleGenerator.
 *
 * IMMUTABILE: l'unico accessor dedicato e' getPoint() (getStart() implementa
 * l'interfaccia); per spostarlo usare MoveGeneratorVisitor.
 */
class SingleGenerator : public DateGenerator {
private:
    TimePoint m_point;

public:
    /** @brief Costruttore */
    explicit SingleGenerator(TimePoint point);

    /** @brief Distruttore virtuale */
    ~SingleGenerator() override = default;

    /** @return L'unica data generata (accessor dedicato) */
    TimePoint getPoint() const;

    /// Implementazione dei metodi virtuali di DateGenerator

    /** @return L'unica data generata */
    TimePoint getStart() const override;

    /** @brief Genera l'unica data se e' compresa in [from, to] (inclusivo) */
    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;

    /** @return Una descrizione testuale del generatore */
    String describe() const override;

    /** @brief Doppio dispatch verso DateGeneratorVisitor::visit(const SingleGenerator&) */
    void accept(DateGeneratorVisitor& visitor) const override;
};

} // namespace events

#endif  // SINGLEGENERATOR_H