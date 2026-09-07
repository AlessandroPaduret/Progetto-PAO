#pragma once

namespace events {

class Activity;
class Task;
class Meeting;

/**
 * @brief Visitor per la gerarchia delle attivita'.
 *
 * Permette operazioni polimorfe (persistenza JSON, dettaglio GUI, ...)
 * senza dynamic_cast o metodi "getType".
 */
class ActivityVisitor {
public:
    virtual ~ActivityVisitor() = default;

    virtual void visit(const Activity& activity) = 0;
    virtual void visit(const Task& task) = 0;
    virtual void visit(const Meeting& meeting) = 0;
};

} // namespace events
