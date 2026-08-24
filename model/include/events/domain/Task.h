#ifndef TASK_H
#define TASK_H

#include <chrono>
#include <memory>
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
 *        completamento (evaso/non evaso).
 *
 * Compare nella timeline come punto sulla data di scadenza; la spunta e'
 * l'interazione principale (meccanica "agenda con stati"). La scadenza e'
 * l'istante di riferimento ereditato (getStart() == getDue()).
 */
class Task : public Activity {
private:
    Priority m_priority;  ///< Priorita' del compito
    bool m_done = false;  ///< Stato di completamento (evaso/non evaso)

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

    /** @return true se il compito e' stato evaso */
    bool isDone() const;

    /** @brief Segna il compito come evaso/non evaso */
    void setDone(bool done = true);

    /** @return true se un compito e' sempre spuntabile (unico tipo con stato) */
    bool isCheckable() const;

    /** @return true se il compito e' stato spuntato (alias di isDone) */
    bool isChecked() const;

    /** @brief Spunta/sblocca il compito (alias di setDone) */
    void setChecked(bool checked = true);

    /** @return true se non e' evaso e l'istante indicato e' successivo alla scadenza */
    bool isOverdue(TimePoint now) const;

    /** @return Tempo mancante alla scadenza rispetto a now (negativo se scaduto) */
    Duration timeRemaining(TimePoint now) const;

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