#include "generators/SingleGenerator.h"

namespace events {

TimePoint SingleGenerator::next(TimePoint /*current*/) const {
    // Non ci sono occorrenze successive per un evento singolo.
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
    // Tutti i SingleGenerator sono stateless e concettualmente identici.
    return true;
}

std::size_t SingleGenerator::hash() const {
    // Valore costante per identificare la classe SingleGenerator nella pool.
    return 0x9e3779b9; // Costante hash arbitraria
}

} // namespace events