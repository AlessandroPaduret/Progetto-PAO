#ifndef ACTIVITYVISITOR_H
#define ACTIVITYVISITOR_H

namespace events {

class Event;
class RecurrentEvent;
class Task;
class Meeting;
class AllDayEvent;
class Anniversary;

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
    virtual void visit(const Task& task) = 0;
    virtual void visit(const Meeting& meeting) = 0;
    virtual void visit(const AllDayEvent& event) = 0;
    virtual void visit(const Anniversary& anniversary) = 0;
};

} // namespace events

#endif // ACTIVITYVISITOR_H