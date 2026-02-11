#include <memory>
#include <vector>
#include <chrono>

#include "core/CommonTypes.h"
#include "generators/DateGeneratorDecorator.h"

DateGeneratorDecorator::DateGeneratorDecorator(std::shared_ptr<DateGenerator> generator) : m_decoratedGenerator(generator) {}

std::vector<TimePoint> DateGeneratorDecorator::generateDates(TimePoint from, TimePoint to) const {
    return m_decoratedGenerator->generateDates(from, to);
}

bool DateGeneratorDecorator::occursInRange(TimePoint from, TimePoint to) const {
    return m_decoratedGenerator->occursInRange(from, to);
}