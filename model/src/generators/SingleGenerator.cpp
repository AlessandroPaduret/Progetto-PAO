#include "generators/SingleGenerator.h"

namespace events {

TimePoint SingleGenerator::next(TimePoint /*current*/) const {
    return TimePoint::max();
}

TimePoint SingleGenerator::align(TimePoint start, TimePoint from) const {
    if (start >= from) {
        return start;
    }
    return TimePoint::max();
}

void SingleGenerator::accept(DateGeneratorVisitor& visitor) const {
    visitor.visit(*this);
}

bool SingleGenerator::isEqualImpl(const utils::Cacheable& /*other*/) const {
    // stateless: tutte le istanze sono equivalenti
    return true;
}

std::size_t SingleGenerator::hash() const {
    return 0x9e3779b9; // costante arbitraria, identifica solo la classe
}

} // namespace events