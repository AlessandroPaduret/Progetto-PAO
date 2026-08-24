#include "events/core/DateGenerator.h"
#include <sstream>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/generators/SingleGenerator.h"

namespace events {

void DateGenerator::setStart(TimePoint newStart) { 
    m_start = newStart;
}

TimePoint DateGenerator::getStart() const { 
    return m_start; 
}

void SingleGenerator::accept(DateGeneratorVisitor& visitor) const {
  visitor.visit(*this);
}

} // namespace events