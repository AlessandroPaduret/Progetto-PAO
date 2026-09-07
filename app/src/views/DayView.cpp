#include "views/DayView.h"

namespace app {

DayView::DayView(CalendarController* controller, QWidget* parent)
    : WeekView(controller, parent) {
    setDayCount(1);
}

} // namespace app
