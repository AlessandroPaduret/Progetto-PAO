#pragma once

#include <memory>
#include <vector>

#include "core/Activity.h"
#include "core/CommonTypes.h"
#include "core/Occurrence.h"

namespace events {

/**
 * @brief L'elenco delle attivita' dell'utente: collezione polimorfa eterogenea.
 *
 * Possiede le attivita' (unique_ptr) e offre le operazioni di interrogazione
 * della timeline (occurrencesIn) e di ricerca testuale (search), entrambe
 * polimorfe: funzionano allo stesso modo su ogni tipo di attivita'.
 */
class Calendar {
private:
    std::vector<std::unique_ptr<Activity>> m_activities;

public:
    /** @brief Aggiunge l'attivita' (ne acquisisce la proprieta'). @return riferimento ad essa. */
    Activity& add(std::unique_ptr<Activity> activity);

    /** @brief Rimuove l'attivita' per identita' del puntatore. */
    bool remove(const Activity* activity);

    Activity* find(const Activity* target);
    const Activity* find(const Activity* target) const;

    /** @brief Rimuove e ritorna l'attivita'. @return nullptr se non trovata. */
    [[nodiscard]] std::unique_ptr<Activity> pop(const Activity* activity);

    void clear();
    size_t size() const;
    bool empty() const;

    /** @brief Aggrega le occorrenze di tutte le attivita' in [from, to], ordinate per inizio. */
    std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const;

    /** @brief Titolo che contiene needle, case-insensitive (vuoto = tutte). */
    std::vector<const Activity*> search(const String& needle) const;

    using const_iterator = std::vector<std::unique_ptr<Activity>>::const_iterator;

    /** @brief Iterazione read-only sulle attivita' (persistenza, viste). */
    const_iterator begin() const;
    const_iterator end() const;
};

} // namespace events
