#ifndef ITEMPROVIDERDECORATOR_H
#define ITEMPROVIDERDECORATOR_H

#include <memory>
#include <chrono>

#include "events/core/CommonTypes.h"
#include "events/core/ItemProvider.h"

template<typename T>
class ItemProviderDecorator : public ItemProvider<T> {
protected:
    std::shared_ptr<ItemProvider<T>> m_decoratedProvider; // Puntatore condiviso al provider decorato
public:
    /** @brief Costruttore che accetta un provider da decorare */
    ItemProviderDecorator(std::shared_ptr<ItemProvider<T>> provider);

    /** @brief Restituisce un puntatore unico all'elemento specifico in una data ricorrenza 
     *  @param tp TimePoint rappresentante la data di ricorrenza specifica
     *  @return Puntatore unico all'elemento specifico in quella data di ricorrenza, o nullptr se non esiste
     */
    std::unique_ptr<T> getItem(TimePoint tp) const override;
};

template<typename T>
ItemProviderDecorator<T>::ItemProviderDecorator(std::shared_ptr<ItemProvider<T>> provider) : m_decoratedProvider(provider) {}

template<typename T>
std::unique_ptr<T> ItemProviderDecorator<T>::getItem(TimePoint tp) const {
    return m_decoratedProvider->getItem(tp);
}

#endif  // ITEMPROVIDERDECORATOR_H
