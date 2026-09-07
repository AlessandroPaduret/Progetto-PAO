#pragma once 

#include <chrono>
#include <memory>
#include <unordered_set>
#include <vector>
#include <cstddef>

#include "core/Activity.h"
#include "core/CommonTypes.h"
#include "core/DateGenerator.h"

namespace events {

enum class Priority { Low, Medium, High };

/**
 * @brief Compito da svolgere: sotto-classe di Activity con scadenza, priorita'
 * e completamento per-occorrenza (m_doneOccurrences, l'unico tipo con stato).
 */
class Task : public Activity {
private:
    Priority m_priority;
    std::unordered_set<TimePoint> m_doneOccurrences;

public:
    /** @brief Costruttore del compito. @param title,due Obbligatori. */
    explicit Task(String title,
                  TimePoint due,
                  Duration duration = Duration::zero(),
                  Priority priority = Priority::Medium,
                  std::shared_ptr<const DateGenerator> generator = nullptr,
                  TimePoint end = TimePoint::max());

    ~Task() override = default;

    /** @brief Alias di getStart(). */
    TimePoint getDue() const { return getStart(); }
    void setDue(TimePoint due) { setStart(due); }

    Priority getPriority() const { return m_priority; }
    void setPriority(Priority priority) { m_priority = priority; }

    bool isDone(TimePoint tp) const;
    void setDone(TimePoint tp, bool done = true);

    /** @brief Occorrenza singola/iniziale (getStart()). */
    bool isDone() const;
    bool setDone(bool done = true);

    bool isCheckable() const { return true; }
    bool isChecked(TimePoint tp) const { return isDone(tp); }
    void setChecked(TimePoint tp, bool checked = true) { setDone(tp, checked); }

    const std::unordered_set<TimePoint>& getDoneOccurrences() const { return m_doneOccurrences; }

    bool isOverdue(TimePoint tp, TimePoint now) const;
    /** @return Tempo mancante a tp rispetto a now (negativo se scaduto). */
    Duration timeRemaining(TimePoint tp, TimePoint now) const;

    static String priorityLabel(Priority priority);

    String describe() const override;
    void accept(ActivityVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<Activity> clone() const override;
};

} // namespace events
