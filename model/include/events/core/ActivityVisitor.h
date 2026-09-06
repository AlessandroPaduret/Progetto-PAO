#pragma once

namespace events {

class Activity;
class Task;
class Meeting;

/**
 * @brief Interfaccia del Visitor per la gerarchia delle attivita.
 *
 * Permette di aggiungere operazioni polimorfe sulla gerarchia (persistenza
 * JSON, pannelli di dettaglio nella GUI, ...) senza modificare le classi
 * del modello e senza ricorrere a dynamic_cast o a metodi "getType".
 *
 * Activity e' la classe concreta "evento": la ricorrenza si deduce dal
 * generatore (Single/Fixed/Monthly/Yearly), il tipo dell'attivita' dal
 * dispatch sul tipo dinamico (Activity/Task/Meeting).
 */
class ActivityVisitor {
public:
    virtual ~ActivityVisitor() = default;

    virtual void visit(const Activity& activity) = 0;
    virtual void visit(const Task& task) = 0;
    virtual void visit(const Meeting& meeting) = 0;
};

} // namespace events
