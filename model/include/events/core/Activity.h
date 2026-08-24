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
     * @param start Istante di riferimento (default: ora attuale)
     * @param duration Durata (default: zero)
     * @param generator Regola di ricorrenza (default: nullptr = SingleGenerator(start))
     * @throws std::invalid_argument se la durata e' negativa
     */
    Activity(String title = "",
             TimePoint start = std::chrono::time_point_cast<std::chrono::seconds>(Clock::now()),
             Duration duration = Duration::zero(),
             std::shared_ptr<DateGenerator> generator = nullptr);

    virtual ~Activity() = default;

    /** @return Il titolo dell'attivita' */
    String getTitle() const;

    /** @brief Imposta il titolo dell'attivita' */
    void setTitle(const String& title);

    /** @return L'istante di riferimento (inizio) dell'attivita' */
    TimePoint getStart() const;

    /** @brief Imposta l'istante di riferimento (inizio) */
    void setStart(TimePoint start);

    /** @return La durata dell'occorrenza */
    Duration getDuration() const;

    /** @brief Imposta la durata @throws std::invalid_argument se negativa */
    void setDuration(Duration duration);

    /** @return Il punto temporale di fine (inizio + durata) */
    TimePoint getEnd() const;

    /** @brief Imposta la fine, modificando la durata */
    void setEnd(TimePoint end);

    /** @return La regola di ricorrenza (condivisa) */
    const std::shared_ptr<DateGenerator>& getGenerator() const;

    /** @return L'insieme delle eccezioni (date delle occorrenze escluse) */
    const std::unordered_set<TimePoint, TimePointHasher>& getExceptions() const;

    /** @brief Aggiunge un'eccezione su una specifica occorrenza */
    void addException(TimePoint tp);

    /** @brief Elimina l'eccezione associata a una specifica occorrenza */
    void deleteExceptions(TimePoint tp);

    /** @brief Tronca la ricorrenza: nessuna occorrenza a partire da tp (esclusa) */
    void truncateBefore(TimePoint tp);

    /** @return true se l'attivita' usa un generatore non singolo/nullo */
    bool isRecurrent() const;

    /** @brief Restituisce le occorrenze in [from, to] come cloni indipendenti */
    std::vector<std::unique_ptr<Activity>> getSchedulable(TimePoint from, TimePoint to) const;

    /** @brief Espande le occorrenze in [from, to] (inclusivo) escludendo le eccezioni */
    virtual std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const;

    /** @brief Sposta l'attivita' al nuovo istante (serie traslata, eccezioni svuotate). */
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