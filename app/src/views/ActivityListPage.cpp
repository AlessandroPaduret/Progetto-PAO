#include "views/ActivityListPage.h"

#include <QComboBox>
#include <QHeaderView>
#include <QIcon>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <algorithm>

#include "CalendarController.h"
#include "views/ActivityViewHelpers.h"
#include "views/ViewShared.h"

namespace app {

namespace {

/** @brief Pallino del colore dell'attivita': lungo la colonna ColorCodes le
 *  righe si riconoscono subito, come i blocchi delle griglie del calendario. */
QIcon colorDotIcon(const QColor& color) {
    QPixmap pixmap(14, 14);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(1, 1, 12, 12);
    return QIcon(pixmap);
}

QString plainTitle(const events::Activity* activity) {
    return QString::fromStdString(activity->getTitle());
}

QString itemType(const events::Activity* activity) {
    return ActivityViewHelpers::typeLabel(*activity);
}

QString itemDetail(const events::Activity* activity) {
    return ActivityViewHelpers::summaryLabel(*activity);
}

} // namespace

ActivityListPage::ActivityListPage(CalendarController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText(tr("Cerca per titolo..."));
    m_searchBox->setClearButtonEnabled(true);

    // Filtro per tipo di attivita' ("Tutti i tipi" = nessun filtro)
    m_typeFilter = new QComboBox(this);
    m_typeFilter->addItem(tr("Tutti i tipi"), QString());
    for (const QString& type :
         {tr("Evento"), tr("Ricorrente"), tr("Scadenza"), tr("Promemoria")}) {
        m_typeFilter->addItem(type, type);
    }

    // Scritte piu' grandi e leggibili anche il resto della vista
    QFont biggerFont = font();
    biggerFont.setPointSize(12);
    m_searchBox->setFont(biggerFont);
    m_typeFilter->setFont(biggerFont);

    // Colonna 0 = pallino colorato dell'attivita' (come le griglie del
    // calendario), poi Titolo / Tipo / Dettaglio
    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels(
        {QString(), tr("Titolo"), tr("Tipo"), tr("Dettaglio")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 30);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setHighlightSections(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    // Ordinamento gestito manualmente (lessThan con chiave composita)
    m_table->setSortingEnabled(false);
    // Aspetto "chiaro" come le griglie del calendario: sfondo bianco, righe
    // alternate, poche linee, spazio per respirare
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(38);
    m_table->setStyleSheet(QStringLiteral(
        "QTableWidget { background: white; alternate-background-color: #f8f9fa;"
        " border: none; outline: none; font-size: 13pt; color: #000000; }"
        "QTableWidget::item { padding: 4px 8px; color: #000000;"
        " border-right: 1px solid #dadce0; }"
        "QTableWidget::item:selected { background: #e8f0fe; color: #000000;"
        " border-right: 1px solid #dadce0; }"
        "QHeaderView::section { background: #f8f9fa; border: none;"
        " border-bottom: 1px solid #dadce0; border-right: 1px solid #dadce0;"
        " padding: 6px; font-weight: bold; color: #202124; font-size: 13pt; }"));

    m_detailButton = new QPushButton(tr("Dettaglio"), this);
    m_editButton = new QPushButton(tr("Modifica"), this);
    m_detailButton->setEnabled(false);
    m_editButton->setEnabled(false);

    auto* layout = new QVBoxLayout(this);
    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(m_searchBox, 1);
    filterRow->addWidget(m_typeFilter);
    layout->addLayout(filterRow);
    layout->addWidget(m_table, 1);
    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(m_detailButton);
    buttons->addWidget(m_editButton);
    layout->addLayout(buttons);

    connect(m_searchBox, &QLineEdit::textChanged,
            this, &ActivityListPage::onSearchTextChanged);
    connect(m_typeFilter, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ActivityListPage::onTypeFilterChanged);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &ActivityListPage::onHeaderClicked);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] {
        const bool hasSelection = !m_table->selectedItems().isEmpty();
        m_detailButton->setEnabled(hasSelection);
        m_editButton->setEnabled(hasSelection);
    });
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &ActivityListPage::onOpenDetail);
    connect(m_detailButton, &QPushButton::clicked,
            this, &ActivityListPage::onOpenDetail);
    connect(m_editButton, &QPushButton::clicked,
            this, &ActivityListPage::onEdit);
}

void ActivityListPage::refresh() {
    reloadTable();
}

void ActivityListPage::onSearchTextChanged(const QString&) {
    reloadTable();
}

void ActivityListPage::onTypeFilterChanged(int) {
    reloadTable();
}

void ActivityListPage::onHeaderClicked(int section) {
    // La colonna del pallino (0) non ha testo: cliccandola si ordina per titolo
    if (section == 0) {
        section = 1;
    }
    if (m_sortColumn == section) {
        m_sortOrder = m_sortOrder == Qt::AscendingOrder
                          ? Qt::DescendingOrder
                          : Qt::AscendingOrder;
    } else {
        m_sortColumn = section;
        m_sortOrder = Qt::AscendingOrder;
    }
    reloadTable();
}

void ActivityListPage::onOpenDetail() {
    const int row = m_table->currentRow();
    if (row >= 0 && row < m_rows.size()) {
        emit detailRequested(m_rows[row]);
    }
}

void ActivityListPage::onEdit() {
    const int row = m_table->currentRow();
    if (row >= 0 && row < m_rows.size()) {
        emit editRequested(m_rows[row]);
    }
}

bool ActivityListPage::lessThan(const events::Activity* a,
                                const events::Activity* b) const {
    // Colonne fisiche: 1=Titolo, 2=Tipo, 3=Dettaglio (la 0 e' il pallino)
    auto columnText = [](const events::Activity* activity, int column) {
        switch (column) {
        case 2:
            return itemType(activity);
        case 3:
            return itemDetail(activity);
        default:
            return plainTitle(activity);
        }
    };

    if (m_sortColumn >= 1) {
        // Chiave primaria: colonna cliccata (confronto case-insensitive)
        const int c = QString::compare(columnText(a, m_sortColumn),
                                       columnText(b, m_sortColumn),
                                       Qt::CaseInsensitive);
        if (c != 0) {
            return m_sortOrder == Qt::AscendingOrder ? c < 0 : c > 0;
        }
        // Chiave secondaria: tipo se ordino per titolo, titolo altrimenti
        const int secondary = m_sortColumn == 1 ? 2 : 1;
        const int c2 = QString::compare(columnText(a, secondary),
                                        columnText(b, secondary),
                                        Qt::CaseInsensitive);
        if (c2 != 0) {
            return m_sortOrder == Qt::AscendingOrder ? c2 < 0 : c2 > 0;
        }
    }
    // Default (nessuna colonna cliccata) e tie-break: per data di inizio
    return a->getStart() < b->getStart();
}

void ActivityListPage::reloadTable() {
    QVector<const events::Activity*> activities =
        m_controller->search(m_searchBox->text());

    // Filtro per tipo di attivita' (etichette per display, coerenti col model)
    const QString filter = m_typeFilter->currentData().toString();
    if (!filter.isEmpty()) {
        activities.erase(
            std::remove_if(activities.begin(), activities.end(),
                           [&filter](const events::Activity* activity) {
                               return ActivityViewHelpers::typeLabel(*activity) != filter;
                           }),
            activities.end());
    }

    m_rows = activities;
    std::sort(m_rows.begin(), m_rows.end(),
              [this](const events::Activity* a, const events::Activity* b) {
                  return lessThan(a, b);
              });

    // Indicatore di ordinamento sull'intestazione (default: nascosto)
    auto* header = m_table->horizontalHeader();
    if (m_sortColumn >= 0) {
        header->setSortIndicator(m_sortColumn, m_sortOrder);
        header->setSortIndicatorShown(true);
    } else {
        header->setSortIndicatorShown(false);
    }

    m_table->setRowCount(static_cast<int>(m_rows.size()));
    for (int row = 0; row < static_cast<int>(m_rows.size()); ++row) {
        const events::Activity* activity = m_rows[row];
        auto* dot = new QTableWidgetItem;
        dot->setFlags(Qt::ItemIsEnabled);  // pallino non interattivo
        dot->setIcon(colorDotIcon(activityColor(activity)));
        auto* title = new QTableWidgetItem(plainTitle(activity));
        auto* type = new QTableWidgetItem(itemType(activity));
        auto* detail = new QTableWidgetItem(itemDetail(activity));
        // Testo nero e leggibile (l'eventuale stile di default non scende sotto)
        for (QTableWidgetItem* item : {title, type, detail}) {
            item->setForeground(QColor("#000000"));
        }
        m_table->setItem(row, 0, dot);
        m_table->setItem(row, 1, title);
        m_table->setItem(row, 2, type);
        m_table->setItem(row, 3, detail);
    }
}

} // namespace app
