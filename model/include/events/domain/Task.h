#pragma once 

#include <chrono>
#include <memory>
#include <unordered_set>
#include <vector>
#include <cstddef>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"

namespace events {

/** @brief Livelli di priorita' di un compito */
enum class Priority { Low, Medium, High };

/**
 * @class Task
 * @brief Compito da svolgere: sotto-classe di Activity con scadenza,
 *        priorita' e stato di completamento per occorrenza (m_doneOccurrences).
 */
class Task : public Activity {
private:
    Priority m_priority;                                                ///< Priorita' del compito
    std::unordered_set<TimePoint> m_doneOccurrences;  ///< Occorrenze evase

public:
    /**
     * @brief Costruttore del compito.
     * @param title Titolo del compito (obbligatorio)
     * @param due Scadenza/inizio del compito (obbligatorio)
     * @param duration Durata facoltativa (default: 0)
     * @param priority Priorita' (default: Medium)
     * @param generator Regola di ricorrenza (default: nullptr = SingleGenerator)
     * @param end Data limite di fine serie (default: max)
     * @param maxOccurrences Limite massimo di occorrenze (default: 0 = illimitate)
     */
    explicit Task(String title,
                  TimePoint due,
                  Duration duration = Duration::zero(),
                  Priority priority = Priority::Medium,
                  std::shared_ptr<const DateGenerator> generator = nullptr,
                  TimePoint end = TimePoint::max());

    ~Task() override = default;

    /** @return L'istante di scadenza (alias di getStart()) */
    TimePoint getDue() const { return getStart(); }

    /** @brief Imposta l'istante di scadenza (alias di setStart()) */
    void setDue(TimePoint due) { setStart(due); }

    /** @return La priorita' del compito */
    Priority getPriority() const { return m_priority; }

    /** @brief Imposta la priorita' del compito */
    void setPriority(Priority priority) { m_priority = priority; }

    /** @return true se l'occorrenza all'istante `tp` e' evasa */
    bool isDone(TimePoint tp) const;

    /** @brief Segna come evasa/non evasa l'occorrenza all'istante `tp` */
    void setDone(TimePoint tp, bool done = true);

    /** @return true se l'occorrenza singola/iniziale (getStart()) e' evasa */
    bool isDone() const;

    /** @brief Segna come evasa/non evasa l'occorrenza singola/iniziale */
    bool setDone(bool done = true);

    /** @return true perche' i compiti sono spuntabili */
    bool isCheckable() const { return true; }

    /** @return true se l'occorrenza `tp` e' spuntata */
    bool isChecked(TimePoint tp) const { return isDone(tp); }

    /** @brief Spunta/sblocca l'occorrenza `tp` */
    void setChecked(TimePoint tp, bool checked = true) { setDone(tp, checked); }

    /** @return L'insieme delle occorrenze evase (read-only) */
    const std::unordered_set<TimePoint>& getDoneOccurrences() const { return m_doneOccurrences; }

    /** @return true se l'occorrenza `tp` non e' evasa e `now` e' successivo a `tp` */
    bool isOverdue(TimePoint tp, TimePoint now) const;

    /** @return Tempo mancante a `tp` rispetto a now (negativo se scaduto) */
    Duration timeRemaining(TimePoint tp, TimePoint now) const;

    /** @return Etichetta testuale della priorita' */
    static String priorityLabel(Priority priority);

    /// @inheritdoc
    String describe() const override;

    /// @inheritdoc
    void accept(ActivityVisitor& visitor) const override;

    /// @inheritdoc
    [[nodiscard]] std::unique_ptr<Activity> clone() const override;
};

} // namespace events
