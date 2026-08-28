#ifndef YEARLY_GENERATOR_H
#define YEARLY_GENERATOR_H

#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"

namespace events {

/**
 * @class YearlyGenerator
 * @brief Generatore concreto per serie temporali ad intervallo di anni con gestione degli anni bisestili.
 *
 * Rappresenta la regola logica di ricorrenza di un'attivita' che si ripete
 * a cadenza annuale (es. ogni 3 Gennaio, ogni 7 Settembre, ecc.).
 *
 * @details
 * - **Mutabilita'**: Il giorno/mese di ricorrenza (da `m_start`) resta fisso dopo
 *   la costruzione, ma i limiti temporali (`m_start`, `m_end`) sono modificabili
 *   tramite @ref setStart e @ref setEnd (spostamento e troncamento della serie).
 * - **Polimorfismo**: Implementa l'interfaccia @ref DateGenerator fornendo sia 
 *   l'algoritmo di generazione delle date sia la capacita' di duplicazione profonda tramite @ref clone().
 */
class YearlyGenerator : public DateGenerator {

private:
    TimePoint m_start;
    TimePoint m_end;

public:
    //@{
    /**
     * @name Implementazione di DateGenerator - Ciclo di Vita
     */
    
    /** @brief Costruttore (immutabile: configurazione solo da qui).
     * 
     *  @param start Prima occorrenza (giorno/mese definiscono la ricorrenza)
     *  @param end Fine della ricorrenza (default: senza fine)
     */
    YearlyGenerator(TimePoint start, TimePoint end = TimePoint::max());

    /// @inheritdoc
    [[nodiscard]] std::unique_ptr<DateGenerator> clone() const override;
    
    /// @inheritdoc
    ~YearlyGenerator() override = default;
    //@}


    //@{
    /** @name Query dello Stato e Accessor Specifici */

    /// @inheritdoc
    TimePoint getStart() const override;

    /** @brief Ritorna l'intervallo di tempo tra le date generate
    *  @return L'intervallo di tempo tra le date generate
    */    
   Duration getInterval() const;

    /// @inheritdoc
    TimePoint getEnd() const override;

    /// @inheritdoc
    void setStart(TimePoint start) override;

    /// @inheritdoc
    void setEnd(TimePoint end) override;
    //@}
    

    //@{
    /** @name Algoritmi di Generazione e Verifica Date */

    /// @inheritdoc
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

#endif // YEARLY_GENERATOR_H