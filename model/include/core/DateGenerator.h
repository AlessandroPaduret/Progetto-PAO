#pragma once

#include <chrono>
#include <cstddef>
#include <generator>

#include "utils/Cacheable.h"
#include "core/CommonTypes.h"
#include "core/DateGeneratorVisitor.h"

namespace events {

/**
 * @brief Interfaccia astratta e stateless per il calcolo delle serie temporali.
 *
 * Pura regola matematica/calendariale (es. "ogni 24h", "ogni mese"), senza
 * m_start/m_end propri — cosi' e' condivisibile/immutabile (shared_ptr<const>).
 */
class DateGenerator : public utils::Cacheable {
public:
    ~DateGenerator() override = default;

    /** @brief Occorrenza successiva a 'current', o TimePoint::max() se la serie e' finita. */
    [[nodiscard]] virtual TimePoint next(TimePoint current) const = 0;

    /** @brief Allinea in O(1) la serie (iniziata in 'start') alla prima data >= 'from'. */
    [[nodiscard]] virtual TimePoint align(TimePoint start, TimePoint from) const = 0;

    virtual void accept(DateGeneratorVisitor& visitor) const = 0;

    /** @brief Genera pigramente le date in [from, limit].
     *  @note coroutine C++23 (std::generator): richiede libstdc++ con GCC >= 14.
     */
    [[nodiscard]] std::generator<TimePoint> occurrences(TimePoint start, TimePoint from, TimePoint limit) const;
};

} // namespace events
