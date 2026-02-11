#ifndef RECURRENCE_STRATEGY_H
#define RECURRENCE_STRATEGY_H

#include <vector>
#include <chrono>
#include <memory>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/generators/ExceptionGenerator.h"
#include "events/core/ItemProvider.h"
#include "events/providers/ModificationProvider.h"

namespace events {

template<typename T>
class RecurrenceStrategy {
private:
    std::shared_ptr<DateGenerator> m_generator;
    std::shared_ptr<ItemProvider<T>> m_provider;
public:

    /** @brief Costruttore che accetta un generatore di ricorrenze e un provider di elementi
     *  @param generator Generatore di ricorrenze da utilizzare per generare le date di ricorrenza
     *  @param provider Provider di elementi da utilizzare per ottenere gli elementi specifici in una data ricorrenza
     */
    RecurrenceStrategy(std::shared_ptr<DateGenerator> generator, std::shared_ptr<ItemProvider<T>> provider);
    
    /** @brief Distruttore virtuale */
    virtual ~RecurrenceStrategy() = default;
    
    /** @brief Genera le date di ricorrenza in un intervallo di tempo
     *  @param from Inizio dell'intervallo
     *  @param to Fine dell'intervallo
     *  @return Vettore di TimePoint rappresentanti le date di ricorrenza generate nell'intervallo specificato
     */
    virtual std::vector<TimePoint> generateDates(const TimePoint from,const TimePoint to) const;

    /** @brief Verifica se esistono ricorrenze nell'intervallo specificato
     *  @param from Inizio dell'intervallo
     *  @param to Fine dell'intervallo
     *  @return true se esistono ricorrenze nell'intervallo specificato, false altrimenti
     */
    virtual bool occursInRange(TimePoint from, TimePoint to) const;

    /** @brief Restituisce un puntatore unico all'elemento specifico in una data ricorrenza 
     *  @param tp TimePoint rappresentante la data di ricorrenza specifica
     *  @return Puntatore unico all'elemento specifico in quella data di ricorrenza, o nullptr se non esiste
     */
    virtual std::unique_ptr<T> getItem(TimePoint tp) const;
    
    /** @brief Aggiunge una modifica a una specifica occorrenza dell'evento ricorrente 
     *  @param tp TimePoint rappresentante la data di ricorrenza specifica a cui applicare la modifica
     *  @param modified Evento modificato da associare a quella data di ricorrenza (es. titolo diverso, orario spostato, ecc.)
    */
    void addModification(TimePoint tp, std::unique_ptr<T> modified);

    /** @brief Elimina tutte le modifiche associate a una specifica occorrenza dell'evento ricorrente
     *  @param tp TimePoint rappresentante la data di ricorrenza specifica a cui rimuovere tutte le modifiche
     */
    void deleteModifications(TimePoint tp);

    /** @brief Aggiunge un'eccezione a una specifica occorrenza dell'evento ricorrente 
     *  @param tp TimePoint rappresentante la data di ricorrenza specifica da escludere
     */
    void addException(TimePoint tp);

    /** @brief Elimina tutte le eccezioni associate a una specifica occorrenza dell'evento ricorrente
     *  @param tp TimePoint rappresentante la data di ricorrenza specifica a cui rimuovere tutte le eccezioni
     */
    void deleteExceptions(TimePoint tp);


};

template<typename T>
RecurrenceStrategy<T>::RecurrenceStrategy(std::shared_ptr<DateGenerator> generator, std::shared_ptr<ItemProvider<T>> provider)
    : m_generator(std::move(generator)), m_provider(std::move(provider)) {
    //assicuro che ci siano almeno i decoratori per gestire modifiche ed eccezioni, anche se non vengono forniti esplicitamente
    if(!dynamic_cast<ModificationProvider<T>*>(m_provider.get())){
        m_provider = std::make_shared<ModificationProvider<T>>(m_provider);
    }
    if(!dynamic_cast<ExceptionGenerator*>(m_generator.get())){
        m_generator = std::make_shared<ExceptionGenerator>(m_generator);
    }

}



template<typename T>
std::vector<TimePoint> RecurrenceStrategy<T>::generateDates(const TimePoint from,const TimePoint to) const {
    return m_generator->generateDates(from, to);
}

template<typename T>
bool RecurrenceStrategy<T>::occursInRange(TimePoint from, TimePoint to) const {
    return m_generator->occursInRange(from, to);
}

template<typename T>
std::unique_ptr<T> RecurrenceStrategy<T>::getItem(TimePoint tp) const {
    return m_provider->getItem(tp);
}

template<typename T>
void RecurrenceStrategy<T>::addModification(TimePoint tp, std::unique_ptr<T> modified) {
    static_cast<ModificationProvider<T>*>(m_provider.get())->addModification(tp, std::move(modified));
}

template<typename T>
void RecurrenceStrategy<T>::deleteModifications(TimePoint tp) {
    static_cast<ModificationProvider<T>*>(m_provider.get())->deleteModification(tp);
}

template<typename T>
void RecurrenceStrategy<T>::addException(TimePoint tp) {
    static_cast<ExceptionGenerator*>(m_generator.get())->addException(tp);
}

template<typename T>
void RecurrenceStrategy<T>::deleteExceptions(TimePoint tp) {
    static_cast<ExceptionGenerator*>(m_generator.get())->deleteException(tp);
}

} // namespace events

#endif  // RECURRENCE_STRATEGY_H