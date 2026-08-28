#include <memory>
#include <sstream>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/generators/SingleGenerator.h"

namespace events {


/// Implementazione di DateGenerator - Ciclo di Vita

SingleGenerator::SingleGenerator(TimePoint point) : m_point(point) {}

std::unique_ptr<DateGenerator> SingleGenerator::clone() const {
  return std::make_unique<SingleGenerator>(m_point);
}


/// Query dello Stato e Accessor Specifici

TimePoint SingleGenerator::getPoint() const { return m_point; }

TimePoint SingleGenerator::getStart() const { return m_point; }

TimePoint SingleGenerator::getEnd() const { return m_point; }

void SingleGenerator::setStart(TimePoint point) { m_point = point; }

void SingleGenerator::setEnd(TimePoint point) { m_point = point; }


/// Algoritmi di Generazione e Verifica Date

std::vector<TimePoint> SingleGenerator::generateDates(TimePoint from,
                                                      TimePoint to) const {
  // Stessa semantica del resto del modello: inizio in [from, to] inclusivo
  if (m_point >= from && m_point <= to) {
    return {m_point};
  }
  return {};
}

bool SingleGenerator::isIn(TimePoint tp) const { return tp == m_point; }


/// Ispezione e Serializzazione

String SingleGenerator::describe() const {
  std::ostringstream oss;
  oss << "[SingleGenerator] at " << formatDateTime(m_point);
  return oss.str();
}

void SingleGenerator::accept(DateGeneratorVisitor& visitor) const {
  visitor.visit(*this);
}

} // namespace events