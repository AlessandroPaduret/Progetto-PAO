#pragma once

#include "core/CommonTypes.h"

namespace events {

class Activity;

/**
 * @brief Vista leggera (value type) di una singola occorrenza sulla timeline.
 *
 * Non possiede la sorgente: valida solo finche' questa resta viva e non modificata.
 */
struct Occurrence {
    const Activity* source;  ///< non owning
    TimePoint start;
    Duration duration;

    /** @return Il punto temporale di fine occorrenza. */
    TimePoint end() const { return start + duration; }

    /** @return true se l'occorrenza si sovrappone all'intervallo [from, to]. */
    bool overlaps(const TimePoint from, const TimePoint to) const {
        return start <= to && end() >= from;
    }
};

} // namespace events
