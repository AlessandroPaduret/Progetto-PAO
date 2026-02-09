#ifndef BASEITEMPROVIDER_H
#define BASEITEMPROVIDER_H

#include <memory>
#include <chrono>

#include "core/CommonTypes.h"
#include "core/ItemProvider.h"

// NullObject per ItemProvider, restituisce sempre nullptr per ogni data di ricorrenza, utile come provider di base quando non si vogliono fornire modifiche specifiche
template<typename T>
class NullProvider : public ItemProvider<T> {
public:
    /** @brief Distruttore virtuale */
    virtual ~NullProvider() = default;

    /** @brief Restituisce un puntatore unico all'elemento specifico in una data ricorrenza 
     *  @param tp TimePoint rappresentante la data di ricorrenza specifica
     *  @return Puntatore unico all'elemento specifico in quella data di ricorrenza, o nullptr se non esiste
     */
    virtual std::unique_ptr<T> getItem(TimePoint) const override {
        return nullptr;
    }
};

#endif  // NULLPROVIDER_H