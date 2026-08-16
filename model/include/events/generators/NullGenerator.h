#ifndef NULLGENERATOR_H
#define NULLGENERATOR_H

#include <memory>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

class NullGenerator: public DateGenerator {
public:
    /** @brief Distruttore virtuale */
    virtual ~NullGenerator() = default;

    /** @brief Doppio dispatch verso DateGeneratorVisitor::visit(const NullGenerator&) */
    void accept(DateGeneratorVisitor& visitor) const override {
        visitor.visit(*this);
    }

    /** @brief Genera le date comprese nell'intervallo [from, to] 
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return Un vettore di TimePoint che rappresentano le date generate
    */
    std::vector<TimePoint> generateDates(TimePoint, TimePoint) const override {
        return {};
    }

    /** @brief Verifica se esistono date comprese nell'intervallo [from, to]
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return true se esistono date comprese nell'intervallo, false altrimenti
    */
    bool occursInRange(TimePoint, TimePoint) const override {
        return false;
    }
    
    /** @brief Restituisce una descrizione del generatore di date
     *  @return Una stringa che descrive il generatore di date
    */
    String describe() const override {
        return "[NullGenerator]";
    }
};

} // namespace events

#endif  // NULLGENERATOR_H