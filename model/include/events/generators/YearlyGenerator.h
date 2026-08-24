#ifndef YEARLY_GENERATOR_H
#define YEARLY_GENERATOR_H

#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"

namespace events {

//gestione anni bisestili
class YearlyGenerator : public DateGenerator {
private:
    TimePoint m_start;
    TimePoint m_end;
    std::size_t m_maxOccurrences;  ///< 0 = illimitate
public:
    /** @brief Costruttore (immutabile: configurazione solo da qui).
     *  @param start Prima occorrenza (giorno/mese definiscono la ricorrenza)
     *  @param end Fine della ricorrenza (default: senza fine)
     *  @param maxOccurrences Limite di occorrenze (0 = illimitate)
     */
    YearlyGenerator(TimePoint start, TimePoint end = TimePoint::max(),
                    std::size_t maxOccurrences = 0);

    /** @brief Ritorna la data di inizio dell'intervallo
    *  @return La data di inizio dell'intervallo
    */
    TimePoint getStart() const override;

    /** @brief Ritorna l'intervallo di tempo tra le date generate
    *  @return L'intervallo di tempo tra le date generate
    */    
   Duration getInterval() const;

    /** @brief Ritorna la data di fine dell'intervallo
    *  @return La data di fine dell'intervallo
    */
    TimePoint getEnd() const override;

    /** @return Il numero massimo di occorrenze (0 = illimitate) */
    std::size_t getMaxOccurrences() const;

    /// Implementazione dei metodi virtuali di DateGenerator

    /** @brief Genera le date comprese nell'intervallo [from, to] 
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return Un vettore di TimePoint che rappresentano le date generate
    */
    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;

    /** @return true se `tp` e' una data generata dalla ricorrenza annuale */
    bool isIn(TimePoint tp) const override;
    
    /** @brief Restituisce una descrizione del generatore di date
    *  @return Una stringa che descrive il generatore di date
    */
    String describe() const override;

    /** @brief Doppio dispatch verso DateGeneratorVisitor::visit(const YearlyGenerator&) */
    void accept(DateGeneratorVisitor& visitor) const override;
};

} // namespace events

#endif // YEARLY_GENERATOR_H