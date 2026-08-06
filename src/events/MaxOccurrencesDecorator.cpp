#include <algorithm>
#include <cstddef>
#include <chrono>
#include <memory>

#include "events/core/CommonTypes.h"
#include "events/generators/MaxOccurrencesDecorator.h"

namespace events {

MaxOccurrencesDecorator::MaxOccurrencesDecorator(
    std::shared_ptr<DateGenerator> generator, size_t maxOccurrences)
    : DateGeneratorDecorator(generator),
      m_maxOccurrences(maxOccurrences),
      m_generatedCount(0) {}

size_t MaxOccurrencesDecorator::getMaxOccurrences() const {
  return m_maxOccurrences;
}

size_t MaxOccurrencesDecorator::getGeneratedCount() const {
  return m_generatedCount;
}

/// Implementazione dei metodi virtuali di DateGeneratorDecorator

std::vector<TimePoint>
MaxOccurrencesDecorator::generateDates(TimePoint from, TimePoint to) const {
  // Se il numero massimo di occorrenze è già stato raggiunto, la ricorrenza
  // è terminata e non genera più nulla
  if (m_generatedCount >= m_maxOccurrences) {
    return {};
  }

  // Genera le date dal generatore decorato e limita al numero massimo residuo
  std::vector<TimePoint> dates = m_decoratedGenerator->generateDates(from, to);
  size_t remaining = m_maxOccurrences - m_generatedCount;

  if (dates.size() > remaining) {
    dates.resize(remaining);
  }

  m_generatedCount += dates.size();
  return dates;
}

bool MaxOccurrencesDecorator::occursInRange(TimePoint from,
                                            TimePoint to) const {
  // Se il generatore decorato ha date nell'intervallo e il limite non è esaurito,
  // generateDates restituirà un vettore non vuoto
  return !generateDates(from, to).empty();
}

String MaxOccurrencesDecorator::describe() const {
  return "[MaxOccurrencesDecorator] wrapping: {" +
         m_decoratedGenerator->describe() + "} with max " +
         std::to_string(m_maxOccurrences) + " occurrences";
}

} // namespace events
