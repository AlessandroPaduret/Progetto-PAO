#ifndef MOVE_GENERATOR_VISITOR_H
#define MOVE_GENERATOR_VISITOR_H

#include <memory>
#include <optional>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

class FixedIntervalGenerator;
class MonthlyGenerator;
class YearlyGenerator;
class NullGenerator;
class SingleGenerator;

/**
 * @class MoveGeneratorVisitor
 * @brief Visitor per spostare/troncare un generatore di date.
 *
 * I generatori sono IMMUTABILI: questo visitor RICOSTRUISCE un nuovo
 * generatore con la configurazione richiesta (nuovo inizio e/o nuova fine),
 * conservando intervallo/mesi e limite di occorrenze. Il risultato e' in
 * `result` (shared_ptr da riassegnare all'attivita').
 *
 *  - spostamento: costruitlo con `newStart` (la fine NON slitta; se pero'
 *    `end < newStart` la fine viene portata a `newStart`);
 *  - troncamento: costruitlo con `newEnd` (usato da Activity::truncateBefore).
 */
class MoveGeneratorVisitor : public DateGeneratorVisitor {
private:
    std::optional<TimePoint> m_newStart;
    std::optional<TimePoint> m_newEnd;

public:
    /** @brief Costruttore.
     *  @param newStart Nuovo inizio della serie (facoltativo)
     *  @param newEnd Nuova fine della serie (facoltativo)
     */
    explicit MoveGeneratorVisitor(std::optional<TimePoint> newStart = std::nullopt,
                                  std::optional<TimePoint> newEnd = std::nullopt);

    /** @brief Il generatore ricostruito (nullptr fino alla visita) */
    std::shared_ptr<DateGenerator> result;

    void visit(const FixedIntervalGenerator& generator) override;
    void visit(const MonthlyGenerator& generator) override;
    void visit(const YearlyGenerator& generator) override;
    void visit(const NullGenerator& generator) override;
    void visit(const SingleGenerator& generator) override;
};

} // namespace events

#endif // MOVE_GENERATOR_VISITOR_H