#pragma once

namespace events {

class FixedIntervalGenerator;
class MonthlyGenerator;
class YearlyGenerator;
class SingleGenerator;

/**
 * @brief Visitor per la gerarchia dei generatori (Strategy).
 *
 * Serializza la regola di ricorrenza (es. in JSON) senza metodi "getType".
 */
class DateGeneratorVisitor {
public:
    virtual ~DateGeneratorVisitor() = default;

    virtual void visit(const FixedIntervalGenerator& generator) = 0;
    virtual void visit(const MonthlyGenerator& generator) = 0;
    virtual void visit(const YearlyGenerator& generator) = 0;
    virtual void visit(const SingleGenerator& generator) = 0;
};

} // namespace events
