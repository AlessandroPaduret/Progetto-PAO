#ifndef GROUPSCHEDULABLE_H
#define GROUPSCHEDULABLE_H

#include <vector>
#include <chrono>
#include <memory>

#include "core/CommonTypes.h"
#include "core/Schedulable.h"

template<typename T>
class GroupSchedulable {
    static_assert(std::is_base_of<Schedulable, T>::value, "T deve derivare da Schedulable");
public:
    virtual std::vector<std::unique_ptr<T>> getSchedulable(TimePoint from, TimePoint to) const = 0;
};

#endif  // GROUPSCHEDULABLE_H