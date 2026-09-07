#pragma once

#include "views/weekly/WeekView.h"

namespace app {

/** @brief Vista "giorno" = WeekView con una sola colonna (dayCount = 1);
 *  eredita interazione, drag&drop e anteprima live. */
class DayView : public WeekView {
public:
    explicit DayView(QWidget* parent = nullptr);
};

} // namespace app
