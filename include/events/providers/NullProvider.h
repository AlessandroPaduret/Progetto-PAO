#ifndef NULLPROVIDER_H
#define NULLPROVIDER_H

#include <memory>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/ItemProvider.h"

namespace events {

// NullObject per ItemProvider, restituisce sempre nullptr per ogni data di ricorrenza, utile come provider di base quando non si vogliono fornire modifiche specifiche
template<typename T>
class NullProvider : public ItemProvider<T> {
public:
    /** @brief Distruttore virtuale */
    virtual ~NullProvider() = default;

    /** @brief Restituisce un puntatore nullptr
     *  @param tp TimePoint rappresentante la data di ricorrenza specifica
     *  @return nullptr, indipendentemente dalla data di ricorrenza
     */
    virtual std::unique_ptr<T> getItem(TimePoint) const override {
        return nullptr;
    }

    /** @brief Restituisce una descrizione del NullProvider
     *  @return Una stringa che descrive il NullProvider
    */
    virtual String describe() const override {
        return "[NullProvider]";
    }
};

} // namespace events

#endif  // NULLPROVIDER_H