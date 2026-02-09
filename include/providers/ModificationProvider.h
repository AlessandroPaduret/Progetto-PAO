#ifndef MODIFICATION_PROVIDER_H
#define MODIFICATION_PROVIDER_H

#include <memory>
#include <chrono>
#include <unordered_map>

#include "core/CommonTypes.h"
#include "providers/ItemProviderDecorator.h"

template<typename T>
class ModificationProvider : public ItemProviderDecorator<T> {
private:
    std::unordered_map<TimePoint, T, TimePointHasher> m_modifications;  // assumo tante modifiche quindi accesso = O(1)
public:
    /** @brief Costruttore che accetta un provider da decorare */
    ModificationProvider(std::shared_ptr<ItemProvider<T>> provider);

    /** @brief Aggiunge una modifica per una data ricorrenza specifica */
    void addModification(TimePoint tp, std::unique_ptr<T> modification);

    /** @brief Verifica se esiste una modifica per una data ricorrenza specifica */
    bool isModified(TimePoint tp) const;

    /** @brief Rimuove una modifica per una data ricorrenza specifica */
    void deleteModification(TimePoint tp);

    /// Implementazione del metodo getItem considerando le modifiche

    /** @brief Restituisce un puntatore unico all'elemento specifico in una data ricorrenza, considerando le modifiche */
    std::unique_ptr<T> getItem(TimePoint tp) const override;

};

template<typename T>
ModificationProvider<T>::ModificationProvider(std::shared_ptr<ItemProvider<T>> provider)
    : ItemProviderDecorator<T>(provider) {}

template<typename T>
void ModificationProvider<T>::addModification(TimePoint tp, std::unique_ptr<T> modification) {
    m_modifications[tp] = *modification;
}

template<typename T>
bool ModificationProvider<T>::isModified(TimePoint tp) const {
    return m_modifications.find(tp) != m_modifications.end();
}

template<typename T>
void ModificationProvider<T>::deleteModification(TimePoint tp) {
    m_modifications.erase(tp);
}

template<typename T>
std::unique_ptr<T> ModificationProvider<T>::getItem(TimePoint tp) const {
    auto it = m_modifications.find(tp);
    if (it != m_modifications.end()) {
        return std::make_unique<T>(it->second);
    }
    return this->m_decoratedProvider->getItem(tp);
}

#endif  // MODIFICATION_PROVIDER_H