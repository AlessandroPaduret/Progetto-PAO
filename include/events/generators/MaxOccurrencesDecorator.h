#ifndef MAXOCCURRENCESDECORATOR_H
#define MAXOCCURRENCESDECORATOR_H

#include <cstddef>
#include <memory>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/generators/DateGeneratorDecorator.h"

namespace events {

/** @brief Decoratore che limita il numero massimo di occorrenze generate
 * 
 *  Equivalente alla regola COUNT di iCalendar: la ricorrenza si interrompe
 *  dopo un numero predefinito di volte anziché a una data specifica.
*/
class MaxOccurrencesDecorator : public DateGeneratorDecorator {
private:
    size_t m_maxOccurrences;
    mutable size_t m_generatedCount; // contatore cumulativo delle date già generate

public:
    /** @brief Costruttore che accetta un generatore di date da decorare e il numero massimo di occorrenze
     *  @param generator Il generatore di date da decorare
     *  @param maxOccurrences Il numero massimo di occorrenze da generare
    */
    MaxOccurrencesDecorator(std::shared_ptr<DateGenerator> generator, size_t maxOccurrences);

    /** @brief Restituisce il numero massimo di occorrenze configurabile
     *  @return Il numero massimo di occorrenze
    */
    size_t getMaxOccurrences() const;

    /** @brief Restituisce il numero di date già generate
     *  @return Il numero di date generate finora
    */
    size_t getGeneratedCount() const;

    /// Implementazione dei metodi virtuali di DateGeneratorDecorator

    /** @brief Genera le date comprese nell'intervallo [from, to] rispettando il limite massimo di occorrenze
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return Un vettore di TimePoint che rappresentano le date generate
    */
    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;

    /** @brief Verifica se esistono date comprese nell'intervallo [from, to] e non ancora esaurite dal limite
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return true se esistono date comprese nell'intervallo, false altrimenti
    */
    bool occursInRange(TimePoint from, TimePoint to) const override;

    /** @brief Restituisce una descrizione del generatore di date
     *  @return Una stringa che descrive il generatore di date
    */
    String describe() const override;
};

} // namespace events

#endif // MAXOCCURRENCESDECORATOR_H
