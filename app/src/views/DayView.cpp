#include "views/DayView.h"

namespace app {

DayView::DayView(QWidget* parent) : WeekView(parent) {
    setDayCount(1);
}

} // namespace app
