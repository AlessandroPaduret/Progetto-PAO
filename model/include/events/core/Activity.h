#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <chrono>
#include <memory>
#include <unordered_set>
#include <vector>
#include <cstddef>

#include "events/core/CommonTypes.h"
#include "events/core/Occurrence.h"
#include "events/core/DateGenerator.h"

namespace events {

class ActivityVisitor;

/**
 * @class Activity
 * @brief Entita' principale per la gestione di un'attivita' o evento nel calendario.
 *
 * Mantiene le informazioni specifiche dell'istanza (titolo, inizio, fine, durata, eccezioni)
 * e delega il calcolo della successione temporale a un DateGenerator stateless e condiviso.
 */
class Activity {
protected:
    String m_title;                                              ///< Titolo dell'attivita'.
    TimePoint m_start;                                           ///< Data e ora d'inizio della serie.
    TimePoint m_end;                                             ///< Data e ora di fine della serie (default: max).
    Duration m_duration;                                         ///< Durata di ogni singola occorrenza.
    std::shared_ptr<const DateGenerator> m_generator;            ///< Regola di ricorrenza condivisa/deduplicata.
    std::unordered_set<TimePoint> m_exceptions; ///< Insieme delle date escluse (EXDATE).

public:
    //@{
    /** @name Costruzione e Gestione Copia/Spostamento */

    /**
     * @brief Costruttore principale dell'attivita'.
     */
    explicit Activity(String title,
                      TimePoint start,
                      Duration duration = Duration::zero(),
                      std::shared_ptr<const DateGenerator> generator = nullptr,
                      TimePoint end = TimePoint::max());

    virtual ~Activity() = default;


    /** @brief Crea una copia profonda polimorfica dell'attivita'. */
    [[nodiscard]] virtual std::unique_ptr<Activity> clone() const;
    //@}

    //@{
    /** @name Query dello Stato e Accessor */

    String getTitle() const { return m_title; }
    void setTitle(const String& title) { m_title = title; }

    TimePoint getStart() const { return m_start; }
    void setStart(TimePoint start) { m_start = start; }

    TimePoint getEnd() const { return m_end; }
    void setEnd(TimePoint end) { m_end = end; }

    Duration getDuration() const { return m_duration; }
    void setDuration(Duration duration);


    const DateGenerator& getGenerator() const;
    std::shared_ptr<const DateGenerator> getGeneratorPtr() const { return m_generator; }
    void setGenerator(std::shared_ptr<const DateGenerator> generator);

    const std::unordered_set<TimePoint>& getExceptions() const { return m_exceptions; }
    //@}

    //@{
    /** @name Gestione delle Occorrenze ed Eccezioni */

    bool addException(TimePoint tp);
    void clearExceptions() { m_exceptions.clear(); }

    /** 
     * @brief Espande e restituisce tutte le occorrenze nell'intervallo [from, to],
     * applicando i limiti temporali, maxOccurrences e filtrando le eccezioni.
     */
    virtual std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const;
    //@}

    //@{
    /** @name Ispezione e Visitor Pattern */

    virtual String describe() const;
    virtual void accept(ActivityVisitor& visitor) const;
    //@}
};

} // namespace events

#endif // ACTIVITY_H