#ifndef EXCEPTIONGENERATOR_H
#define EXCEPTIONGENERATOR_H

#include <memory>
#include <chrono>
#include <unordered_set>

#include "events/core/CommonTypes.h"
#include "events/generators/DateGeneratorDecorator.h"

namespace events {


class ExceptionGenerator : public DateGeneratorDecorator {
private:
    std::unordered_set<TimePoint, TimePointHasher> m_exceptions; // assumo tante eccezione quindi accesso = O(1)
public:
    /** @brief Costruttore che accetta un generatore di date da decorare con eccezioni
     *  @param generator Il generatore di date da decorare
    */
    ExceptionGenerator(std::shared_ptr<DateGenerator> generator);

    /** @brief Aggiunge un'eccezione 
     *  @param exception Eccezione da aggiungere
    */
    void addException(const TimePoint exception);

    /** @brief Toglie un'eccezione 
     * @param exception Eccezione da togliere
    */
    void deleteException(const TimePoint exception);

    /** @brief Verifica se un determinato istante è un'eccezione 
     * @param time Istante da verificare
    */
    bool isException(const TimePoint time) const;

    /// Implementazione dei metodi virtuali di DateGeneratorDecorator

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

    /** @brief Restituisce una descrizione del generatore di date
     *  @return Una stringa che descrive il generatore di date
    */
    String describe() const override;
};


} // namespace events

#endif // EXCEPTIONGENERATOR_H