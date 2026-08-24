#ifndef TASK_H
#define TASK_H

#include <chrono>
#include <memory>
#include <unordered_set>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/Occurrence.h"

namespace events {

/** @brief Livelli di priorita' di un compito */
enum class Priority { Low, Medium, High };

/**
 * @class Task
 * @brief Compito da svolgere: sotto-classe di Activity con scadenza
 *        (istante o giorno limite), una priorita' e uno stato di
 *        completamento PER OCCORRENZA (evaso/non evaso).
 *
 * Un compito e' spuntabile; la spunta riguarda una singola occorrenza
 * (identificata dal suo istante), non l'intero compito: le occorrenze evase
 * sono tenute in un insieme di TimePoint (m_doneOccurrences). La scadenza
 * e' l'istante di riferimento ereditato (getStart() == getDue()).
 */
class Task : public Activity {
private:
    Priority m_priority;  ///< Priorita' del compito
    std::unordered_set<TimePoint, TimePointHasher> m_doneOccurrences;  ///< Occorrenze evase

protected:
    Task* clone_impl() const override;

public:
    /** @brief Costruttore.
     *  @param title Titolo del compito
     *  @param due Scadenza (default: ora attuale)
     *  @param priority Priorita' (default: Medium)
     *  @param generator Regola di ricorrenza (default: nullptr = SingleGenerator(due))
     */
    explicit Task(const String& title = "",
                  const TimePoint due = std::chrono::time_point_cast<std::chrono::seconds>(
                      Clock::now()),
                  const Priority priority = Priority::Medium,
                  std::shared_ptr<DateGenerator> generator = nullptr);

    /** @return L'istante di scadenza */
    TimePoint getDue() const;

    /** @brief Imposta l'istante di scadenza */
    void setDue(TimePoint due);

    /** @return La priorita' del compito */
    Priority getPriority() const;

    /** @brief Imposta la priorita' del compito */
    void setPriority(Priority priority);

    /** @return true se l'occorrenza all'istante `tp` e' evasa */
    bool isDone(TimePoint tp) const;

    /** @brief Segna come evasa/non evasa l'occorrenza all'istante `tp` */
    void setDone(TimePoint tp, bool done = true);

    /** @return true se l'occorrenza (unica) del compito singolo e' evasa
     *  (alias: stato dell'occorrenza a getStart()). */
    bool isDone() const;

    /** @brief Segna come evasa/non evasa l'occorrenza singola (getStart()).
     *  @return true se `tp` (o getStart() quando assente) e' una data
     *          generabile ed e' stata aggiornata */
    bool setDone(bool done = true);

    /** @return true se un compito e' sempre spuntabile (unico tipo con stato) */
    bool isCheckable() const;

    /** @return true se l'occorrenza `tp` e' spuntata (alias di isDone(tp)) */
    bool isChecked(TimePoint tp) const;

    /** @brief Spunta/sblocca l'occorrenza `tp` (alias di setDone(tp)) */
    void setChecked(TimePoint tp, bool checked = true);

    /** @return L'insieme delle occorrenze evase (read-only) */
    const std::unordered_set<TimePoint, TimePointHasher>& getDoneOccurrences() const;

    /** @return true se l'occorrenza `tp` non e' evasa e `now` e' successivo a `tp` */
    bool isOverdue(TimePoint tp, TimePoint now) const;

    /** @return Tempo mancante a `tp` rispetto a now (negativo se scaduto) */
    Duration timeRemaining(TimePoint tp, TimePoint now) const;

    /** @return Etichetta testuale della priorita' (solo per visualizzazione) */
    static String priorityLabel(Priority priority);

    /// Implementazione dei metodi virtuali di Activity

    /** @return L'occorrenza puntuale (durata zero) per ogni data generata in [from, to] */
    std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const override;

    /** @return Descrizione testuale del compito (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso ActivityVisitor::visit(const Task&) */
    void accept(ActivityVisitor& visitor) const override;

    /** @brief Crea una copia del compito */
    std::unique_ptr<Task> clone() const;
};

} // namespace events

#endif // TASK_H