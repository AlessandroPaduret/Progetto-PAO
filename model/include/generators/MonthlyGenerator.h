#pragma once

#include <chrono>
#include <cstddef>
#include "core/DateGenerator.h"

namespace events {

/** @brief Ricorrenza mensile di calendario, passi di N mesi (>= 1); clampa a fine mese (31/1 -> 28/2). */
class MonthlyGenerator : public DateGenerator {
private:
    int m_months;

public:
    explicit MonthlyGenerator(int months = 1);
    ~MonthlyGenerator() override = default;

    [[nodiscard]] int getMonths() const { return m_months; }

    [[nodiscard]] TimePoint next(TimePoint point) const override;
    [[nodiscard]] TimePoint align(TimePoint start, TimePoint from) const override;
    void accept(DateGeneratorVisitor& visitor) const override;

protected:
    [[nodiscard]] bool isEqualImpl(const utils::Cacheable& other) const override;
    [[nodiscard]] std::size_t hash() const override;
};

} // namespace events