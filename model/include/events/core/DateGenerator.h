#pragma once

#include <chrono>
#include <cstddef>
#include <generator>

#include "events/utils/Cacheable.h"
#include "events/core/CommonTypes.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

/**
 * @class DateGenerator
 * @brief Interfaccia astratta e stateless per il calcolo delle serie temporali.
 *
 * Rappresenta la pura regola di calcolo matematico/calendariale (es. "ogni 24h", "ogni mese").
 * Non possiede uno stato interno (nessun m_start o m_end), rendendola idonea per la
 * condivisione e deduplicazione tramite Flyweight/GeneratorPool.
 */
class DateGenerator : public utils::Cacheable {
public:
    ~DateGenerator() override = default;

    /**
     * @brief Calcola l'occorrenza immediatamente successiva a 'current'.
     * @param current Punto di partenza corrente della serie.
     * @return TimePoint L'occorrenza successiva, oppure TimePoint::max() se finita.
     */
    [[nodiscard]] virtual TimePoint next(TimePoint current) const = 0;

    /**
     * @brief Allinea in O(1) la serie (iniziata in 'start') alla prima data utile >= 'from'.
     * @param start La data d'inizio della serie (posseduta dall'Activity).
     * @param from La data di inizio della finestra di ricerca.
     * @return TimePoint La prima occorrenza valida >= 'from'.
     */
    [[nodiscard]] virtual TimePoint align(TimePoint start, TimePoint from) const = 0;

    /**
     * @brief Accetta un visitor per la serializzazione/ispezione polimorfica.
     * @param visitor Riferimento al DateGeneratorVisitor.
     */
    virtual void accept(DateGeneratorVisitor& visitor) const = 0;

    /**
     * @brief Genera pigramente la sequenza di date nell'intervallo [from, limit]
     * @param start l'inizio della ricorrenza di date a cui le date generate si allineeranno
     * @param from l'inizio da cui partire per generare date
     * @param limit il limite oltre al quale non si possono più generare date
     */
    [[nodiscard]] std::generator<TimePoint> occurrences(TimePoint start, TimePoint from, TimePoint limit) const;
};

} // namespace events
