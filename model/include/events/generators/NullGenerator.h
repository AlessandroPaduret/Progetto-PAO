#ifndef NULLGENERATOR_H
#define NULLGENERATOR_H

#include <vector>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

/**
 * @class NullGenerator
 * @brief Generatore "null object": non produce mai alcuna data.
 *
 * Header-only, mai istanziato dall'applicazione. Serve come oggetto nullo
 * (es. per rappresentare una serie disattivata) e per la persistenza.
 */
class NullGenerator : public DateGenerator {
public:
    /** @return Un istante indefinito (nessuna occorrenza prodotta) */
    TimePoint getStart() const override { return TimePoint{}; }

    /** @brief Non genera mai alcuna data (null object) */
    std::vector<TimePoint> generateDates(TimePoint, TimePoint) const override {
        return {};
    }

    /** @brief Non contiene mai alcuna data (null object) */
    bool isIn(TimePoint) const override { return false; }

    /** @return Una descrizione testuale del generatore */
    String describe() const override { return "[NullGenerator]"; }

    /** @brief Doppio dispatch verso DateGeneratorVisitor::visit(const NullGenerator&) */
    void accept(DateGeneratorVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

} // namespace events

#endif // NULLGENERATOR_H
