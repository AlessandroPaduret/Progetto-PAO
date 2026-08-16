#ifndef APP_ACTIVITY_LIST_PAGE_H
#define APP_ACTIVITY_LIST_PAGE_H

#include <QWidget>

#include "events/events.h"

class QLineEdit;
class QPushButton;
class QTableWidget;

namespace app {

class CalendarController;

/** @brief Pagina "elenco attivita'": tabella con ricerca live per titolo.
 *
 *  Ogni riga e' un'attivita'; il tipo e la riga descrittiva sono calcolati
 *  con un Visitor (nessuna stringa "getType" nel modello).
 */
class ActivityListPage : public QWidget {
    Q_OBJECT
public:
    explicit ActivityListPage(CalendarController* controller, QWidget* parent = nullptr);

    /** @brief Ricarica la tabella dal calendario (inclusa la ricerca corrente). */
    void refresh();

signals:
    /** @brief Doppio clic su una riga (o pulsante Info): apri il dettaglio. */
    void detailRequested(const events::Activity* activity);
    /** @brief Pulsante Modifica: apri il form per l'attivita'. */
    void editRequested(const events::Activity* activity);

private slots:
    void onSearchTextChanged(const QString& text);
    void onOpenDetail();
    void onEdit();

private:
    void reloadTable();

    CalendarController* m_controller;
    QLineEdit* m_searchBox;
    QTableWidget* m_table;
    QPushButton* m_detailButton;
    QPushButton* m_editButton;

    /** @brief Attivita' correntemente elencate (riga -> puntatore). */
    QVector<const events::Activity*> m_rows;
};

} // namespace app

#endif // APP_ACTIVITY_LIST_PAGE_H
