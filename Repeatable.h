#ifndef REPEATABLE_H  // Header Guard
#define REPEATABLE_H

#include <vector>
#include <chrono>
#include <memory>

#include "Schedulable.h"
#include "CommonTypes.h"

template<typename T>
class Repeatable {
    // Assicuriamoci che T sia un sottotipo di Schedulable
    static_assert(std::is_base_of<Schedulable, T>::value, 
                  "Errore: T deve essere un sottotipo di Schedulable!");
public:

    /** @brief Distruttore virtuale */
    virtual ~Repeatable() = default;

    /** @brief Aggiunge un'eccezione alla ricorrenza */
    virtual void addException(const TimePoint exception) = 0;

    /** @brief Toglie un'eccezione alla ricorrenza */
    virtual void deleteException(const TimePoint exception) = 0;

    /** @brief Aggiunge una modifica alla ricorrenza */
    virtual void addModification(const T& modifiedEvent) = 0;

    /** @brief Toglie una modifica alla ricorrenza */
    virtual void deleteModification(const TimePoint modifiedEvent) = 0;

    /** @brief Verifica se un determinato momento è un'eccezione */
    virtual bool isException(const TimePoint time) const = 0;

    /** @brief Verifica se un determinato momento è una modifica */
    virtual bool isModified(const TimePoint time) const = 0;

    /** @brief Restituisce le occorrenze in un intervallo di tempo */
    virtual std::vector<std::unique_ptr<T>> getOccurrences(const TimePoint from, const TimePoint to) const = 0;

};

#endif  // REPEATABLE_H