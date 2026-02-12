#ifndef DATEGENERATOR_H
#define DATEGENERATOR_H

#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"

namespace events {

class DateGenerator {
public:
    /** @brief Distruttore virtuale */
    virtual ~DateGenerator() = default;

    /** @brief Genera le date comprese nell'intervallo [from, to] 
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return Un vettore di TimePoint che rappresentano le date generate
    */
    virtual std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const = 0;
    
    /** @brief Verifica se esistono date comprese nell'intervallo [from, to]
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return true se esistono date comprese nell'intervallo, false altrimenti
    */
    virtual bool occursInRange(TimePoint from, TimePoint to) const = 0;

    /** @brief Restituisce una descrizione del generatore di date
     *  @return Una stringa che descrive il generatore di date
    */
    virtual String describe() const = 0;

    friend std::ostream& operator<<(std::ostream& os, const DateGenerator& generator) {
        os << generator.describe();
        return os;
    }
};

} // namespace events

#endif // DATEGENERATOR_H