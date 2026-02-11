#ifndef ITEMPROVIDER_H
#define ITEMPROVIDER_H

#include <memory>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/Schedulable.h"

namespace events {

template<typename T>
class ItemProvider {
    static_assert(std::is_base_of<Schedulable, T>::value, "Errore: T deve essere un sottotipo di Schedulable!");
public:
    /** @brief Distruttore virtuale */
    virtual ~ItemProvider() = default;

    /** @brief Restituisce un puntatore unico all'elemento specifico in una data ricorrenza 
     *  @param tp TimePoint rappresentante la data di ricorrenza specifica
     *  @return Puntatore unico all'elemento specifico in quella data di ricorrenza, o nullptr se non esiste
     */
    virtual std::unique_ptr<T> getItem(TimePoint tp) const = 0;
};

} // namespace events

#endif  // ITEMPROVIDER_H