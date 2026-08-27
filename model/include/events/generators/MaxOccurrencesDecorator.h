#ifndef MAX_OCCURRENCES_DECORATOR_H
#define MAX_OCCURRENCES_DECORATOR_H

#include <cstddef>
#include <vector>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/DateGeneratorVisitor.h"

namespace events {

class MaxOccurrencesDecorator : public DateGenerator {
private:
    std::unique_ptr<DateGenerator> m_generator;
    std::size_t m_maxOccurrences;

public:

    /// Metodi propri di MaxOccurrencesDecorator

    MaxOccurrencesDecorator(std::unique_ptr<DateGenerator> generator, std::size_t maxOccurrences);

    std::size_t getMaxOccurrences() const;

    /// Ridefinizione metodi ereditati

    TimePoint getStart() const override;
    
    TimePoint getEnd() const override;

    std::vector<TimePoint> generateDates(TimePoint from, TimePoint to) const override;

    bool isIn(TimePoint tp) const override;

    String describe() const override;

    void accept(DateGeneratorVisitor& visitor) const override;
};

} // namespace events

#endif // MAX_OCCURRENCES_DECORATOR_H