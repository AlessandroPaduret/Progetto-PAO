#pragma once

#include <chrono>
#include <cstddef>
#include "core/DateGenerator.h"

namespace events {

/** @brief Nessuna ricorrenza: l'attivita' si verifica una sola volta. */
class SingleGenerator : public DateGenerator {
public:
    SingleGenerator() = default;
    ~SingleGenerator() override = default;

    /** @return Sempre TimePoint::max(). */
    [[nodiscard]] TimePoint next(TimePoint current) const override;
    /** @return 'start' se >= 'from', altrimenti TimePoint::max(). */
    [[nodiscard]] TimePoint align(TimePoint start, TimePoint from) const override;
    void accept(DateGeneratorVisitor& visitor) const override;

protected:
    [[nodiscard]] bool isEqualImpl(const utils::Cacheable& other) const override;
    [[nodiscard]] std::size_t hash() const override;
};

} // namespace events