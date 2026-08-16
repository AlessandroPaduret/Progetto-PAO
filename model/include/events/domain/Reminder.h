#ifndef REMINDER_H
#define REMINDER_H

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"

namespace events {

/**
 * @class Reminder
 * @brief Promemoria: attivita' puntuale con messaggio e ripetizione opzionale.
 *
 * A differenza di Deadline non ha priorita' ne' stato di completamento, ma
 * puo' ripetersi a intervalli regolari ed essere posticipato (snooze).
 */
class Reminder : public Activity {
private:
    TimePoint m_trigger;      ///< Istante di attivazione
    String m_message;         ///< Messaggio del promemoria
    Duration m_repeat;        ///< Intervallo di ripetizione (zero = una tantum)

protected:
    Reminder* clone_impl() const override;

public:
    /** @brief Costruttore con parametri opzionali.
     *  @param title Titolo del promemoria.
     *  @param trigger Istante di attivazione (default: ora attuale).
     *  @param message Messaggio del promemoria.
     *  @param repeat Intervallo di ripetizione (default: zero = una tantum).
     *  @throws std::invalid_argument se repeat e' negativo.
     */
    explicit Reminder(const String& title = "",
                      const TimePoint trigger = std::chrono::time_point_cast<std::chrono::seconds>(Clock::now()),
                      const String& message = "",
                      const Duration repeat = Duration::zero());

    /** @brief Operatore di output per stampare i dettagli del promemoria */
    friend std::ostream& operator<<(std::ostream& os, const Reminder& reminder);

    /** @return L'istante di attivazione */
    TimePoint getTrigger() const;

    /** @brief Imposta l'istante di attivazione */
    void setTrigger(TimePoint trigger);

    /** @return Il messaggio del promemoria */
    String getMessage() const;

    /** @brief Imposta il messaggio del promemoria */
    void setMessage(const String& message);

    /** @return L'intervallo di ripetizione (zero = una tantum) */
    Duration getRepeatInterval() const;

    /** @brief Imposta l'intervallo di ripetizione @throws std::invalid_argument se negativo */
    void setRepeatInterval(Duration repeat);

    /** @return true se il promemoria si ripete */
    bool isRepeating() const;

    /** @brief Posticipa l'attivazione di delay (sposta l'intera griglia se ripetuto) */
    void snooze(Duration delay);

    /// Implementazione dei metodi virtuali di Activity

    /** @return L'istante di attivazione */
    TimePoint getStart() const override;

    /** @brief Espande le attivazioni in [from, to] (una se una tantum, molteplici se ripetuto) */
    std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const override;

    /** @brief Sposta l'attivazione al nuovo istante. */
    void moveTo(TimePoint newStart) override;

    /** @return Descrizione testuale del promemoria (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso ActivityVisitor::visit(const Reminder&) */
    void accept(ActivityVisitor& visitor) const override;

    /** @brief Crea una copia del promemoria */
    std::unique_ptr<Reminder> clone() const;
};

} // namespace events

#endif // REMINDER_H
