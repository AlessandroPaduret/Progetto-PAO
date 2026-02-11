#ifndef SCHEDULABLE_H
#define SCHEDULABLE_H

#include <vector>
#include <chrono>
#include <memory>

#include "events/core/CommonTypes.h"

namespace events {

class Schedulable{
protected:
    virtual Schedulable* clone_impl() const = 0; // Metodo per clonare l'oggetto, necessario per gestire le modifiche specifiche in RecurrentEvent
public:
    /** @return Il punto temporale (data/ora) di inizio */
    virtual TimePoint getStart() const = 0;

    /** @return La durata della schedulabile */
    virtual Duration getDuration() const = 0;

    /** @return Il punto temporale (data/ora) di fine */
    virtual TimePoint getEnd() const = 0;

    /** @brief Imposta l'orario di inizio */
    virtual void setStart(const TimePoint start) = 0;

    /** @brief Imposta la durata */
    virtual void setDuration(const Duration duration) = 0;

    /** @brief Imposta l'orario di fine quindi modifica durata */
    virtual void setEnd(const TimePoint end) = 0;

    /** @return Se la schedulabile è compresa nel range specificato */
    virtual bool isIn(const TimePoint from, const TimePoint to) const = 0;

    /** @return Se la schedulabile si sovrappone con un'altra schedulabile */
    virtual bool overlapsWith(const Schedulable& other) const = 0;

    /** @brief Distruttore virtuale */
    virtual ~Schedulable() = default;

    /** @brief Crea una copia della schedulabile */
    std::unique_ptr<Schedulable> clone() const {
        return std::unique_ptr<Schedulable>(clone_impl());
    }
};
} // namespace events

#endif  // SCHEDULABLE_H
