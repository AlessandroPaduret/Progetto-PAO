#pragma once

#include <chrono>
#include <cstddef>
#include "core/DateGenerator.h"

namespace events {

/** @brief Serie con intervallo di tempo fisso (es. ogni 24h, ogni 30min). */
class FixedIntervalGenerator : public DateGenerator {
private:
    Duration m_interval;

public:
    explicit FixedIntervalGenerator(Duration interval);
    ~FixedIntervalGenerator() override = default;

    [[nodiscard]] Duration getInterval() const { return m_interval; }

    [[nodiscard]] TimePoint next(TimePoint point) const override;
    [[nodiscard]] TimePoint align(TimePoint start, TimePoint from) const override;
    void accept(DateGeneratorVisitor& visitor) const override;

protected:
    [[nodiscard]] bool isEqualImpl(const utils::Cacheable& other) const override;
    [[nodiscard]] std::size_t hash() const override;
};

} // namespace events