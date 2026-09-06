#include "views/WeekView.h"

#include <QDate>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFont>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QRubberBand>

#include <algorithm>

#include "views/OccurrenceWidget.h"
#include "views/ViewShared.h"

namespace app {

namespace {
constexpr int kMinutesPerDay = 24 * 60;
} // namespace

WeekView::WeekView(QWidget* parent) : QWidget(parent) {
    setAcceptDrops(true);
    // Sotto le dimensioni base la griglia non scala: compaiono le scrollbar.
    setMinimumSize(baseWidth(), baseHeight());

    // Anteprima live: un widget vero (non un rettangolo disegnato a mano),
    // nascosto finche' non c'e' un'anteprima da mostrare.
    m_previewLabel = new QLabel(this);
    m_previewLabel->setWordWrap(true);
    m_previewLabel->setAttribute(Qt::WA_StyledBackground, true);
    m_previewLabel->setStyleSheet(QStringLiteral(
        "background: rgba(26, 115, 232, 55); border: 1px dashed #1a73e8;"
        " color: #202124; padding: 3px; border-radius: 3px;"));
    m_previewLabel->hide();

    // Evidenzia la cella di destinazione durante il drag&drop nativo:
    // QRubberBand e' il widget Qt pensato apposta per questo (si disegna da
    // solo, niente QPainter manuale sull'overlay del drag).
    m_dropIndicator = new QRubberBand(QRubberBand::Rectangle, this);
}

void WeekView::setDayCount(int days) {
    m_dayCount = qBound(1, days, kDaysPerWeek);
    relayout();
}

int WeekView::dayCount() const {
    return m_dayCount;
}

int WeekView::baseWidth() const {
    return kGutterWidth + m_dayCount * kDayWidth;
}

int WeekView::baseHeight() const {
    return kHeaderHeight + m_allDayHeight + 24 * kHourHeight;
}

int WeekView::gridTop() const {
    return kHeaderHeight + m_allDayHeight;
}

int WeekView::dayWidth() const {
    return qMax(kDayWidth, (width() - kGutterWidth) / m_dayCount);
}

int WeekView::hourHeight() const {
    return qMax(kHourHeight, (height() - gridTop()) / 24);
}

void WeekView::setWeekStart(const QDate& monday) {
    m_monday = monday;
    relayout();
}

void WeekView::setPreview(const std::optional<Preview>& preview) {
    m_preview = preview;
    relayout();
}

const std::optional<WeekView::Preview>& WeekView::preview() const {
    return m_preview;
}

const events::Occurrence* WeekView::selectedOccurrence() const {
    if (m_selected < 0 || m_selected >= static_cast<int>(m_occurrences.size())) {
        return nullptr;
    }
    return &m_occurrences[m_selected];
}

void WeekView::setSelected(int index) {
    if (m_selected == index) {
        return;
    }
    if (m_selected >= 0 && m_selected < static_cast<int>(m_widgets.size())) {
        m_widgets[m_selected]->setSelected(false);
    }
    m_selected = index;
    if (m_selected >= 0 && m_selected < static_cast<int>(m_widgets.size())) {
        m_widgets[m_selected]->setSelected(true);
    }
}

std::optional<QDateTime> WeekView::cellAt(const QPoint& pos) const {
    if (pos.y() < gridTop() || pos.x() < kGutterWidth) {
        return std::nullopt;
    }
    const int dayIndex = (pos.x() - kGutterWidth) / dayWidth();
    if (dayIndex < 0 || dayIndex >= m_dayCount) {
        return std::nullopt;
    }
    const int minutes = (pos.y() - gridTop()) * 60 / hourHeight();
    const QTime time(qBound(0, minutes / 60, 23), qBound(0, minutes % 60, 59));
    return QDateTime(m_monday.addDays(dayIndex), time);
}

QRect WeekView::slotRect(const QDateTime& localStart, const events::Duration duration) const {
    const int dayIndex = m_monday.daysTo(localStart.date());
    if (dayIndex < 0 || dayIndex >= m_dayCount) {
        return QRect();
    }
    const QDateTime localEnd = localStart.addSecs(duration.count());
    const int topMin = localStart.time().msecsSinceStartOfDay() / 60000;
    int bottomMin = localEnd.time().msecsSinceStartOfDay() / 60000;
    if (localStart.date() != localEnd.date()) {
        bottomMin = kMinutesPerDay;
    }
    const int height = qMax(kMinOccurrenceHeight,
                            (qBound(0, bottomMin, kMinutesPerDay) -
                             qBound(0, topMin, kMinutesPerDay)) *
                                hourHeight() / 60 - 4);
    const int x = kGutterWidth + dayIndex * dayWidth() + 2;
    const int y = gridTop() + qBound(0, topMin, kMinutesPerDay) * hourHeight() / 60 + 2;
    return QRect(x, y, dayWidth() - 4, height);
}

void WeekView::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    m_occurrences = occurrences;
    rebuildWidgets();
}

void WeekView::rebuildWidgets() {
    qDeleteAll(m_widgets);
    m_widgets.clear();
    m_selected = -1;
    m_widgets.reserve(m_occurrences.size());

    for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
        const events::Occurrence& occ = m_occurrences[i];
        const bool recurrent = isRecurrent(occ.source);
        const bool draggable = !coversFullDay(occ);
        auto* w = new OccurrenceWidget(occ, OccurrenceWidget::Style::Block,
                                       recurrent, draggable, this);

        connect(w, &OccurrenceWidget::pressed, this,
                [this, i](const events::Occurrence&) { setSelected(i); });
        connect(w, &OccurrenceWidget::doneToggled, this, &WeekView::doneToggled);
        connect(w, &OccurrenceWidget::infoRequested, this, &WeekView::infoRequested);
        connect(w, &OccurrenceWidget::editRequested, this, &WeekView::activityEditRequested);
        connect(w, &OccurrenceWidget::modifyInstanceRequested,
                this, &WeekView::modifyEventRequested);
        connect(w, &OccurrenceWidget::deleteRequested, this, &WeekView::deleteEventRequested);
        connect(w, &OccurrenceWidget::doubleClicked, this,
                [this, i](const events::Occurrence& occurrence) {
                    setSelected(i);
                    // Doppio clic su un'occorrenza successiva alla prima
                    // della serie: e' ambiguo, chiede serie o istanza.
                    if (isRecurrent(occurrence.source) &&
                        occurrence.start > occurrence.source->getStart()) {
                        emit occurrenceEditChoiceRequested(occurrence);
                    } else {
                        emit activityEditRequested(occurrence);
                    }
                });

        w->show();
        m_widgets.push_back(w);
    }
    relayout();
}

// ---------------------------------------------------------------------------
// Layout: striscia "tutto il giorno" + griglia a colonne per le occorrenze
// sovrapposte dello stesso giorno. A differenza della vecchia implementazione
// non si disegnano rettangoli: si posizionano i widget reali con setGeometry.
// ---------------------------------------------------------------------------
void WeekView::relayout() {
    std::vector<bool> placed(m_occurrences.size(), false);

    // --- Striscia "tutto il giorno" -----------------------------------------
    std::vector<std::vector<bool>> dayRows(m_dayCount);  // [giorno][riga] occupata
    struct AllDayItem {
        int index, firstDay, lastDay, row;
    };
    std::vector<AllDayItem> allDayItems;

    for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
        const events::Occurrence& occ = m_occurrences[i];
        if (!coversFullDay(occ)) {
            continue;
        }
        const QDate startDate = localTime(occ.start).date();
        const QDate endExclusive = localTime(occ.end()).date();
        int firstDay = m_monday.daysTo(startDate);
        int lastDay = m_monday.daysTo(endExclusive) - 1;
        if (lastDay < firstDay) {
            lastDay = firstDay;
        }
        firstDay = qBound(0, firstDay, m_dayCount - 1);
        lastDay = qBound(0, lastDay, m_dayCount - 1);

        int row = 0;
        bool free = false;
        while (!free) {
            free = true;
            for (int d = firstDay; d <= lastDay; ++d) {
                if (static_cast<int>(dayRows[d].size()) > row && dayRows[d][row]) {
                    free = false;
                    ++row;
                    break;
                }
            }
        }
        for (int d = firstDay; d <= lastDay; ++d) {
            if (static_cast<int>(dayRows[d].size()) <= row) {
                dayRows[d].resize(row + 1, false);
            }
            dayRows[d][row] = true;
        }
        allDayItems.push_back({i, firstDay, lastDay, row});
    }

    int maxRows = 1;
    for (int d = 0; d < m_dayCount; ++d) {
        maxRows = std::max(maxRows, static_cast<int>(dayRows[d].size()));
    }
    m_allDayHeight = maxRows * kAllDayHeight;

    for (const AllDayItem& item : allDayItems) {
        const int x = kGutterWidth + item.firstDay * dayWidth() + 2;
        const int w = (item.lastDay - item.firstDay + 1) * dayWidth() - 4;
        const int y = kHeaderHeight + 2 + item.row * kAllDayHeight;
        m_widgets[item.index]->setGeometry(x, y, w, kAllDayHeight - 4);
        m_widgets[item.index]->setVisible(true);
        placed[item.index] = true;
    }

    // --- Griglia oraria: layout a colonne per giorno ------------------------
    for (int day = 0; day < m_dayCount; ++day) {
        std::vector<int> dayIndex;
        for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
            if (coversFullDay(m_occurrences[i])) {
                continue;
            }
            if (m_monday.daysTo(localTime(m_occurrences[i].start).date()) == day) {
                dayIndex.push_back(i);
            }
        }
        std::sort(dayIndex.begin(), dayIndex.end(),
                  [this](int a, int b) {
                      return m_occurrences[a].start < m_occurrences[b].start;
                  });

        int k = 0;
        while (k < static_cast<int>(dayIndex.size())) {
            auto effectiveEnd = [this](const events::Occurrence& o) {
                return o.duration > events::Duration::zero()
                           ? o.end()
                           : o.start + std::chrono::minutes(1);
            };

            auto clusterStop = effectiveEnd(m_occurrences[dayIndex[k]]);
            int j = k + 1;
            while (j < static_cast<int>(dayIndex.size()) &&
                   m_occurrences[dayIndex[j]].start < clusterStop) {
                clusterStop =
                    std::max(clusterStop, effectiveEnd(m_occurrences[dayIndex[j]]));
                ++j;
            }

            std::vector<int> column;
            std::vector<events::TimePoint> columnEnd;
            for (int t = k; t < j; ++t) {
                const int idx = dayIndex[t];
                const events::TimePoint start = m_occurrences[idx].start;
                int col = 0;
                while (col < static_cast<int>(columnEnd.size()) &&
                       !(start >= columnEnd[col])) {
                    ++col;
                }
                if (col == static_cast<int>(columnEnd.size())) {
                    columnEnd.push_back(start);
                }
                column.push_back(col);
                columnEnd[col] = std::max(columnEnd[col],
                                          effectiveEnd(m_occurrences[idx]));
            }
            const int clusterCols = std::max(1, static_cast<int>(columnEnd.size()));

            for (int t = k; t < j; ++t) {
                const int idx = dayIndex[t];
                const events::Occurrence& occ = m_occurrences[idx];
                const QDateTime localStart = localTime(occ.start);
                const QDateTime localEnd = localTime(occ.end());

                const int topMin = localStart.time().msecsSinceStartOfDay() / 60000;
                int bottomMin = localEnd.time().msecsSinceStartOfDay() / 60000;
                if (localEnd.date() != localStart.date()) {
                    bottomMin = kMinutesPerDay;
                }
                const int lo = qBound(0, topMin, kMinutesPerDay);
                const int hi = qBound(0, bottomMin, kMinutesPerDay);
                int h = (hi - lo) * hourHeight() / 60 - 4;
                if (h < kMinOccurrenceHeight) {
                    h = kMinOccurrenceHeight;
                }

                const int colWidth = dayWidth() / clusterCols;
                const int x = kGutterWidth + day * dayWidth() +
                              column[t - k] * colWidth + 2;
                const int y = gridTop() + lo * hourHeight() / 60 + 2;
                m_widgets[idx]->setGeometry(x, y, colWidth - 4, h);
                m_widgets[idx]->setVisible(true);
                placed[idx] = true;
            }
            k = j;
        }
    }

    // Occorrenze fuori dai giorni mostrati (dayCount ridotto): nascoste.
    for (int i = 0; i < static_cast<int>(m_widgets.size()); ++i) {
        if (!placed[i]) {
            m_widgets[i]->setVisible(false);
        }
    }

    if (m_preview) {
        const QRect r = slotRect(m_preview->start, m_preview->duration);
        m_previewLabel->setText(m_preview->title);
        m_previewLabel->setVisible(r.isValid());
        if (r.isValid()) {
            m_previewLabel->setGeometry(r);
            m_previewLabel->raise();
        }
    } else {
        m_previewLabel->hide();
    }

    setMinimumSize(baseWidth(), baseHeight());
    update();
}

void WeekView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    relayout();
}

void WeekView::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    // Scala dei font proporzionale all'ingrandimento della griglia
    const qreal scale = qMin(static_cast<qreal>(dayWidth()) / kDayWidth,
                             static_cast<qreal>(hourHeight()) / kHourHeight);
    const int headerFontSize = qMax(10, qRound(10 * scale));
    const int smallFontSize = qMax(8, qRound(8 * scale));

    // --- Intestazione: nome giorno + numero ---
    for (int day = 0; day < m_dayCount; ++day) {
        const QRect headerRect(kGutterWidth + day * dayWidth(), 0,
                               dayWidth(), kHeaderHeight);
        painter.fillRect(headerRect, Qt::white);

        const QDate date = m_monday.addDays(day);
        const bool isToday = date == QDate::currentDate();
        painter.setPen(isToday ? QColor("#1a73e8") : QColor("#5f6368"));
        QFont font = painter.font();
        font.setBold(isToday);
        font.setPointSize(headerFontSize);
        painter.setFont(font);
        painter.drawText(headerRect, Qt::AlignCenter,
                         QStringLiteral("%1 %2")
                             .arg(QString::fromLatin1(shortDayName(date.dayOfWeek())))
                             .arg(date.day()));
    }

    // --- Striscia "tutto il giorno" (sfondo + separatori) ---
    painter.fillRect(QRect(kGutterWidth, kHeaderHeight, width() - kGutterWidth,
                           m_allDayHeight),
                     QColor("#f8f9fa"));
    painter.setPen(QColor("#dadce0"));
    painter.drawLine(kGutterWidth, kHeaderHeight, width(), kHeaderHeight);
    painter.drawLine(kGutterWidth, gridTop(), width(), gridTop());
    for (int day = 0; day <= m_dayCount; ++day) {
        const int x = kGutterWidth + day * dayWidth();
        painter.drawLine(x, kHeaderHeight, x, gridTop());
    }

    // --- Linee della griglia ---
    painter.setPen(QColor("#dadce0"));
    for (int day = 0; day <= m_dayCount; ++day) {
        const int x = kGutterWidth + day * dayWidth();
        painter.drawLine(x, gridTop(), x, height());
    }

    // --- Ore sul bordo sinistro + linee orizzontali ---
    QFont hourFont = painter.font();
    hourFont.setPointSize(smallFontSize);
    painter.setFont(hourFont);
    for (int hour = 0; hour < 24; ++hour) {
        const int y = gridTop() + hour * hourHeight();
        painter.setPen(QColor("#dadce0"));
        painter.drawLine(kGutterWidth, y, width(), y);
        painter.setPen(QColor("#5f6368"));
        painter.drawText(QRect(0, y - smallFontSize, kGutterWidth - 8,
                               smallFontSize * 2),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QStringLiteral("%1:00").arg(hour, 2, 10,
                                                     QLatin1Char('0')));
    }
}

void WeekView::mousePressEvent(QMouseEvent* event) {
    // Le occorrenze sono widget figli: se il clic arriva qui e' su una
    // cella vuota (Qt ha gia' fatto l'hit-test consegnando l'evento al
    // figlio quando serviva).
    if (event->button() == Qt::RightButton) {
        QMenu menu(this);
        QAction* createAction = menu.addAction(tr("Nuova attivita'..."));
        if (menu.exec(event->globalPosition().toPoint()) == createAction) {
            if (std::optional<QDateTime> cell = cellAt(event->pos())) {
                emit emptySlotClicked(*cell);
            }
        }
        return;
    }
    if (event->button() == Qt::LeftButton) {
        setSelected(-1);
    }
    QWidget::mousePressEvent(event);
}

void WeekView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (std::optional<QDateTime> cell = cellAt(event->pos())) {
        emit emptySlotClicked(*cell);
    }
}

void WeekView::dragEnterEvent(QDragEnterEvent* event) {
    if (qobject_cast<OccurrenceWidget*>(event->source())) {
        event->acceptProposedAction();
    }
}

void WeekView::dragMoveEvent(QDragMoveEvent* event) {
    auto* source = qobject_cast<OccurrenceWidget*>(event->source());
    if (!source) {
        event->ignore();
        return;
    }
    if (std::optional<QDateTime> cell = cellAt(event->position().toPoint())) {
        const QRect r = slotRect(*cell, source->occurrence().duration);
        if (r.isValid()) {
            event->acceptProposedAction();
            m_dropIndicator->setGeometry(r);
            m_dropIndicator->show();
            return;
        }
    }
    event->ignore();
    m_dropIndicator->hide();
}

void WeekView::dragLeaveEvent(QDragLeaveEvent*) {
    m_dropIndicator->hide();
}

void WeekView::dropEvent(QDropEvent* event) {
    m_dropIndicator->hide();
    auto* source = qobject_cast<OccurrenceWidget*>(event->source());
    const std::optional<QDateTime> cell = cellAt(event->position().toPoint());
    if (!source || !cell) {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    const events::Occurrence occurrence = source->occurrence();
    // Se trascino un'occorrenza successiva alla prima della serie, chiede se
    // spostare la serie o la singola occorrenza.
    if (isRecurrent(occurrence.source) && occurrence.start > occurrence.source->getStart()) {
        emit occurrenceDragChoiceRequested(occurrence, *cell);
    } else {
        emit activityMoved(occurrence, *cell);
    }
}

} // namespace app
