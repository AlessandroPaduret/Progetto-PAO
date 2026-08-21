#include "views/YearView.h"

#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include "views/ViewShared.h"

namespace app {

namespace {

constexpr int kPanelHeaderHeight = 18;  // nome del mese
constexpr int kDayNamesHeight = 12;     // riga Lun..Dom

const QColor kGridColor("#e8eaed");
const QColor kTextColor("#202124");
const QColor kFadedTextColor("#9aa0a6");
const QColor kTodayColor("#1a73e8");

} // namespace

YearView::YearView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    // Sotto le dimensioni base la griglia non scala: compaiono le scrollbar.
    setMinimumSize(baseWidth(), baseHeight());
}

int YearView::baseWidth() const {
    return 3 * 200;
}

int YearView::baseHeight() const {
    return 4 * 170;
}

void YearView::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    m_occurrences = occurrences;
    update();
}

void YearView::setYear(const QDate& januaryFirst) {
    m_year = januaryFirst;
    update();
}

QRect YearView::monthRect(int monthIndex) const {
    constexpr int cols = 3;
    constexpr int rows = 4;
    const int w = width() / cols;
    const int h = height() / rows;
    return QRect((monthIndex % cols) * w, (monthIndex / cols) * h, w, h);
}

std::optional<QDate> YearView::dateAt(const QPoint& pos) const {
    for (int m = 0; m < 12; ++m) {
        const QRect panel = monthRect(m);
        if (!panel.contains(pos)) {
            continue;
        }
        const QDate first(m_year.year(), m + 1, 1);
        const QDate gridStart = first.addDays(1 - first.dayOfWeek());
        const int weeks = (first.dayOfWeek() - 1 + first.daysInMonth() + 6) / 7;
        const int cellW = panel.width() / 7;
        const int cellH = (panel.height() - kPanelHeaderHeight - kDayNamesHeight) / weeks;
        const int col = (pos.x() - panel.left()) / cellW;
        const int row = (pos.y() - panel.top() - kPanelHeaderHeight - kDayNamesHeight) / cellH;
        if (col < 0 || col > 6 || row < 0 || row >= weeks) {
            return std::nullopt;
        }
        const QDate date = gridStart.addDays(row * 7 + col);
        if (date.year() == m_year.year() && date.month() == m + 1) {
            return date;
        }
        return std::nullopt;
    }
    return std::nullopt;
}

void YearView::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    for (int m = 0; m < 12; ++m) {
        const QRect panel = monthRect(m);
        painter.fillRect(panel, Qt::white);

        const QDate first(m_year.year(), m + 1, 1);
        const QDate gridStart = first.addDays(1 - first.dayOfWeek());
        const int weeks = (first.dayOfWeek() - 1 + first.daysInMonth() + 6) / 7;
        const int cellW = panel.width() / 7;
        const int cellH = (panel.height() - kPanelHeaderHeight - kDayNamesHeight) / weeks;
        const int gridTop = panel.top() + kPanelHeaderHeight;

        // --- Nome del mese ---
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(9);
        painter.setFont(font);
        painter.setPen(kTextColor);
        painter.drawText(QRect(panel.left(), panel.top(), panel.width(),
                               kPanelHeaderHeight),
                         Qt::AlignCenter,
                         first.toString(QStringLiteral("MMMM")));

        // --- Intestazione Lun..Dom ---
        font.setBold(false);
        font.setPointSize(6);
        painter.setFont(font);
        painter.setPen(kFadedTextColor);
        for (int d = 0; d < 7; ++d) {
            const QRect hdr(panel.left() + d * cellW, gridTop, cellW,
                            kDayNamesHeight);
            painter.drawText(hdr, Qt::AlignCenter,
                             QString::fromLatin1(shortDayName(d + 1)));
        }
        const int dayGridTop = gridTop + kDayNamesHeight;

        // --- Celle dei giorni ---
        for (int r = 0; r < weeks; ++r) {
            for (int d = 0; d < 7; ++d) {
                const QDate date = gridStart.addDays(r * 7 + d);
                const bool inMonth = date.month() == m + 1;
                const bool isToday = date == QDate::currentDate();
                const QRect cell(panel.left() + d * cellW,
                                 dayGridTop + r * cellH, cellW, cellH);

                font.setBold(isToday);
                font.setPointSize(7);
                painter.setFont(font);
                painter.setPen(isToday ? kTodayColor
                                       : (inMonth ? kTextColor : kFadedTextColor));
                painter.drawText(cell, Qt::AlignCenter,
                                 QString::number(date.day()));
            }
        }

        // --- Pallini colorati per le attivita' ---
        for (const events::Occurrence& occ : m_occurrences) {
            const QDate date = localTime(occ.start).date();
            if (date.year() != m_year.year() || date.month() != m + 1) {
                continue;
            }
            const int offset = gridStart.daysTo(date);
            const int r = offset / 7;
            const int d = offset % 7;
            if (r < 0 || r >= weeks) {
                continue;
            }
            const QRect cell(panel.left() + d * cellW,
                             dayGridTop + r * cellH, cellW, cellH);
            painter.setPen(Qt::NoPen);
            const bool done = occ.source->isDoneAt(occ.start);
            painter.setBrush(done ? QColor("#bdc1c6") : activityColor(occ.source));
            painter.drawEllipse(QPointF(cell.center().x(), cell.bottom() - 4),
                                2.5, 2.5);
        }
    }
}

void YearView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (std::optional<QDate> date = dateAt(event->pos())) {
        emit daySelected(*date);
    }
}

bool YearView::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        auto* help = static_cast<QHelpEvent*>(event);
        if (std::optional<QDate> date = dateAt(help->pos())) {
            QStringList titles;
            for (const events::Occurrence& occ : m_occurrences) {
                if (localTime(occ.start).date() == *date) {
                    const QString title =
                        QString::fromStdString(occ.source->getTitle());
                    if (!titles.contains(title)) {
                        titles.append(title);
                    }
                }
            }
            QString text = QStringLiteral("Giorno del %1")
                               .arg(date->toString(QStringLiteral("dd/MM/yyyy")));
            if (!titles.isEmpty()) {
                text += QLatin1Char('\n') + titles.join(QLatin1Char('\n'));
            }
            QToolTip::showText(help->globalPos(), text, this);
        } else {
            QToolTip::hideText();
        }
        return true;
    }
    return QWidget::event(event);
}

} // namespace app
