#pragma once

#include <QWidget>

#include "domain/Task.h"

class QCheckBox;
class QComboBox;

namespace app {

/** @brief Sezione specifica del Compito (priorita' + stato evaso), estratta
 *  da ActivitySidebarWidget in un widget figlio autonomo. */
class TaskFormWidget : public QWidget {
    Q_OBJECT
public:
    explicit TaskFormWidget(QWidget* parent = nullptr);

    events::Priority priority() const;
    void setPriority(events::Priority priority);

    bool isDone() const;
    void setDone(bool done);

    /** @brief In creazione lo stato "Evaso" non ha senso (non esiste ancora
     *  nessuna occorrenza passata da spuntare): il chiamante disabilita la
     *  checkbox, cosi' il valore scritto in fase di costruzione resta il
     *  default "da fare" a prescindere dallo stato visivo precedente. */
    void setDoneEnabled(bool enabled);
    bool isDoneEnabled() const;

    /** @brief Riporta il pannello ai valori di default (nuova attivita'). */
    void resetToDefaults();

private:
    QComboBox* m_priorityCombo = nullptr;
    QCheckBox* m_doneCheck = nullptr;
};

} // namespace app
