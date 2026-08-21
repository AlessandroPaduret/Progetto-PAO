#ifndef TASK_H
#define TASK_H

#include <chrono>
#include <memory>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"
#include "events/core/Occurrence.h"

namespace events {

/** @brief Livelli di priorita' di un compito */
enum class Priority { Low, Medium, High };

/**
 * @class Task
 * @brief Compito da svolgere: ha una scadenza (istante o giorno limite),
 *        una priorita' e uno stato di completamento (evaso/non evaso).
 *
 * Compare nella timeline come punto sulla data di scadenza; la spunta e'
 * l'interazione principale (meccanica "agenda con stati").
 */
class Task : public Activity {
private:
    TimePoint m_due;      ///< Scadenza (istante)
    Priority m_priority;  ///< Priorita' del compito

protected:
    Task* clone_impl() const override;

public:
    /** @brief Costruttore.
     *  @param title Titolo del compito
     *  @param due Scadenza (default: ora attuale)
     *  @param priority Priorita' (default: Medium)
     */
    explicit Task(const String& title = "",
                  const TimePoint due =
                      std::chrono::time_point_cast<std::chrono::seconds>(
                          Clock::now()),
                  const Priority priority = Priority::Medium);

    /** @return L'istante di scadenza */
    TimePoint getDue() const;

    /** @brief Imposta l'istante di scadenza */
    void setDue(TimePoint due);

    /** @return La priorita' del compito */
    Priority getPriority() const;

    /** @brief Imposta la priorita' del compito */
    void setPriority(Priority priority);

    /** @return true se non e' evaso e l'istante indicato e' successivo alla scadenza */
    bool isOverdue(TimePoint now) const;

    /** @return Tempo mancante alla scadenza rispetto a now (negativo se scaduto) */
    Duration timeRemaining(TimePoint now) const;

    /** @return Etichetta testuale della priorita' (solo per visualizzazione) */
    static String priorityLabel(Priority priority);

    /// Implementazione dei metodi virtuali di Activity

    /** @return L'istante di scadenza */
    TimePoint getStart() const override;

    /** @return L'occorrenza puntuale (durata zero) se la scadenza e' in [from, to] */
    std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const override;

    /** @brief Sposta il compito alla nuova scadenza. */
    void moveTo(TimePoint newStart) override;

    /** @return Descrizione testuale del compito (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso ActivityVisitor::visit(const Task&) */
    void accept(ActivityVisitor& visitor) const override;

    /** @brief Crea una copia del compito */
    std::unique_ptr<Task> clone() const;
};

} // namespace events

#endif // TASK_H