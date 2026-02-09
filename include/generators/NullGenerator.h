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

    /** @brief Restituisce un puntatore unico all'elemento specifico in una data ricorrenza 
     *  @param tp TimePoint rappresentante la data di ricorrenza specifica
     *  @return Puntatore unico all'elemento specifico in quella data di ricorrenza, o nullptr se non esiste
     */
    virtual std::unique_ptr<T> getItem(TimePoint) const override {
        return nullptr; 
    }
};

#endif  // NULLGENERATOR_H