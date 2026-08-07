#ifndef FIXED_INTERVAL_GENERATOR_H
#define FIXED_INTERVAL_GENERATOR_H

#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"

namespace events {

class FixedIntervalGenerator : public DateGenerator {
private:
    TimePoint m_start;
    Duration m_interval;
    TimePoint m_end;
public:

    /** @brief Costruttore */
    FixedIntervalGenerator(TimePoint start, Duration interval, TimePoint end = TimePoint::max());

    /** @brief Ritorna la data di inizio dell'intervallo
    *  @return La data di inizio dell'intervallo
    */
    TimePoint getStart() const;

    /** @brief Ritorna l'intervallo di tempo tra le date generate
    *  @return L'intervallo di tempo tra le date generate
    */    
   Duration getInterval() const;

    /** @brief Ritorna la data di fine dell'intervallo
    *  @return La data di fine dell'intervallo
    */
    TimePoint getEnd() const;

    /** @brief Imposta la data di inizio dell'intervallo
    *  @param newStart La nuova data di inizio dell'intervallo
    */
    void setStart(TimePoint newStart);

    /** @brief Imposta l'intervallo di tempo tra le date generate
    *  @param newInterval Il nuovo intervallo di tempo tra le date generate
    */        
    void setInterval(Duration newInterval);

    /** @brief Imposta la data di fine dell'intervallo
     * @param newEnd La nuova data di fine dell'intervallo
     */
    void setEnd(TimePoint newEnd);

    /// Implementazione dei metodi virtuali di DateGenerator

    /** @brief Genera le date comprese nell'intervallo [from, to] 
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return Un vettore di TimePoint che rappresentano le date generate
    */
    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;
    
    /** @brief Verifica se esistono date generate comprese nell'intervallo [from, to]
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return true se esistono date generate comprese nell'intervallo, false altrimenti
    */
    bool occursInRange(TimePoint from, TimePoint to) const override;

    /** @brief Restituisce una descrizione del generatore di date
    *  @return Una stringa che descrive il generatore di date
    */    
   String describe() const override;

};

} // namespace events

#endif // FIXED_INTERVAL_GENERATOR_H