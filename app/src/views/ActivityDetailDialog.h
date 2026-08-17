#ifndef APP_ACTIVITY_DETAIL_DIALOG_H
#define APP_ACTIVITY_DETAIL_DIALOG_H

#include <QFrame>

#include "events/events.h"

class QLabel;
class QToolButton;

namespace app {

class CalendarController;

/** @brief Finestra interna di dettaglio di un'attivita': un pannello ridotto
 *  mostrato DENTRO la MainWindow (widget figlio della MainWindow, quindi
 *  clipato ai suoi bordi e ricentrato in resizeEvent), con i campi
 *  specifici per tipo calcolati con un Visitor.
 *
 *  Ha una "X" in alto a destra per chiuderla e due pulsanti in basso:
 *  "Modifica" (apre il form di modifica via segnale `editRequested`) ed
 *  "Elimina".
 */
class ActivityDetailDialog : public QFrame {
    Q_OBJECT
public:
    explicit ActivityDetailDialog(CalendarController* controller,
                                  QWidget* parent = nullptr);

    /** @brief Mostra il dettaglio dell'attivita' indicata. */
    void showActivity(const events::Activity* activity);

    /** @brief Centra il pannello nella finestra principale e lo mostra. */
    void showCentered();

signals:
    /** @brief Pulsante Modifica: apri il form precompilato. */
    void editRequested(const events::Activity* activity);
    /** @brief Il pannello si e' chiuso (X, Modifica o Elimina). */
    void closed();

private slots:
    void onEdit();
    void onDelete();

private:
    CalendarController* m_controller;
    const events::Activity* m_activity = nullptr;
    QLabel* m_titleLabel;
    QLabel* m_fieldsLabel;
    QToolButton* m_closeButton;
};

} // namespace app

#endif // APP_ACTIVITY_DETAIL_DIALOG_H