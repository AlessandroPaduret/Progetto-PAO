#pragma once

#include <QWidget>

#include "events.h"

class QComboBox;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace app {

class CalendarController;

/** @brief Pagina "elenco attivita'": tabella con ricerca live + filtro per
 *  tipo. Tipo e riga descrittiva sono calcolati con un Visitor (niente
 *  "getType" nel modello). Ordinamento: clic sull'intestazione, "Titolo" usa
 *  "Tipo" come chiave secondaria (e viceversa); default per data di inizio. */
class ActivityListPage : public QWidget {
    Q_OBJECT
public:
    explicit ActivityListPage(CalendarController* controller, QWidget* parent = nullptr);

    /** @brief Ricarica dal calendario, mantenendo ricerca/filtro/ordinamento correnti. */
    void refresh();

signals:
    void detailRequested(const events::Activity* activity);
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

    /** @brief Chiave primaria = colonna ordinata, secondaria = tipo/titolo
     *  complementare, per ultimo l'inizio (stabile). */
    bool lessThan(const events::Activity* a, const events::Activity* b) const;

    CalendarController* m_controller;
    QLineEdit* m_searchBox;
    QComboBox* m_typeFilter = nullptr;
    QCheckBox* m_pendingOnly = nullptr;
    QTableWidget* m_table;
    QPushButton* m_detailButton;
    QPushButton* m_editButton;

    int m_sortColumn = -1;  // -1 = default, per data di inizio
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    QVector<const events::Activity*> m_rows;  // riga -> puntatore
};

} // namespace app
