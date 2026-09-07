#include "views/TimeGutterWidget.h"

#include <QLabel>
#include <QVBoxLayout>

#include "views/utils/WeekGridLayout.h"

namespace app {

TimeGutterWidget::TimeGutterWidget(QWidget* parent) : QWidget(parent) {
    setFixedWidth(kWeekGutterWidth);
    setFixedHeight(24 * kWeekHourHeight);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 4, 0);
    layout->setSpacing(0);

    for (int hour = 0; hour < 24; ++hour) {
        auto* label = new QLabel(QStringLiteral("%1:00").arg(hour, 2, 10, QLatin1Char('0')), this);
        label->setObjectName(QStringLiteral("weekHourLabel"));
        label->setFixedHeight(kWeekHourHeight);
        label->setAlignment(Qt::AlignRight | Qt::AlignTop);
        layout->addWidget(label);
    }
}

} // namespace app
