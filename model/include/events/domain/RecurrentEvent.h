#ifndef RECURRENTEVENT_H
#define RECURRENTEVENT_H

#include <chrono>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/domain/Event.h"

namespace events {

/**
 * @class RecurrentEvent
 * @brief Attivita' ricorrente: regola di ricorrenza (DateGenerator, Strategy)
 *        + evento template + insieme di eccezioni (EXDATE).
 *
 * Il titolo dell'attivita' e' quello ereditato da Activity; il titolo del
 * template viene allineato ad esso in costruzione e nelle occorrenze generate.
 */
class RecurrentEvent : public Activity {
private:
    std::shared_ptr<DateGenerator> m_generator;   ///< Generatore delle date di ricorrenza
    Event m_templateEvent;                        ///< Evento template da cui generare le occorrenze
    std::unordered_set<TimePoint, TimePointHasher> m_exceptions;  ///< Occorrenze escluse
    std::unordered_set<TimePoint, TimePointHasher> m_doneOccurrences;  ///< Occorrenze evase
    bool m_allDay = false;  ///< true se ogni occorrenza occupa l'intero giorno

protected:
    RecurrentEvent* clone_impl() const override;

public:
    /** @brief Costruttore.
     *  @param generator Generatore di date di ricorrenza (ownership condivisa)
     *  @param templateEvent Evento template da cui generare le occorrenze
     */
    RecurrentEvent(std::shared_ptr<DateGenerator> generator, Event templateEvent = Event());

    /** @brief Operatore di output per stampare i dettagli dell'evento ricorrente */
    friend std::ostream& operator<<(std::ostream& os, const RecurrentEvent& event);

    /** @return Il generatore di date (condiviso) */
    const std::shared_ptr<DateGenerator>& getGenerator() const;

    /** @return L'evento template */
    const Event& getTemplateEvent() const;

    /** @brief Marca la serie come "tutto il giorno": ogni occorrenza occupa
     *  l'intero giorno (da mostrare nella striscia in alto). */
    void setAllDay(bool allDay);

    /** @return true se la serie e' "tutto il giorno" */
    bool isAllDay() const override;

    /** @return L'insieme delle eccezioni (date delle occorrenze escluse) */
    const std::unordered_set<TimePoint, TimePointHasher>& getExceptions() const;

    /** @brief Aggiunge un'eccezione su una specifica occorrenza
     *  @param tp Data della ricorrenza da escludere
     */
    void addException(TimePoint tp);

    /** @brief Elimina l'eccezione associata a una specifica occorrenza
     *  @param tp Data della ricorrenza da ripristinare
     */
    void deleteExceptions(TimePoint tp);

    /** @brief Tronca la ricorrenza: nessuna occorrenza a partire da tp.
     *  @param tp Inizio della prima occorrenza da sopprimere (esclusa)
     */
    void truncateBefore(TimePoint tp);

    // --- Stato per-occorrenza ----------------------------------------------

    /** @return Le occorrenze gia' evase */
    const std::unordered_set<TimePoint, TimePointHasher>&
    getDoneOccurrences() const;

    /** @return true se l'occorrenza all'istante indicato e' evasa */
    bool isDoneAt(TimePoint occurrenceStart) const override;

    /** @brief Segna l'occorrenza all'istante indicato come evasa/non evasa */
    void setDoneAt(TimePoint occurrenceStart, bool done) override;

    /** @brief Restituisce le occorrenze in [from, to] come cloni indipendenti del template
     *  @param from Inizio dell'intervallo
     *  @param to Fine dell'intervallo
     *  @return Vettore di puntatori unici a Event nell'intervallo specificato
     */
    std::vector<std::unique_ptr<Event>> getSchedulable(TimePoint from, TimePoint to) const;

    /// Implementazione dei metodi virtuali di Activity

    /** @return L'inizio della prima occorrenza (l'inizio del template) */
    TimePoint getStart() const override;

    /** @brief Espande la ricorrenza in [from, to] escludendo le eccezioni */
    std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const override;

    /** @brief Sposta l'intera serie al nuovo primo inizio (eccezioni traslate). */
    void moveTo(TimePoint newStart) override;

    /** @return Descrizione testuale dell'evento ricorrente (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso ActivityVisitor::visit(const RecurrentEvent&) */
    void accept(ActivityVisitor& visitor) const override;

    /** @brief Crea una copia dell'evento ricorrente */
    std::unique_ptr<RecurrentEvent> clone() const;
};

} // namespace events

#endif // RECURRENTEVENT_H
