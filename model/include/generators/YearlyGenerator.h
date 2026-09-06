#pragma once

#include <chrono>
#include <cstddef>
#include "core/DateGenerator.h"

namespace events {

/**
 * @class YearlyGenerator
 * @brief Generatore per serie temporali con ricorrenza annuale di calendario.
 * 
 * Gestisce l'avanzamento a passi di N anni preservando mese, giorno e orario
 * (con gestione del clamping per il 29 febbraio negli anni non bisestili).
 */
class YearlyGenerator : public DateGenerator {
private:
    int m_years; ///< Passo in anni (>= 1).

public:
    explicit YearlyGenerator(int years = 1);
    ~YearlyGenerator() override = default;

    /// @brief Restituisce il passo in anni.
    [[nodiscard]] int getYears() const { return m_years; }

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