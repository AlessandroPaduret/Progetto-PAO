#ifndef SCHEDULABLE_H
#define SCHEDULABLE_H

#include <vector>
#include <chrono>

#include "CommonTypes.h"

class Schedulable {
public:
    /** @return Il punto temporale (data/ora) di inizio */
    virtual TimePoint getStart() const = 0;

    /** @return La durata dell'evento */
    virtual Duration getDuration() const = 0;

    /** @return Il punto temporale (data/ora) di fine */
    virtual TimePoint getEnd() const = 0;

    /** @brief Imposta l'orario di inizio */
    virtual void setStart(const TimePoint start) = 0;

    /** @brief Imposta la durata */
    virtual void setDuration(const Duration duration) = 0;

    /** @brief Imposta l'orario di fine quindi modifica durata */
    virtual void setEnd(const TimePoint end) = 0;

    /** @return Se l'evento è compreso nel range specificato */
    virtual bool isIn(const TimePoint from, const TimePoint to) const = 0;

    /** @return Se l'evento si sovrappone con un altro evento */
    virtual bool overlapsWith(const Schedulable& other) const = 0;

    /** @brief Distruttore virtuale */
    virtual ~Schedulable() = default;
};
#endif  // SCHEDULABLE_H
