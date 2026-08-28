#ifndef CALENDAR_H
#define CALENDAR_H

#include <memory>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"
#include "events/core/Occurrence.h"

namespace events {

/**
 * @class Calendar
 * @brief L'elenco delle attivita' dell'utente: collezione polimorfa eterogenea.
 *
 * Possiede le attivita' (unique_ptr) e offre le operazioni di interrogazione
 * della timeline (occurrencesIn) e di ricerca testuale (search), entrambe
 * polimorfe: funzionano allo stesso modo su ogni tipo di attivita'.
 */
class Calendar {
private:
    std::vector<std::unique_ptr<Activity>> m_activities;  ///< Attivita' possedute dalla collezione

public:
    /** @brief Aggiunge un'attivita' alla collezione (ne acquisisce la proprieta')
     *  @param activity L'attivita' da aggiungere (non nulla)
     *  @return Riferimento all'attivita' appena inserita
     */
    Activity& add(std::unique_ptr<Activity> activity);

    /** @brief Rimuove l'attivita' identificata dal puntatore
     *  @param activity Puntatore a un'attivita' della collezione (confronto per identita')
     *  @return true se l'attivita' era presente ed e' stata rimossa
     */
    bool remove(const Activity* activity);

    /** 
     * @brief Cerca un'attività e la restituisce per puntatore modificabile.
     * @param target Puntatore all'attività da cercare.
     * @return Activity* che punta all'evento 
     */
    Activity* find(const Activity* target);

    /** 
     * @brief Cerca un'attività e la restituisce per puntatore modificabile.
     * @param target Puntatore all'attività da cercare.
     * @return const Activity* che punta all'evento 
     */
    const Activity* find(const Activity* target) const;

    /** @brief Rimuove l'attivita' identificata dal puntatore e la ritorna
     *  @param activity Puntatore a un'attivita' della collezione (confronto per identita')
     * @return std::unique_ptr<Activity> L'attivita' estratta (nullptr se non trovata).
     */
    [[nodiscard]] std::unique_ptr<Activity> pop(const Activity* activity);

    /** @brief Rimuove tutte le attivita' */
    void clear();

    /** @return Il numero di attivita' nella collezione */
    size_t size() const;

    /** @return true se la collezione e' vuota */
    bool empty() const;

    /** @brief Aggrega le occorrenze di tutte le attivita' in [from, to], ordinate per inizio
     *  @param from Inizio dell'intervallo
     *  @param to Fine dell'intervallo
     *  @return Le occorrenze nell'intervallo (viste non owning sulle attivita')
     */
    std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const;

    /** @brief Cerca le attivita' il cui titolo contiene needle (case-insensitive)
     *  @param needle Testo da cercare (vuoto = tutte le attivita')
     *  @return Puntatori non owning alle attivita' corrispondenti
     */
    std::vector<const Activity*> search(const String& needle) const;

    using const_iterator = std::vector<std::unique_ptr<Activity>>::const_iterator;

    /** @brief Iterazione read-only sulle attivita' (persistenza, viste) */
    const_iterator begin() const;
    const_iterator end() const;
};

} // namespace events

#endif // CALENDAR_H
