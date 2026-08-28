#ifndef DATEGENERATOR_H
#define DATEGENERATOR_H

#include <memory>
#include <ostream>
#include <vector>

#include "events/core/CommonTypes.h"

namespace events {

class DateGeneratorVisitor;

/**
 * @class DateGenerator
 * @brief Interfaccia astratta (Strategy Pattern) per la generazione di serie temporali ricorrenti.
 *
 * Rappresenta la regola logica di ricorrenza di un'attivita' (es. giornaliera, mensile, singola).
 *
 * @details
 * - **Mutabilita'**: I parametri strutturali (intervallo, passo, giorno di
 *   ricorrenza) entrano dal costruttore, ma i limiti temporali della serie
 *   (inizio/fine) sono modificabili a posteriori tramite @ref setStart e
 *   @ref setEnd: spostare o troncare una serie non richiede piu' di
 *   ricostruire il generatore.
 * - **Polimorfismo & Ownership**: Gestito tramite ownership esclusiva (`std::unique_ptr<DateGenerator>`).
 *   La duplicazione avviene esplicitamente tramite il metodo polimorfico @ref clone().
 * - **Visitor Pattern**: Supporta l'ispezione tramite @ref accept per operazioni di serializzazione.
 */
class DateGenerator {
protected:

    //@{
    /**
     * @name Gestione del Ciclo di Vita e Semantica di Spostamento
     * Metodi per la costruzione, lo spostamento in memoria e la clonazione polimorfica.
     */

    /** @brief Costruttore di default. */
    DateGenerator() = default;

    /** @brief Costruttore di copia protetto per consentire il clone() nelle classi derivate ed evitare lo slicing all'esterno. */
    DateGenerator(const DateGenerator&) = default;

    /** @brief Operatore di assegnamento per copia protetto per evitare l'assegnamento diretto tra riferimenti polimorfici. */
    DateGenerator& operator=(const DateGenerator&) = default;

    /** @brief Costruttore di spostamento (Move) abilitato per tipi derivati. */
    DateGenerator(DateGenerator&&) noexcept = default;

    /** @brief Operatore di assegnamento per spostamento (Move) abilitato per tipi derivati. */
    DateGenerator& operator=(DateGenerator&&) noexcept = default;

public:

    /** 
     * @brief Distruttore virtuale di default.
     * Garantisce la corretta deallocazione della memoria nelle classi derivate.
     */
    virtual ~DateGenerator() = default;

    /** 
     * @brief Crea una copia profonda (deep copy) polimorfica dell'oggetto.
     * 
     * Implementa il Virtual Constructor / Prototype Pattern per consentire la duplicazione
     * di generatori senza conoscerne il tipo concreto a tempo di compilazione.
     * 
     * @note L'attributo `[[nodiscard]]` genera un warning se il puntatore restituito viene ignorato.
     * @return std::unique_ptr<DateGenerator> Un nuovo smart pointer che possiede la copia.
     */
    [[nodiscard]] virtual std::unique_ptr<DateGenerator> clone() const = 0;
    
    //@}

    //@{
    /**
     * @name Query dello Stato della Ricorrenza
     * Metodi di sola lettura per ispezionare i limiti temporali della serie.
     */

    /** 
     * @brief Restituisce l'istante di inizio della serie ricorrente.
     * @return TimePoint della prima data generata.
     */
    virtual TimePoint getStart() const = 0;

    /** 
     * @brief Restituisce la data di fine della serie.
     * @return TimePoint di fine della serie
     */
    virtual TimePoint getEnd() const = 0;

    /** 
     * @brief Imposta l'istante di inizio della serie (spostamento).
     * 
     * Trasla la prima occorrenza al nuovo istante. Se il nuovo inizio supera
     * la fine corrente della serie, la fine viene portata al nuovo inizio
     * (la serie non resta mai "a rovescio").
     * 
     * @param start Nuovo istante di inizio della serie.
     */
    virtual void setStart(TimePoint start) = 0;

    /** 
     * @brief Imposta la data di fine della serie (troncamento).
     * 
     * Nessuna occorrenza viene generata oltre questo istante.
     * 
     * @param end Nuova fine della serie (`TimePoint::max()` = senza fine).
     */
    virtual void setEnd(TimePoint end) = 0;
    //@}

    //@{
    /**
     * @name Generazione e Controllo delle Date
     * Algoritmi core della Strategy per la produzione e la verifica degli istanti temporali.
     */

    /** 
     * @brief Genera tutte le occorrenze valide nell'escursione temporale specificata.
     * 
     * @param from Data di inizio della finestra di ricerca (inclusa).
     * @param to Data di fine della finestra di ricerca (inclusa).
     * @return std::vector<TimePoint> Vettore ordinato contenente le date generate nell'intervallo [from, to].
     */
    virtual std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const = 0;

    /** 
     * @brief Verifica se uno specifico istante fa parte della serie prodotta.
     * 
     * Garantisce coerenza logica con @ref generateDates (rispetta l'allineamento dell'intervallo,
     * i limiti temporali e i vincoli dei decoratori).
     * 
     * @param tp Istante temporale da verificare.
     * @return true Se l'istante appartiene alla serie, false altrimenti.
     */
    virtual bool isIn(TimePoint tp) const = 0;
    //@}

    //@{
    /**
     * @name Ispezione e Serializzazione
     * Interfacce per l'esportazione dello stato interno e l'integrazione con il Visitor Pattern.
     */

    /** 
     * @brief Accetta un visitor per il doppio dispatch (Visitor Pattern).
     * @param visitor Riferimento al visitor da eseguire.
     */
    virtual void accept(DateGeneratorVisitor& visitor) const = 0;

    /** 
     * @brief Fornisce una descrizione testuale e formattata della regola di ricorrenza.
     * @return Stringa descrittiva (es. "Ogni 2 settimane a partire da 2026-01-05").
     */
    virtual String describe() const = 0;

    /**
     * @brief Overload dell'operatore di inserimento su stream per la stampa diretta.
     * @param os Stream di output.
     * @param generator Generatore da stampare.
     * @return Reference allo stream di output.
     */
    friend std::ostream& operator<<(std::ostream& os, const DateGenerator& generator) {
        os << generator.describe();
        return os;
    }
    //@}
};

} // namespace events

#endif // DATEGENERATOR_H