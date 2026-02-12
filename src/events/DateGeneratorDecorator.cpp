#include <chrono>
#include <memory>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/generators/DateGeneratorDecorator.h"

namespace events {

DateGeneratorDecorator::DateGeneratorDecorator(
    std::shared_ptr<DateGenerator> generator)
    : m_decoratedGenerator(generator) {}

std::vector<TimePoint>
DateGeneratorDecorator::generateDates(TimePoint from, TimePoint to) const {
  return m_decoratedGenerator->generateDates(from, to);
}

bool DateGeneratorDecorator::occursInRange(TimePoint from, TimePoint to) const {
  return m_decoratedGenerator->occursInRange(from, to);
}

String DateGeneratorDecorator::describe() const {
    return "[DateGeneratorDecorator] wrapping: [" + m_decoratedGenerator->describe() + "]" ;
} 

} // namespace events
