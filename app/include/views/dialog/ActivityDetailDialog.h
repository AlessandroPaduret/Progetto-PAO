#pragma once

#include <QDialog>

#include "events.h"

class QLabel;

namespace app {

class CalendarController;

/** @brief Dialog nativo di dettaglio di un'attivita': i campi specifici per
 *  tipo (calcolati con un Visitor) piu' i pulsanti "Modifica" ed "Elimina".
 *
 *  Apertura/centraggio/modalita' sono gestiti nativamente da Qt tramite
 *  exec() (nessun posizionamento manuale, niente QFrame figlio da
 *  ricentrare a mano nel resizeEvent della finestra principale).
 */
class ActivityDetailDialog : public QDialog {
    Q_OBJECT
public:
    explicit ActivityDetailDialog(CalendarController* controller,
                                  QWidget* parent = nullptr);

    /** @brief Mostra il dettaglio dell'attivita' indicata. */
    void showActivity(const events::Activity* activity);

signals:
    /** @brief Pulsante Modifica: apri il form precompilato. */
    void editRequested(const events::Activity* activity);

private slots:
    void onEdit();
    void onDelete();

private:
    CalendarController* m_controller;
    const events::Activity* m_activity = nullptr;
    QLabel* m_titleLabel;
    QLabel* m_fieldsLabel;
};

} // namespace app
