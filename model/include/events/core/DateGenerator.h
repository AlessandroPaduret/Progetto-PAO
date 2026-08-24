#ifndef DATEGENERATOR_H
#define DATEGENERATOR_H

#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"

namespace events {

class DateGeneratorVisitor;

class DateGenerator {
private:
    TimePoint m_start;
public:
    /** @brief Distruttore virtuale */
    virtual ~DateGenerator() = default;

    /** @brief Imposta la prima data che genera
     *  @return L'istante della prima data generata */
    TimePoint getStart() const;

    /** @brief Imposta la prima data che genera
     *  @return L'istante della prima data generata */
    void setStart(TimePoint newStart);

    /** @brief Genera le date comprese nell'intervallo [from, to] 
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return Un vettore di TimePoint che rappresentano le date generate
    */
    virtual std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const = 0;
    
    /** @brief Doppio dispatch verso DateGeneratorVisitor::visit(...) del tipo concreto */
    virtual void accept(DateGeneratorVisitor& visitor) const = 0;

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