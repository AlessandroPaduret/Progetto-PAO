#ifndef DATEGENERATOR_H
#define DATEGENERATOR_H

#include <vector>

#include "events/core/CommonTypes.h"

namespace events {

class DateGeneratorVisitor;

/**
 * @class DateGenerator
 * @brief Interfaccia (Strategy) per la generazione delle date di un'attivita'.
 *
 * E' un'interfaccia pura SENZA stato: i generatori concreti sono immutabili,
 * tutta la configurazione (inizio, intervallo, fine, limite di occorrenze)
 * entra dal costruttore e viene esposta solo tramite accessor read-only.
 * Per spostare/troncare un generatore si usa MoveGeneratorVisitor, che ne
 * ricostruisce uno nuovo: non esistono setter pubblici.
 */
class DateGenerator {
public:
    /** @brief Distruttore virtuale */
    virtual ~DateGenerator() = default;

    /** @return L'istante della prima data generata (read-only) */
    virtual TimePoint getStart() const = 0;

    /** @brief Genera le date comprese nell'intervallo [from, to] 
     *  @param from Data di inizio dell'intervallo
     *  @param to Data di fine dell'intervallo
     *  @return Un vettore di TimePoint che rappresentano le date generate
    */
    virtual std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const = 0;

    /** @brief Verifica se `tp` e' una data generabile da questo generatore.
     *         Coerente con generateDates: restituisce true se e solo se
     *         tp compare tra le occorrenze prodotte (vincoli di fine,
     *         limite di occorrenze e allineamento inclusi).
     *  @param tp L'istante da verificare
     *  @return true se `tp` e' una data generata dal generatore
     */
    virtual bool isIn(TimePoint tp) const = 0;
    
    /** @brief Doppio dispatch verso DateGeneratorVisitor::visit(...) del tipo concreto */
    virtual void accept(DateGeneratorVisitor& visitor) const = 0;

    /** @brief Restituisce una descrizione del generatore di date
     *  @return Una stringa che descrive il generatore di date
    */
    virtual String describe() const = 0;

    friend std::ostream& operator<<(std::ostream& os, const DateGenerator& generator) {
        os << generator.describe();
        return os;
    }
};

} // namespace events

#endif // DATEGENERATOR_H