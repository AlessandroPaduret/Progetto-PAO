#include "views/HeaderWidget.h"

#include <QApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

#include "views/utils/ViewShared.h"
#include "views/utils/WeekGridLayout.h"
#include "views/utils/WidgetUtils.h"

namespace app {

namespace {
constexpr int kHeaderFontSize = 10;
} // namespace

HeaderWidget::HeaderWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(kWeekHeaderHeight);
    setObjectName(QStringLiteral("weekHeader"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* leadingSpacer = new QWidget(this);
    leadingSpacer->setFixedWidth(kWeekGutterWidth);
    layout->addWidget(leadingSpacer);

    // Le etichette dei giorni si inseriscono qui (indice 1..dayCount),
    // ricostruite ad ogni setDays; lo spaziatore finale resta sempre
    // l'ultimo elemento del layout.
    auto* trailingSpacer = new QWidget(this);
    trailingSpacer->setFixedWidth(qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent));
    layout->addWidget(trailingSpacer);
}

void HeaderWidget::setDays(const QDate& viewStart, int dayCount) {
    auto* boxLayout = qobject_cast<QHBoxLayout*>(layout());

    for (QLabel* label : m_dayLabels) {
        boxLayout->removeWidget(label);
        label->deleteLater();
    }
    m_dayLabels.clear();

    const bool isDayView = dayCount == 1;
    for (int day = 0; day < dayCount; ++day) {
        const QDate date = viewStart.addDays(day);
        auto* label = new QLabel(this);
        label->setObjectName(QStringLiteral("weekDayHeaderLabel"));
        label->setAlignment(Qt::AlignCenter);
        QFont font = label->font();
        font.setPointSize(kHeaderFontSize);
        const bool isToday = date == QDate::currentDate();
        font.setBold(isToday);
        label->setFont(font);
        label->setText(isDayView
                           ? date.toString(QStringLiteral("dddd d MMMM"))
                           : QStringLiteral("%1 %2")
                                 .arg(QString::fromLatin1(shortDayName(date.dayOfWeek())))
                                 .arg(date.day()));
        label->setProperty("today", isToday);
        repolish(label);

        // Inserita prima dello spaziatore finale (sempre ultimo elemento).
        boxLayout->insertWidget(boxLayout->count() - 1, label, 1);
        m_dayLabels.push_back(label);
    }
}

} // namespace app
