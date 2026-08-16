#ifndef APP_ACTIVITY_DETAIL_PAGE_H
#define APP_ACTIVITY_DETAIL_PAGE_H

#include <QWidget>

#include "events/events.h"

class QLabel;
class QPushButton;

namespace app {

class CalendarController;

/** @brief Pagina "dettaglio di una singola attivita'": campi calcolati con un
 *  Visitor, quindi diversi e specifici per ogni tipo concreto.
 */
class ActivityDetailPage : public QWidget {
    Q_OBJECT
public:
    explicit ActivityDetailPage(CalendarController* controller, QWidget* parent = nullptr);

    /** @brief Mostra il dettaglio dell'attivita' indicata. */
    void showActivity(const events::Activity* activity);

    /** @return L'attivita' correntemente mostrata (o nullptr). */
    const events::Activity* currentActivity() const;

signals:
    void backRequested();
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
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
};

} // namespace app

#endif // APP_ACTIVITY_DETAIL_PAGE_H
