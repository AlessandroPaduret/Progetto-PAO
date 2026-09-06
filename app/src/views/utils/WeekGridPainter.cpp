#include "views/utils/WeekGridPainter.h"

#include <QFont>
#include <QPainter>

#include "views/utils/Theme.h"
#include "views/utils/ViewShared.h"

namespace app {

namespace WeekGridPainter {

void paint(QPainter& painter, const QRect& viewport, const QDate& viewStart,
          int dayCount, int allDayHeight, const WeekGridGeometry& geometry,
          int headerFontSize, int smallFontSize) {
    const int gridTop = geometry.headerHeight + allDayHeight;

    painter.fillRect(viewport, Qt::white);

    // --- Intestazione: nome giorno + numero ---
    for (int day = 0; day < dayCount; ++day) {
        const QRect headerRect(geometry.gutterWidth + day * geometry.dayWidth, 0,
                               geometry.dayWidth, geometry.headerHeight);
        painter.fillRect(headerRect, Qt::white);

        const QDate date = viewStart.addDays(day);
        const bool isToday = date == QDate::currentDate();
        painter.setPen(isToday ? theme::kAccentBlue : theme::kSecondaryText);
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
    painter.fillRect(QRect(geometry.gutterWidth, geometry.headerHeight,
                           viewport.width() - geometry.gutterWidth, allDayHeight),
                     theme::kPanelBackground);
    painter.setPen(theme::kBorderGray);
    painter.drawLine(geometry.gutterWidth, geometry.headerHeight, viewport.width(),
                     geometry.headerHeight);
    painter.drawLine(geometry.gutterWidth, gridTop, viewport.width(), gridTop);
    for (int day = 0; day <= dayCount; ++day) {
        const int x = geometry.gutterWidth + day * geometry.dayWidth;
        painter.drawLine(x, geometry.headerHeight, x, gridTop);
    }

    // --- Linee della griglia ---
    painter.setPen(theme::kBorderGray);
    for (int day = 0; day <= dayCount; ++day) {
        const int x = geometry.gutterWidth + day * geometry.dayWidth;
        painter.drawLine(x, gridTop, x, viewport.height());
    }

    // --- Ore sul bordo sinistro + linee orizzontali ---
    QFont hourFont = painter.font();
    hourFont.setPointSize(smallFontSize);
    painter.setFont(hourFont);
    for (int hour = 0; hour < 24; ++hour) {
        const int y = gridTop + hour * geometry.hourHeight;
        painter.setPen(theme::kBorderGray);
        painter.drawLine(geometry.gutterWidth, y, viewport.width(), y);
        painter.setPen(theme::kSecondaryText);
        painter.drawText(QRect(0, y - smallFontSize, geometry.gutterWidth - 8,
                               smallFontSize * 2),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QStringLiteral("%1:00").arg(hour, 2, 10, QLatin1Char('0')));
    }
}

} // namespace WeekGridPainter

} // namespace app
