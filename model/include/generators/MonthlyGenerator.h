#pragma once

#include <chrono>
#include <cstddef>
#include "core/DateGenerator.h"

namespace events {

/**
 * @class MonthlyGenerator
 * @brief Generatore per serie temporali con ricorrenza mensile di calendario.
 */
class MonthlyGenerator : public DateGenerator {
private:
    int m_months; ///< Passo in mesi solari (>= 1).

public:
    explicit MonthlyGenerator(int months = 1);
    ~MonthlyGenerator() override = default;

    /// @brief Restituisce il passo in mesi.
    [[nodiscard]] int getMonths() const { return m_months; }

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