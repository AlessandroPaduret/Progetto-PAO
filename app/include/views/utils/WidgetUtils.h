#pragma once

#include <QStyle>
#include <QWidget>

namespace app {

/** @brief Rivaluta le regole QSS che dipendono da una proprieta' dinamica dopo
 *  un setProperty() (Qt non lo fa da solo): chiamare subito dopo ogni
 *  setProperty rilevante per lo stile (es. "selected", "inMonth", "dayState").
 *  In un header a parte perche' richiede Qt Widgets, a differenza di
 *  ViewShared.h (incluso anche da app_controller_tests, che collega solo Qt Gui). */
inline void repolish(QWidget* widget) {
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

} // namespace app
