#pragma once
#include <typeindex>
#include <cstddef>
#include <memory>

namespace utils {

class Cacheable {
public:
    virtual ~Cacheable() = default;

    friend bool operator==(const Cacheable& lhs, const Cacheable& rhs) {
        if (&lhs == &rhs) return true;
        if (typeid(lhs) != typeid(rhs)) return false;
        return lhs.isEqualImpl(rhs);
    }

    [[nodiscard]] virtual std::size_t hash() const = 0;

protected:
    [[nodiscard]] virtual bool isEqualImpl(const Cacheable& other) const = 0;
};

// functor per std::unordered_map/set di shared_ptr<const Cacheable> (dedup futura via pool)
struct CacheableHash {
    std::size_t operator()(const std::shared_ptr<const Cacheable>& ptr) const {
        return ptr ? ptr->hash() : 0;
    }
};

struct CacheableEqual {
    bool operator()(const std::shared_ptr<const Cacheable>& lhs, 
                    const std::shared_ptr<const Cacheable>& rhs) const {
        if (lhs == rhs) return true;
        if (!lhs || !rhs) return false;
        return *lhs == *rhs;
    }
};

} // namespace utils