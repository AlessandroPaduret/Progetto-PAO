#pragma once

#include <QStyle>
#include <QWidget>

namespace app {

/** @brief Rivaluta le regole del foglio di stile (resources/style.qss) che
 *  dipendono da una proprieta' dinamica dopo che il C++ l'ha cambiata con
 *  setProperty(): Qt non lo fa da solo (a differenza del semplice
 *  ridisegno). Va chiamata subito dopo ogni setProperty rilevante per lo
 *  stile (es. "selected", "inMonth", "dayState").
 *
 *  In un header a parte (non ViewShared.h) perche' richiede Qt Widgets:
 *  ViewShared.h resta incluso anche da app_controller_tests, che collega
 *  solo Qt Gui (pao_core e' testabile senza Widgets). */
inline void repolish(QWidget* widget) {
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

} // namespace app
