#include <sstream>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/generators/SingleGenerator.h"

namespace events {

SingleGenerator::SingleGenerator(TimePoint point) : m_point(point) {}

TimePoint SingleGenerator::getPoint() const { return m_point; }

TimePoint SingleGenerator::getStart() const { return m_point; }

TimePoint SingleGenerator::getEnd() const { return m_point; }

std::vector<TimePoint> SingleGenerator::generateDates(TimePoint from,
                                                      TimePoint to) const {
  // Stessa semantica del resto del modello: inizio in [from, to] inclusivo
  if (m_point >= from && m_point <= to) {
    return {m_point};
  }
  return {};
}

bool SingleGenerator::isIn(TimePoint tp) const { return tp == m_point; }

String SingleGenerator::describe() const {
  std::ostringstream oss;
  oss << "[SingleGenerator] at " << formatDateTime(m_point);
  return oss.str();
}

void SingleGenerator::accept(DateGeneratorVisitor& visitor) const {
  visitor.visit(*this);
}

} // namespace events