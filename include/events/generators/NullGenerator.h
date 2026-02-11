#ifndef NULLGENERATOR_H
#define NULLGENERATOR_H

#include <memory>
#include <chrono>

#include "core/CommonTypes.h"
#include "core/ItemProvider.h"

template<typename T>
class NullGenerator: public ItemProvider<T> {
public:
    /** @brief Distruttore virtuale */
    virtual ~NullGenerator() = default;

    /** @brief Restituisce un puntatore nullptr
     *  @return nullptr, indipendentemente dalla data di ricorrenza
     */
    virtual std::unique_ptr<T> getItem(TimePoint) const override {
        return nullptr; 
    }
};

#endif  // NULLGENERATOR_H