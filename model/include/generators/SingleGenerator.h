#pragma once

#include <chrono>
#include <cstddef>
#include "core/DateGenerator.h"

namespace events {

/**
 * @class SingleGenerator
 * @brief Generatore per eventi singoli senza ricorrenza.
 * 
 * Rappresenta la regola per un'attivita' che si verifica una sola volta.
 */
class SingleGenerator : public DateGenerator {
public:
    SingleGenerator() = default;
    ~SingleGenerator() override = default;

    /// @brief Restituisce TimePoint::max() ad indicare l'assenza di date successive.
    [[nodiscard]] TimePoint next(TimePoint current) const override;

    /// @brief Restituisce 'start' se >= 'from', altrimenti TimePoint::max().
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