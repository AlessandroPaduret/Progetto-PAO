#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <memory>
#include <vector>

#include "events/core/CommonTypes.h"
#include "events/core/Occurrence.h"

namespace events {

class ActivityVisitor;

/**
 * @class Activity
 * @brief Radice astratta della gerarchia delle attivita' personali
 *        (eventi, eventi ricorrenti, scadenze, promemoria).
 *
 * I metodi polimorfi non banali sono:
 *  - occurrencesIn(from, to): ogni classe concreta espande la propria
 *    nozione di "quando accade" in modo profondamente diverso;
 *  - accept(visitor): doppio dispatch per operazioni esterne (persistenza, GUI);
 *  - describe(): riassunto testuale specifico per tipo (solo visualizzazione);
 *  - clone(): Prototype, copia polimorfa profonda.
 */
class Activity {
private:
    String m_title;  ///< Titolo dell'attivita'
    bool m_done = false;  ///< Stato di completamento (attivita' a occorrenza singola)

protected:
    virtual Activity* clone_impl() const = 0;  ///< Implementazione della clonazione polimorfa

public:
    explicit Activity(String title = "");
    virtual ~Activity() = default;

    /** @return Il titolo dell'attivita' */
    String getTitle() const;

    /** @brief Imposta il titolo dell'attivita' */
    void setTitle(const String& title);

    // --- Stato di completamento (meccanica "agenda con stati") ----------------

    /** @return true se l'attivita' (intera) e' stata evasa */
    bool isDone() const;

    /** @brief Segna l'attivita' (intera) come evasa/non evasa */
    void setDone(bool done = true);

    /** @return true se l'OCCORRENZA all'istante indicato e' evasa.
     *  Default: restituisce lo stato dell'attivita' intera; le attivita'
     *  ricorrenti (Serie, Anniversario) lo sovrascrivono con lo stato
     *  per-occorrenza. */
    virtual bool isDoneAt(TimePoint occurrenceStart) const;

    /** @brief Segna l'OCCORRENZA all'istante indicato come evasa/non evasa.
     *  Default: aggiorna lo stato dell'attivita' intera. */
    virtual void setDoneAt(TimePoint occurrenceStart, bool done);

    /** @return true se l'attivita' occupa date intere SENZA orario e va
     *  mostrata nella striscia "tutto il giorno" (AllDayEvent, Anniversary).
     *  Default: false. */
    virtual bool isAllDay() const;

    /** @return L'istante temporale di riferimento dell'attivita'
     *          (inizio per gli eventi, scadenza per le Deadline, ecc.) */
    virtual TimePoint getStart() const = 0;

    /** @brief Espande le occorrenze dell'attivita' nell'intervallo [from, to] (inclusivo)
     *  @param from Inizio dell'intervallo
     *  @param to Fine dell'intervallo
     *  @return Le occorrenze nell'intervallo; il comportamento dipende dal tipo dinamico
     */
    virtual std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const = 0;

    /** @brief Sposta l'attivita' all'istante indicato (drag&drop nella GUI).
     *  @param newStart Nuovo istante di inizio/riferimento
     *
     *  Ogni tipo si sposta coerentemente (Event: inizio; RecurrentEvent: primo
     *  inizio e regola di ricorrenza, con eccezioni traslate; Deadline: scadenza;
     *  Reminder: attivazione). La durata non cambia.
     */
    virtual void moveTo(TimePoint newStart) = 0;

    /** @return Descrizione testuale dell'attivita' (solo per visualizzazione) */
    virtual String describe() const = 0;

    /** @brief Doppio dispatch: accetta un visitor e gli delega la visita del tipo concreto */
    virtual void accept(ActivityVisitor& visitor) const = 0;

    /** @brief Crea una copia polimorfa profonda dell'attivita' */
    std::unique_ptr<Activity> clone() const;
};

} // namespace events

#endif // ACTIVITY_H
