#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <chrono>
#include <memory>
#include <unordered_set>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/core/Occurrence.h"

namespace events {

class ActivityVisitor;
class DateGenerator;

/**
 * @class Activity
 * @brief Entita' base per la gestione di un'attivita' o evento.
 *
 * Associa le informazioni di un'attivita' (titolo, durata) a una regola 
 * di ricorrenza (@ref DateGenerator) gestita tramite Strategy pattern e a un 
 * insieme di eccezioni (date escluse dalla serie).
 *
 * @details
 * - **Sostituisce**: Le vecchie classi `Event`, `RecurrentEvent` e `Anniversary`.
 * - **Ownership & Polimorfismo**: Gestisce la regola di ricorrenza tramite 
 *   `std::unique_ptr<DateGenerator>` ed e' clonabile tramite @ref clone().
 * - **Semantica**: Disabilita la copia diretta in favore della sola Move Semantics. 
 *   Per la duplicazione profonda occorre usare esplicitamente @ref clone().
 */
class Activity {
protected:
    String m_title;                                              ///< Titolo dell'attivita'.
    Duration m_duration;                                         ///< Durata dell'occorrenza.
    std::unique_ptr<DateGenerator> m_generator;                  ///< Regola di ricorrenza (fallback: SingleGenerator).
    std::unordered_set<TimePoint, TimePointHasher> m_exceptions; ///< Insieme delle date escluse (EXDATE).

    /** 
     * @brief Helper protetto per calcolare le date generate nell'intervallo [from, to],
     *        escludendo le eccezioni presenti in `m_exceptions`.
     */
    std::vector<TimePoint> occurrenceDates(TimePoint from, TimePoint to) const;

public:
    //@{
    /** @name Costruzione, Distruzione e Move Semantics */

    /**
     * @brief Costruttore principale dell'attivita'.
     * 
     * @param title Titolo dell'attivita' (default: stringa vuota).
     * @param duration Durata di ciascuna occorrenza (default: zero).
     * @param generator Regola di ricorrenza (default: nullptr = SingleGenerator a `now`).
     * @throws std::invalid_argument Se la durata e' negativa.
     */
    Activity(String title = "",
             Duration duration = Duration::zero(),
             std::unique_ptr<DateGenerator> generator = nullptr);

    /** @brief Distruttore virtuale di default per distruzione polimorfica. */
    virtual ~Activity() = default;

    /** @brief Costruttore di spostamento (move-only per via della gestione esclusiva del generatore). */
    Activity(Activity&&) noexcept = default;

    /** @brief Operatore di assegnamento per spostamento. */
    Activity& operator=(Activity&&) noexcept = default;

    /**
     * @brief Crea una copia profonda polimorfica dell'attivita'.
     * 
     * @return std::unique_ptr<Activity> Nuova istanza duplicata sullo heap.
     */
    [[nodiscard]] virtual std::unique_ptr<Activity> clone() const;
    //@}

    //@{
    /** @name Query dello Stato e Accessor */

    /** 
     * @brief Restituisce il titolo dell'attivita'.
     * @return Stringa del titolo.
     */
    String getTitle() const;

    /** 
     * @brief Imposta il titolo dell'attivita'.
     * @param title Nuovo titolo.
     */
    void setTitle(const String& title);

    /** 
     * @brief Restituisce l'istante di riferimento (inizio) dell'attivita'.
     * Equivalent alla chiamata `getStart()` sul generatore associato.
     * @return TimePoint della prima occorrenza.
     */
    TimePoint getStart() const;

    /** 
     * @brief Restituisce il limite di fine della serie di occorrenze.
     * Equivalente a `getEnd()` sul generatore (`TimePoint::max()` se illimitata).
     * @return TimePoint di fine della serie.
     */
    TimePoint getEnd() const;

    /** 
     * @brief Restituisce la durata di ciascuna occorrenza.
     * @return Duration rappresentante la durata.
     */
    Duration getDuration() const;

    /** 
     * @brief Imposta la durata dell'occorrenza.
     * @param duration Nuova durata (deve essere >= 0).
     * @throws std::invalid_argument Se la durata e' negativa.
     */
    void setDuration(Duration duration);

    /** 
     * @brief Restituisce la regola di ricorrenza in sola lettura.
     * @return Reference const al DateGenerator interno.
     */
    const DateGenerator& getGenerator() const;

    /** 
     * @brief Restituisce l'insieme delle eccezioni (date escluse).
     * @return Reference const all'insieme delle eccezioni.
     */
    const std::unordered_set<TimePoint, TimePointHasher>& getExceptions() const;
    //@}

    //@{
    /** @name Modifica del Generatore e della Serie */

    /** 
     * @brief Imposta l'inizio della serie ricostruendo/spostando il generatore.
     * Le eccezioni esistenti non vengono traslate.
     * @param start Nuova data di inizio.
     */
    void setStart(TimePoint start);

    /** 
     * @brief Imposta la data di fine della serie, troncando il generatore sottostante.
     * @param end Nuova data di fine.
     */
    void setEnd(TimePoint end);

    /** 
     * @brief Sostituisce la regola di ricorrenza dell'attivita'.
     * @param generator Nuovo generatore (se nullo, viene sostituito da un SingleGenerator a `now`).
     */
    void setGenerator(std::unique_ptr<DateGenerator> generator);
    //@}

    //@{
    /** @name Gestione delle Occorrenze ed Eccezioni */

    /** 
     * @brief Aggiunge un'eccezione su una specifica data della serie.
     * 
     * L'eccezione viene accettata solo se `tp` e' una data generabile dal generatore 
     * (`m_generator->isIn(tp)`).
     * 
     * @param tp Istante dell'occorrenza da escludere.
     * @return true Se l'eccezione e' stata aggiunta, false se `tp` non e' valida per la serie.
     */
    bool addException(TimePoint tp);

    /** 
     * @brief Svuota l'insieme delle eccezioni (serie "intonsa").
     * Usato ad esempio quando l'attivita' viene spostata: le eccezioni sono
     * date assolute che non seguono la nuova posizione della serie.
     */
    void clearExceptions();

    /** 
     * @brief Espande e restituisce tutte le occorrenze nell'intervallo [from, to] (inclusivo),
     * escludendo quelle marcate come eccezioni.
     * 
     * @param from Data iniziale della finestra di ricerca.
     * @param to Data finale della finestra di ricerca.
     * @return std::vector<Occurrence> Vettore delle occorrenze calcolate.
     */
    virtual std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const;
    //@}

    //@{
    /** @name Ispezione e Visitor Pattern */

    /** 
     * @brief Restituisce una descrizione testuale dell'attivita'.
     * @return Stringa descrittiva (es. titolo, durata e dettagli della ricorrenza).
     */
    virtual String describe() const;

    /** 
     * @brief Accetta un visitor per il doppio dispatch (Visitor Pattern).
     * @param visitor Riferimento al visitor da eseguire.
     */
    virtual void accept(ActivityVisitor& visitor) const;
    //@}
};

} // namespace events

#endif // ACTIVITY_H