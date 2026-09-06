#ifndef APP_ACTIVITY_LIST_PAGE_H
#define APP_ACTIVITY_LIST_PAGE_H

#include <QWidget>

#include "events/events.h"

class QComboBox;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace app {

class CalendarController;

/** @brief Pagina "elenco attivita'": tabella con ricerca live per titolo e
 *  filtro per tipo.
 *
 *  Ogni riga e' un'attivita'; il tipo e la riga descrittiva sono calcolati
 *  con un Visitor (nessuna stringa "getType" nel modello). La prima colonna
 *  e' un pallino del colore dell'attivita' (come nelle griglie del
 *  calendario); le righe alternate e il fondo bianco rendono la vista
 *  chiara e leggibile.
 *
 *  Ordinamento: clic sull'intestazione di una colonna riordina le righe;
 *  sulla colonna "Titolo" l'ordinamento e' per titolo con "Tipo" come chiave
 *  secondaria (e viceversa su "Tipo"). Default: per data di inizio.
 */
class ActivityListPage : public QWidget {
    Q_OBJECT
public:
    explicit ActivityListPage(CalendarController* controller, QWidget* parent = nullptr);

    /** @brief Ricarica la tabella dal calendario (inclusa la ricerca corrente
     *  e il filtro per tipo, mantenendo ordinamento e indicatore). */
    void refresh();

signals:
    /** @brief Doppio clic su una riga (o pulsante Info): apri il dettaglio. */
    void detailRequested(const events::Activity* activity);
    /** @brief Pulsante Modifica: apri il form per l'attivita'. */
    void editRequested(const events::Activity* activity);

private slots:
    void onSearchTextChanged(const QString& text);
    void onTypeFilterChanged(int index);
    void onHeaderClicked(int section);
    void onOpenDetail();
    void onEdit();
    void onPendingOnlyToggled(bool checked);

private:
    void reloadTable();

    /** @brief Confronto con chiave primaria = colonna correntemente ordinata,
     *  secondaria = tipo/titolo complementare; per ultimo l'inizio (stabile). */
    bool lessThan(const events::Activity* a, const events::Activity* b) const;

    CalendarController* m_controller;
    QLineEdit* m_searchBox;
    QComboBox* m_typeFilter = nullptr;
    QCheckBox* m_pendingOnly = nullptr;
    QTableWidget* m_table;
    QPushButton* m_detailButton;
    QPushButton* m_editButton;

    /** @brief Colonna ordinata (-1 = default, per data di inizio). */
    int m_sortColumn = -1;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    /** @brief Attivita' correntemente elencate (riga -> puntatore). */
    QVector<const events::Activity*> m_rows;
};

} // namespace app

#endif // APP_ACTIVITY_LIST_PAGE_H
