#pragma once

#include <chrono>
#include <cstddef>
#include "core/DateGenerator.h"

namespace events {

/** @brief Ricorrenza annuale, passi di N anni preservando mese/giorno/ora; 29 feb -> 28 feb se l'anno non e' bisestile. */
class YearlyGenerator : public DateGenerator {
private:
    int m_years;

public:
    explicit YearlyGenerator(int years = 1);
    ~YearlyGenerator() override = default;

    [[nodiscard]] int getYears() const { return m_years; }

    [[nodiscard]] TimePoint next(TimePoint point) const override;
    [[nodiscard]] TimePoint align(TimePoint start, TimePoint from) const override;
    void accept(DateGeneratorVisitor& visitor) const override;

protected:
    [[nodiscard]] bool isEqualImpl(const utils::Cacheable& other) const override;
    [[nodiscard]] std::size_t hash() const override;
};

} // namespace events