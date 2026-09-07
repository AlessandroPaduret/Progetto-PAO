#pragma once

#include <chrono>
#include <memory>
#include <unordered_set>
#include <vector>
#include <cstddef>

#include "core/CommonTypes.h"
#include "core/Occurrence.h"
#include "core/DateGenerator.h"

namespace events {

class ActivityVisitor;

/**
 * @brief Entita' concreta "evento/attivita'" del calendario.
 *
 * Tiene i dati dell'istanza (titolo, inizio, fine, durata, eccezioni) e delega
 * la ricorrenza a un DateGenerator stateless condiviso via shared_ptr<const>
 * (immutabile, quindi safe da condividere tra piu' attivita').
 */
class Activity {
protected:
    String m_title;
    TimePoint m_start;
    TimePoint m_end;                                             ///< default: TimePoint::max() (nessun limite)
    Duration m_duration;
    std::shared_ptr<const DateGenerator> m_generator;
    std::unordered_set<TimePoint> m_exceptions;                  ///< date escluse (EXDATE)

public:
    explicit Activity(String title,
                      TimePoint start,
                      Duration duration = Duration::zero(),
                      std::shared_ptr<const DateGenerator> generator = nullptr,
                      TimePoint end = TimePoint::max());

    virtual ~Activity() = default;

    [[nodiscard]] virtual std::unique_ptr<Activity> clone() const;

    String getTitle() const { return m_title; }
    void setTitle(const String& title) { m_title = title; }

    TimePoint getStart() const { return m_start; }
    void setStart(TimePoint start) { m_start = start; }

    TimePoint getEnd() const { return m_end; }
    void setEnd(TimePoint end) { m_end = end; }

    Duration getDuration() const { return m_duration; }
    /** @brief Imposta la durata. @throws std::invalid_argument se negativa. */
    void setDuration(Duration duration);

    const DateGenerator& getGenerator() const;
    std::shared_ptr<const DateGenerator> getGeneratorPtr() const { return m_generator; }
    void setGenerator(std::shared_ptr<const DateGenerator> generator);

    const std::unordered_set<TimePoint>& getExceptions() const { return m_exceptions; }

    bool addException(TimePoint tp);
    void clearExceptions() { m_exceptions.clear(); }

    /** @brief Occorrenze in [from, to] (estremi inclusi), eccezioni filtrate. */
    virtual std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const;

    virtual String describe() const;
    virtual void accept(ActivityVisitor& visitor) const;
};

} // namespace events