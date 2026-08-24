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
 * @brief Attivita' (evento): regola di ricorrenza (DateGenerator, Strategy)
 *        + istante di riferimento/durata + insieme di eccezioni (EXDATE).
 *
 * Sostituisce le vecchie classi Event (evento singolo), RecurrentEvent (serie
 * ricorrente) e Anniversary (anniversario annuale): un evento singolo e' una
 * Activity il cui generatore e' un SingleGenerator; un anniversario e' una
 * Activity con YearlyGenerator. Meeting e Task sono sottoclassi che aggiungono
 * i propri attributi specifici.
 */
class Activity {
protected:
    String m_title;                                    ///< Titolo dell'attivita'
    Duration m_duration;                               ///< Durata dell'occorrenza
    std::shared_ptr<DateGenerator> m_generator;        ///< Regola di ricorrenza (fallback: SingleGenerator)
    std::unordered_set<TimePoint, TimePointHasher> m_exceptions;  ///< Occorrenze escluse

    /** @brief Date generate in [from, to] meno le eccezioni (helper condiviso) */
    std::vector<TimePoint> occurrenceDates(TimePoint from, TimePoint to) const;

    virtual Activity* clone_impl() const;              ///< Clonazione polimorfa

public:
    /**
     * @brief Costruttore.
     * @param title Titolo dell'attivita'
     * @param duration Durata (default: zero)
     * @param generator Regola di ricorrenza (default: nullptr = SingleGenerator(now))
     * @throws std::invalid_argument se la durata e' negativa
     */
    Activity(String title = "",
             Duration duration = Duration::zero(),
             std::shared_ptr<DateGenerator> generator = nullptr);

    virtual ~Activity() = default;

    /** @return Il titolo dell'attivita' */
    String getTitle() const;

    /** @brief Imposta il titolo dell'attivita' */
    void setTitle(const String& title);

    /** @return L'istante di riferimento (inizio) dell'attivita' = getStart() del generatore */
    TimePoint getStart() const;

    /** @return La durata dell'occorrenza */
    Duration getDuration() const;

    /** @brief Imposta la durata @throws std::invalid_argument se negativa */
    void setDuration(Duration duration);

    /** @return La fine della ricorrenza (= end del generatore; TimePoint::max() = senza fine) */
    TimePoint getEnd() const;

    /** @brief Imposta la fine della ricorrenza, troncando il generatore */
    void setEnd(TimePoint end);

    /** @return La regola di ricorrenza (condivisa) */
    const std::shared_ptr<DateGenerator>& getGenerator() const;

    /** @return L'insieme delle eccezioni (date delle occorrenze escluse) */
    const std::unordered_set<TimePoint, TimePointHasher>& getExceptions() const;

    /** @brief Aggiunge un'eccezione su una specifica occorrenza.
     *         L'eccezione e' accettata SOLO se `tp` e' una data generabile
     *         dal generatore (isIn): niente date arbitrarie fuori dalla serie.
     *  @param tp L'istante dell'occorrenza da escludere
     *  @return true se l'eccezione e' stata aggiunta, false se `tp` non e'
     *          una data generabile
     */
    bool addException(TimePoint tp);

    /** @brief Tronca la ricorrenza: nessuna occorrenza a partire da tp (esclusa) */
    void truncateBefore(TimePoint tp);

    /** @brief Espande le occorrenze in [from, to] (inclusivo) escludendo le eccezioni */
    virtual std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const;

    /** @brief Sposta l'attivita' al nuovo istante (serie traslata, eccezioni svuotate).
     *         Unica operazione di spostamento: ricostruisce il generatore tramite
     *         MoveGeneratorVisitor (nessun setter sui generatori immutabili). */
    virtual void moveTo(TimePoint newStart);

    /** @return Descrizione testuale dell'attivita' (solo visualizzazione) */
    virtual String describe() const;

    /** @brief Doppio dispatch verso ActivityVisitor::visit(const Activity&) */
    virtual void accept(ActivityVisitor& visitor) const;

    /** @brief Crea una copia polimorfa profonda dell'attivita' */
    std::unique_ptr<Activity> clone() const;
};

} // namespace events

#endif // ACTIVITY_H