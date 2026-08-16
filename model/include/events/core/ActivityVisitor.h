#ifndef ACTIVITY_VISITOR_H
#define ACTIVITY_VISITOR_H

namespace events {

class Event;
class RecurrentEvent;
class Deadline;
class Reminder;

/**
 * @brief Interfaccia del Visitor per la gerarchia delle attivita.
 *
 * Permette di aggiungere operazioni polimorfe sulla gerarchia (persistenza
 * JSON, pannelli di dettaglio nella GUI, ...) senza modificare le classi
 * del modello e senza ricorrere a dynamic_cast o a metodi "getType".
 */
class ActivityVisitor {
public:
    virtual ~ActivityVisitor() = default;

    virtual void visit(const Event& event) = 0;
    virtual void visit(const RecurrentEvent& event) = 0;
    virtual void visit(const Deadline& deadline) = 0;
    virtual void visit(const Reminder& reminder) = 0;
};

} // namespace events

#endif // ACTIVITY_VISITOR_H
