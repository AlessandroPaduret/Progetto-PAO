#include "views/AllDayAreaWidget.h"

#include <QApplication>
#include <QGridLayout>
#include <QMouseEvent>
#include <QStyle>

#include <algorithm>

#include "views/OccurrenceWidget.h"
#include "views/utils/ViewShared.h"
#include "views/utils/WeekGridLayout.h"

namespace app {

AllDayAreaWidget::AllDayAreaWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName(QStringLiteral("weekAllDayArea"));
    m_grid = new QGridLayout(this);
    m_grid->setContentsMargins(0, 2, 0, 2);
    m_grid->setSpacing(2);
    setFixedHeight(kWeekAllDayRowHeight);
}

void AllDayAreaWidget::setDays(const QDate& viewStart, int dayCount) {
    // Neutralizza le colonne di una precedente dayCount piu' larga (es. il
    // passaggio Settimana(7) -> Giorno(1)): setColumnStretch/MinimumWidth
    // restano altrimenti applicati anche a colonne senza piu' alcun widget,
    // allargando il layout oltre le colonne davvero in uso.
    for (int c = 0; c < m_grid->columnCount(); ++c) {
        m_grid->setColumnStretch(c, 0);
        m_grid->setColumnMinimumWidth(c, 0);
    }

    m_viewStart = viewStart;
    m_dayCount = dayCount;

    m_grid->setColumnMinimumWidth(0, kWeekGutterWidth);
    for (int d = 0; d < dayCount; ++d) {
        m_grid->setColumnStretch(d + 1, 1);
    }
    const int scrollbarWidth = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    m_grid->setColumnMinimumWidth(dayCount + 1, scrollbarWidth);
}

void AllDayAreaWidget::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    // deleteLater(), non delete: stesso motivo di DayColumnWidget/WeekView
    // (nessun chip qui e' trascinabile, ma per coerenza con lo stesso
    // pattern usato altrove non c'e' motivo di rischiare diversamente).
    for (OccurrenceWidget* chip : m_chips) {
        chip->hide();
        chip->deleteLater();
    }
    m_chips.clear();

    const std::vector<AllDayItem> items =
        WeekGridLayout::layoutAllDayRows(occurrences, m_viewStart, m_dayCount);

    int maxRow = 0;
    for (const AllDayItem& item : items) {
        const events::Occurrence& occ = occurrences[item.index];
        auto* chip = new OccurrenceWidget(occ, OccurrenceWidget::Style::Chip,
                                          isRecurrent(occ.source), /*draggable=*/false, this);
        connect(chip, &OccurrenceWidget::pressed, this,
                [this, chip](const events::Occurrence& o) { emit chipPressed(chip, o); });
        connect(chip, &OccurrenceWidget::doneToggled, this, &AllDayAreaWidget::doneToggled);
        connect(chip, &OccurrenceWidget::infoRequested, this, &AllDayAreaWidget::infoRequested);
        connect(chip, &OccurrenceWidget::editRequested,
                this, &AllDayAreaWidget::activityEditRequested);
        connect(chip, &OccurrenceWidget::modifyInstanceRequested,
                this, &AllDayAreaWidget::modifyEventRequested);
        connect(chip, &OccurrenceWidget::deleteRequested,
                this, &AllDayAreaWidget::deleteEventRequested);
        connect(chip, &OccurrenceWidget::doubleClicked, this,
                [this](const events::Occurrence& o) {
                    if (isRecurrent(o.source) && o.start > o.source->getStart()) {
                        emit occurrenceEditChoiceRequested(o);
                    } else {
                        emit activityEditRequested(o);
                    }
                });
        m_grid->addWidget(chip, item.row, item.firstDay + 1, 1,
                          item.lastDay - item.firstDay + 1);
        chip->show();
        m_chips.push_back(chip);
        maxRow = std::max(maxRow, item.row);
    }

    setFixedHeight((maxRow + 1) * kWeekAllDayRowHeight);
}

void AllDayAreaWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit backgroundClicked();
    }
    QWidget::mousePressEvent(event);
}

} // namespace app
