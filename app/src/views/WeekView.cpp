#include "views/WeekView.h"

#include <QDate>
#include <QFont>
#include <QHelpEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include <algorithm>

#include "events/domain/RecurrentEvent.h"

namespace app {

namespace {

const QColor kPalette[] = {
    QColor("#4285F4"), QColor("#EA4335"), QColor("#34A853"), QColor("#FBBC04"),
    QColor("#A142F4"), QColor("#24C1E0"), QColor("#F28B82"), QColor("#81C995"),
};

const char* kDayNames[] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};

constexpr int kMinutesPerDay = 24 * 60;

// Colore stabile per attivita': deriva dall'indirizzo dell'oggetto.
QColor colorForActivity(const events::Activity* activity) {
    constexpr int count = sizeof(kPalette) / sizeof(kPalette[0]);
    const auto address = reinterpret_cast<quintptr>(activity);
    return kPalette[(address >> 4) % count];
}

QDateTime localTime(const events::TimePoint tp) {
    return QDateTime::fromSecsSinceEpoch(tp.time_since_epoch().count())
        .toLocalTime();
}

} // namespace

WeekView::WeekView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    // Sotto le dimensioni base la griglia non scala: compaiono le scrollbar.
    setMinimumSize(baseWidth(), baseHeight());
}

int WeekView::baseWidth() const {
    return kGutterWidth + kDaysPerWeek * kDayWidth;
}

int WeekView::baseHeight() const {
    return kHeaderHeight + 24 * kHourHeight;
}

int WeekView::dayWidth() const {
    return qMax(kDayWidth, (width() - kGutterWidth) / kDaysPerWeek);
}

int WeekView::hourHeight() const {
    return qMax(kHourHeight, (height() - kHeaderHeight) / 24);
}

QTime WeekView::localTimeOf(const events::Occurrence& occurrence) const {
    return localTime(occurrence.start).time();
}

int WeekView::minuteOf(const QTime& time) const {
    return time.msecsSinceStartOfDay() / 60000;
}

void WeekView::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    m_occurrences = occurrences;
    m_selected = -1;
    m_dragActive = false;
    m_dragMoved = false;
    m_dragIndex = -1;
    m_dropCell.reset();
    m_rects.clear();
    update();
}

void WeekView::setWeekStart(const QDate& monday) {
    m_monday = monday;
    m_dropCell.reset();
    update();
}

const events::Occurrence* WeekView::selectedOccurrence() const {
    if (m_selected < 0 || m_selected >= static_cast<int>(m_occurrences.size())) {
        return nullptr;
    }
    return &m_occurrences[m_selected];
}

// ---------------------------------------------------------------------------
// Layout a colonne: le occorrenze sovrapposte dello stesso giorno vengono
// affiancate, cosi' possono coesistere nella stessa casella.
// ---------------------------------------------------------------------------
void WeekView::ensureRects() {
    m_rects.assign(m_occurrences.size(), QRect());

    for (int day = 0; day < kDaysPerWeek; ++day) {
        // Indici delle occorrenze del giorno (ordinate per inizio)
        std::vector<int> dayIndex;
        for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
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
            // Fine effettiva (durata zero -> +1 minuto per l'affiancamento)
            auto effectiveEnd = [this](const events::Occurrence& o) {
                return o.duration > events::Duration::zero()
                           ? o.end()
                           : o.start + std::chrono::minutes(1);
            };

            // Cluster: occorrenze sovrapposte (transitivamente)
            auto clusterStop = effectiveEnd(m_occurrences[dayIndex[k]]);
            int j = k + 1;
            while (j < static_cast<int>(dayIndex.size()) &&
                   m_occurrences[dayIndex[j]].start < clusterStop) {
                clusterStop =
                    std::max(clusterStop, effectiveEnd(m_occurrences[dayIndex[j]]));
                ++j;
            }

            // Assegnazione greedy delle colonne
            std::vector<int> column;                       // colonna per indice
            std::vector<events::TimePoint> columnEnd;      // fine ultimo evento per colonna
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

            // Rects: colonne affiancate larghe dayWidth/clusterCols
            for (int t = k; t < j; ++t) {
                const int idx = dayIndex[t];
                const events::Occurrence& occ = m_occurrences[idx];
                const QDateTime localStart = localTime(occ.start);
                const QDateTime localEnd = localTime(occ.end());

                const int topMin = minuteOf(localStart.time());
                int bottomMin = minuteOf(localEnd.time());
                if (localEnd.date() != localStart.date()) {
                    bottomMin = kMinutesPerDay;  // attraversa la mezzanotte
                }
                const int lo = qBound(0, topMin, kMinutesPerDay);
                const int hi = qBound(0, bottomMin, kMinutesPerDay);
                int height = (hi - lo) * hourHeight() / 60 - 4;
                if (height < kMinOccurrenceHeight) {
                    height = kMinOccurrenceHeight;
                }

                const int colWidth = dayWidth() / clusterCols;
                const int x = kGutterWidth + day * dayWidth() +
                              column[t - k] * colWidth + 2;
                const int y = kHeaderHeight + lo * hourHeight() / 60 + 2;
                m_rects[idx] = QRect(x, y, colWidth - 4, height);
            }
            k = j;
        }
    }
}

int WeekView::hitTest(const QPoint& pos) const {
    // Ricalcola il layout se necessario (paint precedente) per un hit corretto
    const_cast<WeekView*>(this)->ensureRects();
    for (int i = 0; i < static_cast<int>(m_rects.size()); ++i) {
        if (m_rects[i].contains(pos)) {
            return i;
        }
    }
    return -1;
}

std::optional<QDateTime> WeekView::cellAt(const QPoint& pos) const {
    if (pos.y() < kHeaderHeight || pos.x() < kGutterWidth) {
        return std::nullopt;
    }
    const int dayIndex = (pos.x() - kGutterWidth) / dayWidth();
    if (dayIndex < 0 || dayIndex >= kDaysPerWeek) {
        return std::nullopt;
    }
    const int minutes = (pos.y() - kHeaderHeight) * 60 / hourHeight();
    const QTime time(qBound(0, minutes / 60, 23), qBound(0, minutes % 60, 59));
    return QDateTime(m_monday.addDays(dayIndex), time);
}

QRect WeekView::dragGhostRect(const QDateTime& localStart,
                              const events::Duration duration) const {
    const int dayIndex = m_monday.daysTo(localStart.date());
    if (dayIndex < 0 || dayIndex >= kDaysPerWeek) {
        return QRect();
    }

    const QDateTime localEnd = localStart.addSecs(duration.count());
    const int topMin = minuteOf(localStart.time());
    int bottomMin = minuteOf(localEnd.time());
    if (localStart.date() != localEnd.date()) {
        bottomMin = kMinutesPerDay;
    }
    const int height = qMax(kMinOccurrenceHeight,
                            (qBound(0, bottomMin, kMinutesPerDay) -
                             qBound(0, topMin, kMinutesPerDay)) *
                                hourHeight() / 60 - 4);
    const int x = kGutterWidth + dayIndex * dayWidth() + 2;
    const int y = kHeaderHeight + qBound(0, topMin, kMinutesPerDay) *
                                      hourHeight() / 60 + 2;
    return QRect(x, y, dayWidth() - 4, height);
}

void WeekView::paintEvent(QPaintEvent*) {
    ensureRects();
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    // Scala dei font proporzionale all'ingrandimento della griglia
    const qreal scale = qMin(static_cast<qreal>(dayWidth()) / kDayWidth,
                             static_cast<qreal>(hourHeight()) / kHourHeight);
    const int headerFontSize = qMax(10, qRound(10 * scale));
    const int smallFontSize = qMax(8, qRound(8 * scale));

    // --- Intestazione: nome giorno + numero ---
    for (int day = 0; day < kDaysPerWeek; ++day) {
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
                             .arg(QString::fromLatin1(kDayNames[day]))
                             .arg(date.day()));
    }

    // --- Linee della griglia ---
    painter.setPen(QColor("#dadce0"));
    painter.drawLine(kGutterWidth, kHeaderHeight, width(), kHeaderHeight);
    for (int day = 0; day <= kDaysPerWeek; ++day) {
        const int x = kGutterWidth + day * dayWidth();
        painter.drawLine(x, kHeaderHeight, x, height());
    }

    // --- Ore sul bordo sinistro + linee orizzontali ---
    QFont hourFont = painter.font();
    hourFont.setPointSize(smallFontSize);
    painter.setFont(hourFont);
    for (int hour = 0; hour < 24; ++hour) {
        const int y = kHeaderHeight + hour * hourHeight();
        painter.setPen(QColor("#dadce0"));
        painter.drawLine(kGutterWidth, y, width(), y);
        painter.setPen(QColor("#5f6368"));
        painter.drawText(QRect(0, y - smallFontSize, kGutterWidth - 8,
                               smallFontSize * 2),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QStringLiteral("%1:00").arg(hour, 2, 10,
                                                     QLatin1Char('0')));
    }

    // --- Evidenziazione della cella di destinazione durante il drag ---
    if (m_dragActive && m_dragMoved && m_dropCell &&
        m_dragIndex >= 0 && m_dragIndex < static_cast<int>(m_occurrences.size())) {
        const events::Occurrence& dragged = m_occurrences[m_dragIndex];
        const QRect ghost = dragGhostRect(*m_dropCell, dragged.duration);
        if (!ghost.isValid()) {
            QColor overlay("#1a73e8");
            overlay.setAlpha(12);
            painter.fillRect(QRect(kGutterWidth, kHeaderHeight,
                                   width() - kGutterWidth,
                                   height() - kHeaderHeight),
                             overlay);
        } else {
            QColor overlay("#1a73e8");
            overlay.setAlpha(25);
            painter.fillRect(ghost, overlay);
            painter.setPen(QColor("#1a73e8"));
            painter.drawRect(ghost);
        }
    }

    // --- Attivita' come blocchi colorati (layout a colonne) ---
    for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
        const QRect rect = m_rects[i];
        if (!rect.isValid()) {
            continue;
        }
        const events::Occurrence& occurrence = m_occurrences[i];
        const QColor color = colorForActivity(occurrence.source);

        painter.setPen(i == m_selected ? QPen(QColor("#1a73e8"), 2)
                                       : QPen(color.darker(120), 1));
        painter.setBrush(color.lighter(150));
        painter.drawRect(rect);

        QFont eventFont = painter.font();
        eventFont.setPointSize(smallFontSize);
        painter.setFont(eventFont);
        painter.setPen(QColor("#202124"));

        QString text = QString::fromStdString(occurrence.source->getTitle());
        if (i == m_selected) {
            text = localTime(occurrence.start).toString(QStringLiteral("HH:mm")) +
                   QLatin1Char(' ') + text;
        }
        painter.drawText(rect.adjusted(4, 3, -4, -3),
                         Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text);
    }
}

void WeekView::mousePressEvent(QMouseEvent* event) {
    const int index = hitTest(event->pos());

    if (event->button() == Qt::RightButton) {
        QMenu menu(this);
        if (index >= 0) {
            m_selected = index;
            update();
            QAction* infoAction = menu.addAction(tr("Info"));
            QAction* modifyAction = menu.addAction(tr("Modifica"));
            // Per un'attivita' ricorrente: modifica sia la serie sia la singola
            // occorrenza (istanza) selezionata.
            const bool isRecurrent =
                dynamic_cast<const events::RecurrentEvent*>(
                    m_occurrences[index].source) != nullptr;
            QAction* modifyInstanceAction =
                isRecurrent ? menu.addAction(tr("Modifica istanza")) : nullptr;
            QAction* deleteAction = menu.addAction(tr("Elimina"));
            QAction* chosen = menu.exec(event->globalPosition().toPoint());
            if (chosen == infoAction) {
                emit infoRequested(m_occurrences[index]);
            } else if (chosen == modifyAction) {
                // Modifica la serie (o l'attivita' singola)
                emit activityEditRequested(m_occurrences[index]);
            } else if (chosen == modifyInstanceAction) {
                emit modifyEventRequested(m_occurrences[index]);
            } else if (chosen == deleteAction) {
                emit deleteEventRequested(m_occurrences[index]);
            }
        } else {
            QAction* createAction = menu.addAction(tr("Nuova attivita'..."));
            if (menu.exec(event->globalPosition().toPoint()) == createAction) {
                if (std::optional<QDateTime> cell = cellAt(event->pos())) {
                    emit emptySlotClicked(*cell);
                }
            }
        }
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_selected = index;
        // Avvia il potenziale trascinamento se si preme su un'occorrenza
        m_dragActive = index >= 0;
        m_dragMoved = false;
        m_dragIndex = index;
        m_dragPressPos = event->pos();
        m_dropCell.reset();
        update();
    }
}

void WeekView::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragActive && m_dragIndex >= 0 &&
        m_dragIndex < static_cast<int>(m_occurrences.size())) {
        if (!m_dragMoved &&
            (event->pos() - m_dragPressPos).manhattanLength() > kDragThresholdPx) {
            m_dragMoved = true;
        }
        if (m_dragMoved) {
            m_dropCell = cellAt(event->pos());
            update();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void WeekView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_dragActive && event->button() == Qt::LeftButton) {
        m_dragActive = false;
        if (m_dragMoved && m_dropCell && m_dragIndex >= 0 &&
            m_dragIndex < static_cast<int>(m_occurrences.size())) {
            const events::Occurrence& occurrence = m_occurrences[m_dragIndex];
            // Se trascino un'occorrenza successiva alla prima della serie,
            // chiede se spostare la serie o la singola occorrenza.
            if (const auto* recurrent =
                    dynamic_cast<const events::RecurrentEvent*>(occurrence.source)) {
                if (occurrence.start > recurrent->getStart()) {
                    emit occurrenceDragChoiceRequested(occurrence, *m_dropCell);
                    m_dragIndex = -1;
                    m_dragMoved = false;
                    m_dropCell.reset();
                    update();
                    return;
                }
            }
            emit activityMoved(occurrence, *m_dropCell);
        }
        m_dragIndex = -1;
        m_dragMoved = false;
        m_dropCell.reset();
        update();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void WeekView::mouseDoubleClickEvent(QMouseEvent* event) {
    const int index = hitTest(event->pos());
    if (index >= 0) {
        // Doppio clic su un'occorrenza: modifica l'attivita' sorgente e
        // mantiene il tipo originale. Se l'occorrenza NON e' la prima della
        // serie, chiede se agire sulla serie o sulla singola occorrenza.
        m_selected = index;
        update();
        const events::Occurrence& occurrence = m_occurrences[index];
        if (const auto* recurrent =
                dynamic_cast<const events::RecurrentEvent*>(occurrence.source)) {
            if (occurrence.start > recurrent->getStart()) {
                emit occurrenceEditChoiceRequested(occurrence);
                return;
            }
        }
        emit activityEditRequested(occurrence);
        return;
    }
    if (std::optional<QDateTime> cell = cellAt(event->pos())) {
        emit emptySlotClicked(*cell);
    }
}

bool WeekView::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        auto* help = static_cast<QHelpEvent*>(event);
        const int index = hitTest(help->pos());
        if (index >= 0) {
            const events::Occurrence& occurrence = m_occurrences[index];
            const QDateTime localStart = localTime(occurrence.start);
            const QDateTime localEnd = localTime(occurrence.end());
            const QString text =
                QString::fromStdString(occurrence.source->getTitle());
            QToolTip::showText(help->globalPos(),
                               QStringLiteral("%1\n%2 \u2013 %3")
                                   .arg(text,
                                        localStart.toString(
                                            QStringLiteral("dd/MM/yyyy HH:mm")),
                                        localEnd.toString(
                                            QStringLiteral("HH:mm"))),
                               this);
        } else {
            QToolTip::hideText();
        }
        return true;
    }
    return QWidget::event(event);
}

} // namespace app
