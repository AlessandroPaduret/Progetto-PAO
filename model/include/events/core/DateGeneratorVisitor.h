#ifndef DATEGENERATOR_VISITOR_H
#define DATEGENERATOR_VISITOR_H

namespace events {

class FixedIntervalGenerator;
class MonthlyGenerator;
class YearlyGenerator;
class NullGenerator;
class SingleGenerator;

/**
 * @brief Visitor per la gerarchia dei generatori di date (Strategy).
 *
 * Permette di serializzare la regola di ricorrenza (es. in JSON) senza
 * introdurre metodi "getType" nella gerarchia DateGenerator.
 */
class DateGeneratorVisitor {
public:
    virtual ~DateGeneratorVisitor() = default;

    virtual void visit(const FixedIntervalGenerator& generator) = 0;
    virtual void visit(const MonthlyGenerator& generator) = 0;
    virtual void visit(const YearlyGenerator& generator) = 0;
    virtual void visit(const NullGenerator& generator) = 0;
    virtual void visit(const SingleGenerator& generator) = 0;
};

} // namespace events

#endif // DATEGENERATOR_VISITOR_H
