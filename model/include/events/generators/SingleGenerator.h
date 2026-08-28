#ifndef SINGLEGENERATOR_H
#define SINGLEGENERATOR_H

#include <memory>
#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

/**
 * @class SingleGenerator
 * @brief Generatore che produce una sola data: sostituisce l'evento singolo.
 *
 * Rappresenta la regola logica di un'attivita' che non si ripete
 */
class SingleGenerator : public DateGenerator {
private:
    TimePoint m_point;

public:
    //@{
    /**
     * @name Implementazione di DateGenerator - Ciclo di Vita
     */
    
    /** @brief Costruttore.
     *  @param point unica occorrenza che verrà restituita da questo generatore
     */
    explicit SingleGenerator(TimePoint point);

    /** 
     * @brief Crea una copia profonda polimorfica di questo generatore
     * @return std::unique_ptr<DateGenerator> Nuova istanza clonata di @ref SingleGenerator.
     */
    [[nodiscard]] std::unique_ptr<DateGenerator> clone() const override;
    
    /** @brief Distruttore virtuale di default. */
    ~SingleGenerator() override = default;
    //@}


    //@{
    /** @name Query dello Stato e Accessor Specifici */

    /// @inheritdoc
    TimePoint getStart() const override;

    /** @brief Ritorna l'unica data generata
     * @return l'unica data generata
     */
    TimePoint getPoint() const;

    /// @inheritdoc
    TimePoint getEnd() const override;

    /// @inheritdoc
    void setStart(TimePoint point) override;

    /// @inheritdoc
    void setEnd(TimePoint point) override;
    //@}


    //@{
    /**
     * @name Algoritmi di Generazione e Verifica Date
     */

    /** @brief Genera le date comprese nell'intervallo [from, to] 
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return L'unica data generata se appartiene all'intervallo [from, to]
    */
    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;

    /** @return true se `tp` coincide con l'unica data generata */
    bool isIn(TimePoint tp) const override;
    //@}


    //@{
    /**
     * @name Ispezione e Serializzazione
     */

    /// @inheritdoc
    String describe() const override;

    /// @inheritdoc
    void accept(DateGeneratorVisitor& visitor) const override;
    //@}
};

} // namespace events

#endif  // SINGLEGENERATOR_H