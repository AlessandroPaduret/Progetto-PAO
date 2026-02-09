#ifndef BASEITEMPROVIDER_H
#define BASEITEMPROVIDER_H

#include <memory>
#include <chrono>

#include "core/CommonTypes.h"
#include "core/ItemProvider.h"

template<typename T>
class BaseItemProvider : public ItemProvider<T> {
public:
    /** @brief Distruttore virtuale */
    virtual ~BaseItemProvider() = default;

    /** @brief Restituisce un puntatore unico all'elemento specifico in una data ricorrenza 
     *  @param tp TimePoint rappresentante la data di ricorrenza specifica
     *  @return Puntatore unico all'elemento specifico in quella data di ricorrenza, o nullptr se non esiste
     */
    virtual std::unique_ptr<T> getItem(TimePoint tp) const override;
};

template<typename T>
std::unique_ptr<T> BaseItemProvider<T>::getItem(TimePoint tp) const  {
    return nullptr;
}

#endif  // BASEITEMPROVIDER_H