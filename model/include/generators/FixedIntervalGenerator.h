#pragma once

#include <chrono>
#include <cstddef>
#include "core/DateGenerator.h"

namespace events {

/**
 * @class FixedIntervalGenerator
 * @brief Generatore per serie temporali con intervallo di tempo fisso (es. ogni 24h, ogni 30min).
 */
class FixedIntervalGenerator : public DateGenerator {
private:
    Duration m_interval; ///< Intervallo di tempo costante tra due occorrenze.

public:
    explicit FixedIntervalGenerator(Duration interval);
    ~FixedIntervalGenerator() override = default;

    /// @brief Restituisce l'intervallo di tempo impostato.
    [[nodiscard]] Duration getInterval() const { return m_interval; }

    /// @inheritdoc
    [[nodiscard]] TimePoint next(TimePoint point) const override;

    /// @inheritdoc
    [[nodiscard]] TimePoint align(TimePoint start, TimePoint from) const override;

    /// @inheritdoc
    void accept(DateGeneratorVisitor& visitor) const override;

protected:
    /// @inheritdoc
    [[nodiscard]] bool isEqualImpl(const utils::Cacheable& other) const override;

    /// @inheritdoc
    [[nodiscard]] std::size_t hash() const override;
};

} // namespace events