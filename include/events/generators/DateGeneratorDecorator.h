#ifndef DATEGENERATORDECORATOR_H
#define DATEGENERATORDECORATOR_H

#include <memory>
#include <vector>
#include <chrono>

#include "core/CommonTypes.h"
#include "core/DateGenerator.h"

class DateGeneratorDecorator : public DateGenerator {
protected:
    std::shared_ptr<DateGenerator> m_decoratedGenerator;

public:

    /** @brief Costruttore decoratore base(non fa nulla)
     *  @param generator Il generatore di date da decorare
    */
    DateGeneratorDecorator(std::shared_ptr<DateGenerator> generator);

    /** @brief Genera le date comprese nell'intervallo [from, to] 
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return Un vettore di TimePoint che rappresentano le date generate
    */
    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;

    /** @brief Verifica se esistono date comprese nell'intervallo [from, to]
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return true se esistono date comprese nell'intervallo, false altrimenti
    */
    bool occursInRange(TimePoint from, TimePoint to) const override;
};

#endif  // DATEGENERATORDECORATOR_H