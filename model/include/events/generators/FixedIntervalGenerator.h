#ifndef FIXED_INTERVAL_GENERATOR_H
#define FIXED_INTERVAL_GENERATOR_H

#include <chrono>
#include <cstddef>
#include <memory>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

/**
 * @class FixedIntervalGenerator
 * @brief Generatore concreto per serie temporali ad intervallo fisso.
 *
 * Rappresenta la regola logica di ricorrenza di un'attivita' che si ripete
 * a cadenza costante (es. ogni 24 ore, ogni 7 giorni, ecc.).
 *
 * @details
 * - **Mutabilita'**: L'intervallo (`m_interval`) resta fisso dopo la costruzione,
 *   ma i limiti temporali (`m_start`, `m_end`) sono modificabili tramite
 *   @ref setStart e @ref setEnd (spostamento e troncamento della serie).
 * - **Polimorfismo**: Implementa l'interfaccia @ref DateGenerator fornendo sia 
 *   l'algoritmo di generazione delle date sia la capacita' di duplicazione profonda tramite @ref clone().
 */
class FixedIntervalGenerator : public DateGenerator {
private:
    TimePoint m_start;        ///< Istante temporale della prima occorrenza valida.
    const Duration m_interval;///< Durata fissa dell'intervallo tra due occorrenze successive.
    TimePoint m_end;          ///< Istante temporale limite oltre il quale non vengono prodotte occorrenze.

public:
    /** 
     * @brief Costruttore principale con iniezione di tutti i parametri di configurazione.
     * 
     * @param start Prima occorrenza della serie.
     * @param interval Passo temporale tra le occorrenze (deve essere > 0).
     * @param end Limite temporale superiore (default: `TimePoint::max()` per serie illimitate).
     */
    FixedIntervalGenerator(TimePoint start, Duration interval,
                           TimePoint end = TimePoint::max());

    /** @brief Distruttore virtuale di default. */
    ~FixedIntervalGenerator() override = default;

    //@{
    /** @name Implementazione di DateGenerator - Ciclo di Vita */

    /// @inheritdoc
    [[nodiscard]] std::unique_ptr<DateGenerator> clone() const override;
    //@}

    
    //@{
    /** @name Query dello Stato e Accessor Specifici */

    /// @inheritdoc
    TimePoint getStart() const override;

    /** 
     * @brief Restituisce l'intervallo di tempo fisso tra due date successive.
     * @return Duration rappresentante la frequenza dell'intervallo.
     */
    Duration getInterval() const;

    /// @inheritdoc
    TimePoint getEnd() const override;

    /** @brief Imposta l'inizio della serie (se supera la fine, la fine si allinea) */
    void setStart(TimePoint start) override;

    /** @brief Imposta la fine della serie (troncamento) */
    void setEnd(TimePoint end) override;
    //@}


    //@{
    /** @name Algoritmi di Generazione e Verifica Date */

    /** 
     * @brief Genera le date comprese nell'intervallo [from, to] (inclusivo).
     * 
     * Calcola matematicamente le date prodotte dalla regola `m_start + k * m_interval`
     * restituendo solo quelle che ricadono all'interno dell'intervallo di ricerca [from, to]
     * e non superano `m_end`.
     * 
     * @param from Data di inizio della finestra di ricerca (inclusa).
     * @param to Data di fine della finestra di ricerca (inclusa).
     * @return std::vector<TimePoint> Vettore ordinato delle date generate.
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

#endif // FIXED_INTERVAL_GENERATOR_H