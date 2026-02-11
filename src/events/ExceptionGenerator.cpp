#include <algorithm>
#include <chrono>
#include <memory>
#include <unordered_set>

#include "events/core/CommonTypes.h"
#include "events/generators/ExceptionGenerator.h"

namespace events {

ExceptionGenerator::ExceptionGenerator(std::shared_ptr<DateGenerator> generator)
    : DateGeneratorDecorator(generator) {}

void ExceptionGenerator::addException(const TimePoint exception) {
  m_exceptions.insert(exception);
}

void ExceptionGenerator::deleteException(const TimePoint exception) {
  m_exceptions.erase(exception);
}

bool ExceptionGenerator::isException(const TimePoint time) const {
  return m_exceptions.find(time) != m_exceptions.end();
}

/// Implementazione dei metodi virtuali di DateGeneratorDecorator

std::vector<TimePoint> ExceptionGenerator::generateDates(TimePoint from,
                                                         TimePoint to) const {
  std::vector<TimePoint> dates = m_decoratedGenerator->generateDates(from, to);

  // 1. Sposta tutti i validi all'inizio e ottieni l'inizio della "zona da
  // eliminare"
  auto it_garbage =
      std::remove_if(dates.begin(), dates.end(), [this](const TimePoint &d) {
        return this->isException(d);
      });

  // 2. Cancella TUTTO dal primo elemento non valido fino alla fine del vettore
  dates.erase(it_garbage, dates.end());

  return dates;
}

bool ExceptionGenerator::occursInRange(TimePoint from, TimePoint to) const {
  // Se il generatore decorato non ha date in questo intervallo, restituisci
  // false
  if (!m_decoratedGenerator->occursInRange(from, to)) {
    return false;
  }

  // Se il generatore decorato ha date in questo intervallo, dobbiamo verificare
  // se tutte sono eccezioni
  std::vector<TimePoint> dates = m_decoratedGenerator->generateDates(from, to);
  return !dates.empty();
}

} // namespace events
