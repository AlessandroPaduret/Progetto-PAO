#ifndef MONTHLY_GENERATOR_H
#define MONTHLY_GENERATOR_H

#include <chrono>
#include <cstddef>
#include <memory>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

/**
 * @class MonthlyGenerator
 * @brief Generatore concreto per serie temporali con ricorrenza mensile di calendario.
 *
 * Genera un'occorrenza ogni N mesi di calendario, mantenendo lo stesso giorno del mese
 * (con gestione del *clamping*: es. il 31 gennaio scivola all'ultimo giorno utile di febbraio,
 * come il 28 febbraio negli anni non bisestili).
 *
 * @details
 * - **Mutabilita'**: I limiti temporali (`m_start`, `m_end`) e il passo in mesi
 *   (`m_months`) sono modificabili tramite @ref setStart/@ref setEnd e
 *   @ref setMonths (spostamento, troncamento e cambio di frequenza della serie).
 * - **Polimorfismo**: Supporta la clonazione profonda tramite @ref clone() e l'ispezione tramite Visitor.
 */
class MonthlyGenerator : public DateGenerator {
private:
    TimePoint m_start;   ///< Istante temporale della prima occorrenza valida.
    int m_months;        ///< Passo in mesi solari tra un'occorrenza e la successiva (deve essere >= 1).
    TimePoint m_end;     ///< Istante temporale limite oltre il quale non vengono prodotte occorrenze.

public:

    //@{
    /** @name Implementazione di DateGenerator - Ciclo di Vita */

    /** 
     * @brief Costruttore principale con iniezione di tutti i parametri di configurazione.
     * 
     * @param start Data e ora della prima occorrenza della serie.
     * @param months Passo in mesi di calendario (default: 1).
     * @param end Limite temporale superiore (default: `TimePoint::max()` per serie illimitate).
     */
    MonthlyGenerator(TimePoint start, int months = 1,
                     TimePoint end = TimePoint::max());

    /// @inheritdoc
    [[nodiscard]] std::unique_ptr<DateGenerator> clone() const override;
    
    /// @inheritdoc
    ~MonthlyGenerator() override = default;
    //@}


    //@{
    /**
     * @name Query dello Stato e Accessor Specifici
     * Metodi di sola lettura per ispezionare il comportamento mensile.
     */

    /// @inheritdoc
    TimePoint getStart() const override;

    /** 
     * @brief Restituisce il passo in mesi solari della ricorrenza.
     * @return int Rappresentante l'intervallo in mesi.
     */
    int getMonths() const;

    /** 
     * @brief Imposta il passo in mesi solari della ricorrenza.
     * @param months Nuovo passo in mesi (deve essere >= 1).
     */
    void setMonths(int months);

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

#endif // MONTHLY_GENERATOR_H