#ifndef MAX_OCCURRENCES_DECORATOR_H
#define MAX_OCCURRENCES_DECORATOR_H

#include <cstddef>
#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

/**
 * @class MaxOccurrencesDecorator
 * @brief Decoratore per un generatore
 * 
 * Imposta un limite massimo alle occorrenze di un Generatore.
 *
 * @details
 * - **Delega**: Il generatore decorato resta ownership esclusiva (`unique_ptr`);
 *   il limite di occorrenze e' l'unico parametro proprio del decoratore.
 * - **Polimorfismo**: Supporta la clonazione profonda tramite @ref clone() e l'ispezione tramite Visitor.
 */

class MaxOccurrencesDecorator : public DateGenerator {
private:
    const std::unique_ptr<DateGenerator> m_generator;
    const std::size_t m_maxOccurrences;

public:

    //@{
    /** @name Implementazione di DateGenerator - Ciclo di Vita */

    /** 
     * @brief Costruttore principale con iniezione di tutti i parametri di configurazione.
     * 
     * @param generator Generatore da decorare
     * @param maxOccurrences Il massimo numero di date generabili con generateDates()
     */
    MaxOccurrencesDecorator(std::unique_ptr<DateGenerator> generator, std::size_t maxOccurrences);

    /// @inheritdoc
    std::unique_ptr<DateGenerator> clone() const override;
    //@}

    //@{
    /** @name Query dello Stato e Accessor Specifici */

    /** 
     * @brief Restituisce il massimo numero di occorrenze generate a partire dall'inizio
     * @return int Il massimo numero di occorrenze
     */
    std::size_t getMaxOccurrences() const;


    /** 
     * @brief Restituisce un riferimento costante al generatore sottostante
     * @return const DateGenerator& Il generatore sottostante
     */
    const DateGenerator& getWrappedGenerator() const;

    /// @inheritdoc
    TimePoint getStart() const override;
    
    /// @inheritdoc
    TimePoint getEnd() const override;

    /// @inheritdoc
    void setStart(TimePoint start) override;

    /// @inheritdoc
    void setEnd(TimePoint end) override;
    //@}


    //@{
    /** @name Algoritmi di Generazione e Verifica Date */

    /** 
     * @brief Genera le date comprese nell'intervallo [from, to] (inclusivo). 
     *
     * Le date valide sono le prime n date generate dal generatore sottostante dove n = maxOccurrences
     * 
     * @param from Data di inizio della finestra di ricerca (inclusa). 
     * @param to Data di fine della finestra di ricerca (inclusa). 
     * @return std::vector<TimePoint> Vettore di date valide generate nell'intervallo [from, to].
     */
    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;

    /// @inheritdoc
    bool isIn(TimePoint tp) const override;
    //@}


    //@{
    /** @name Ispezione e Serializzazione */

    /// @inheritdoc
    String describe() const override;

    /// @inheritdoc
    void accept(DateGeneratorVisitor& visitor) const override;
    //@}
};

} // namespace events

#endif // MAX_OCCURRENCES_DECORATOR_H