#include "views/WeekView.h"

#include <QDate>
#include <QFont>
#include <QHelpEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

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
    // Se la finestra e' piu' larga delle dimensioni base, allarga le colonne.
    return qMax(kDayWidth, (width() - kGutterWidth) / kDaysPerWeek);
}

int WeekView::hourHeight() const {
    // Se la finestra e' piu' alta delle dimensioni base, allarga le ore.
    return qMax(kHourHeight, (height() - kHeaderHeight) / 24);
}

void WeekView::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    m_occurrences = occurrences;
    m_selected = -1;
    update();
}

void WeekView::setWeekStart(const QDate& monday) {
    m_monday = monday;
    update();
}

const events::Occurrence* WeekView::selectedOccurrence() const {
    if (m_selected < 0 || m_selected >= static_cast<int>(m_occurrences.size())) {
        return nullptr;
    }
    return &m_occurrences[m_selected];
}

QRect WeekView::occurrenceRect(const events::Occurrence& occurrence) const {
    if (!m_monday.isValid()) {
        return QRect();
    }
    const QDateTime localStart = localTime(occurrence.start);
    const QDateTime localEnd = localTime(occurrence.end());

    const int dayIndex = m_monday.daysTo(localStart.date());
    if (dayIndex < 0 || dayIndex >= kDaysPerWeek) {
        return QRect();
    }

    const int startMin = localStart.time().msecsSinceStartOfDay() / 60000;
    int endMin = localEnd.time().msecsSinceStartOfDay() / 60000;
    if (localEnd.date() != localStart.date()) {
        endMin = kMinutesPerDay;  // l'attivita' attraversa la mezzanotte
    }

    const int topMin = qBound(0, startMin, kMinutesPerDay);
    const int bottomMin = qBound(0, endMin, kMinutesPerDay);
    int height = (bottomMin - topMin) * hourHeight() / 60 - 4;
    if (height < kMinOccurrenceHeight) {
        height = kMinOccurrenceHeight;  // durata zero: chip minimo visibile
    }

    const int x = kGutterWidth + dayIndex * dayWidth() + 2;
    const int y = kHeaderHeight + topMin * hourHeight() / 60 + 2;
    return QRect(x, y, dayWidth() - 4, height);
}

int WeekView::hitTest(const QPoint& pos) const {
    for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
        if (occurrenceRect(m_occurrences[i]).contains(pos)) {
            return i;
        }
    }
    return -1;
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

    // --- Attivita' come blocchi colorati ---
    for (int i = 0; i < static_cast<int>(m_occurrences.size()); ++i) {
        const QRect rect = occurrenceRect(m_occurrences[i]);
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
            QAction* modifyAction = menu.addAction(tr("Modifica istanza"));
            QAction* deleteAction = menu.addAction(tr("Elimina"));
            QAction* chosen = menu.exec(event->globalPosition().toPoint());
            if (chosen == infoAction) {
                emit infoRequested(m_occurrences[index]);
            } else if (chosen == modifyAction) {
                emit modifyEventRequested(m_occurrences[index]);
            } else if (chosen == deleteAction) {
                emit deleteEventRequested(m_occurrences[index]);
            }
        } else {
            QAction* createAction = menu.addAction(tr("Nuova attivita'..."));
            if (menu.exec(event->globalPosition().toPoint()) == createAction) {
                const QPoint pos = event->pos();
                if (pos.y() >= kHeaderHeight) {
                    const int dayIndex = (pos.x() - kGutterWidth) / dayWidth();
                    if (dayIndex >= 0 && dayIndex < kDaysPerWeek) {
                        const int minutes =
                            (pos.y() - kHeaderHeight) * 60 / hourHeight();
                        const QTime time(qBound(0, minutes / 60, 23),
                                         qBound(0, minutes % 60, 59));
                        emit emptySlotClicked(
                            QDateTime(m_monday.addDays(dayIndex), time));
                    }
                }
            }
        }
        return;
    }

    m_selected = index;
    update();
}

void WeekView::mouseDoubleClickEvent(QMouseEvent* event) {
    const int index = hitTest(event->pos());
    if (index >= 0) {
        // Doppio clic su un'occorrenza: modifica l'attivita' sorgente
        // mantiene il tipo originale (es. ricorrente con la sua serie).
        m_selected = index;
        update();
        emit activityEditRequested(m_occurrences[index]);
        return;
    }
    const QPoint pos = event->pos();
    if (pos.y() < kHeaderHeight) {
        return;
    }
    const int dayIndex = (pos.x() - kGutterWidth) / dayWidth();
    if (dayIndex < 0 || dayIndex >= kDaysPerWeek) {
        return;
    }
    const int minutes = (pos.y() - kHeaderHeight) * 60 / hourHeight();
    const QTime time(qBound(0, minutes / 60, 23), qBound(0, minutes % 60, 59));
    emit emptySlotClicked(QDateTime(m_monday.addDays(dayIndex), time));
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
