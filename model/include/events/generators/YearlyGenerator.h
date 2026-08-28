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

    /** 
     * @brief Crea una copia profonda polimorfica di questo generatore annuale.
     * 
     * @return std::unique_ptr<DateGenerator> Nuova istanza clonata di @ref YearlyGenerator.
     */
    [[nodiscard]] std::unique_ptr<DateGenerator> clone() const override;
    
    /** @brief Distruttore virtuale di default. */
    ~YearlyGenerator() override = default;
    //@}


    //@{
    /**
     * @name Query dello Stato e Accessor Specifici
     * Metodi di sola lettura per ispezionare il comportamento mensile.
     */

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

    /** @brief Imposta l'inizio della serie (se supera la fine, la fine si allinea) */
    void setStart(TimePoint start) override;

    /** @brief Imposta la fine della serie (troncamento) */
    void setEnd(TimePoint end) override;
    //@}
    

    //@{
    /**
     * @name Algoritmi di Generazione e Verifica Date
     */

    /** @brief Genera le date comprese nell'intervallo [from, to] 
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return Un vettore di TimePoint che rappresentano le date generate
    */
    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;

    /** @return true se `tp` e' una data generata dalla ricorrenza annuale */
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