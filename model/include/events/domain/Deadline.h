#ifndef DEADLINE_H
#define DEADLINE_H

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"

namespace events {

/** @brief Livelli di priorita' di una scadenza */
enum class Priority { Low, Medium, High };

/**
 * @class Deadline
 * @brief Attivita' puntuale con priorita' e stato di completamento:
 *        una scadenza da rispettare.
 */
class Deadline : public Activity {
private:
    TimePoint m_due;          ///< Istante di scadenza
    Priority m_priority;      ///< Priorita' della scadenza
    bool m_done;              ///< true se la scadenza e' stata evasa

protected:
    Deadline* clone_impl() const override;

public:
    /** @brief Costruttore con parametri opzionali.
     *  @param title Titolo della scadenza.
     *  @param due Istante di scadenza (default: ora attuale).
     *  @param priority Priorita' (default: Medium).
     */
    explicit Deadline(const String& title = "",
                      const TimePoint due = std::chrono::time_point_cast<std::chrono::seconds>(Clock::now()),
                      const Priority priority = Priority::Medium);

    /** @brief Operatore di output per stampare i dettagli della scadenza */
    friend std::ostream& operator<<(std::ostream& os, const Deadline& deadline);

    /** @return L'istante di scadenza */
    TimePoint getDue() const;

    /** @brief Imposta l'istante di scadenza */
    void setDue(TimePoint due);

    /** @return La priorita' della scadenza */
    Priority getPriority() const;

    /** @brief Imposta la priorita' della scadenza */
    void setPriority(Priority priority);

    /** @return true se la scadenza e' stata evasa */
    bool isDone() const;

    /** @brief Segna la scadenza come evasa/non evasa */
    void setDone(bool done = true);

    /** @return true se la scadenza non e' evasa e l'istante indicato e' successivo alla scadenza */
    bool isOverdue(TimePoint now) const;

    /** @return Tempo mancante alla scadenza rispetto a now (negativo se gia' scaduta) */
    Duration timeRemaining(TimePoint now) const;

    /** @return Etichetta testuale della priorita' (solo per visualizzazione) */
    static String priorityLabel(Priority priority);

    /// Implementazione dei metodi virtuali di Activity

    /** @return L'istante di scadenza */
    TimePoint getStart() const override;

    /** @return L'occorrenza puntuale (durata zero) se la scadenza e' in [from, to] */
    std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const override;

    /** @brief Sposta la scadenza al nuovo istante. */
    void moveTo(TimePoint newStart) override;

    /** @return Descrizione testuale della scadenza (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso ActivityVisitor::visit(const Deadline&) */
    void accept(ActivityVisitor& visitor) const override;

    /** @brief Crea una copia della scadenza */
    std::unique_ptr<Deadline> clone() const;
};

} // namespace events

#endif // DEADLINE_H
