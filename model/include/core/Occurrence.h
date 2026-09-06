#pragma once

#include "core/CommonTypes.h"

namespace events {

class Activity;

/**
 * @brief Vista leggera (value type) di una singola occorrenza di un'attivita
 *        sulla timeline.
 *
 * Non possiede l'attivita sorgente: e' valida finche' la sorgente resta viva
 * e non viene modificata.
 */
struct Occurrence {
    const Activity* source;  ///< Attivita che ha prodotto l'occorrenza (non owning)
    TimePoint start;         ///< Inizio dell'occorrenza
    Duration duration;       ///< Durata (zero per attivita puntuali)

    /** @return Il punto temporale di fine occorrenza */
    TimePoint end() const { return start + duration; }

    /** @return true se l'occorrenza si sovrappone all'intervallo [from, to] */
    bool overlaps(const TimePoint from, const TimePoint to) const {
        return start <= to && end() >= from;
    }
};

} // namespace events
